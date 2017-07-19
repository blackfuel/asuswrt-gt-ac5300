/*
* <:copyright-BRCM:2013:proprietary:standard
* 
*    Copyright (c) 2013 Broadcom 
*    All Rights Reserved
* 
*  This program is the proprietary software of Broadcom and/or its
*  licensors, and may only be used, duplicated, modified or distributed pursuant
*  to the terms and conditions of a separate, written license agreement executed
*  between you and Broadcom (an "Authorized License").  Except as set forth in
*  an Authorized License, Broadcom grants no license (express or implied), right
*  to use, or waiver of any kind with respect to the Software, and Broadcom
*  expressly reserves all rights in and to the Software and all intellectual
*  property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE
*  NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY
*  BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.
* 
*  Except as expressly set forth in the Authorized License,
* 
*  1. This program, including its structure, sequence and organization,
*     constitutes the valuable trade secrets of Broadcom, and you shall use
*     all reasonable efforts to protect the confidentiality thereof, and to
*     use this information only in connection with your use of Broadcom
*     integrated circuit products.
* 
*  2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS"
*     AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS OR
*     WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
*     RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND
*     ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT,
*     FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR
*     COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE
*     TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT OF USE OR
*     PERFORMANCE OF THE SOFTWARE.
* 
*  3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
*     ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
*     INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY
*     WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
*     IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES;
*     OR (ii) ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
*     SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS
*     SHALL APPLY NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY
*     LIMITED REMEDY.
* :> 
*/

/*
 * rdpa_epon.c
 *
 * EPON driver
 */
#include "bdmf_dev.h"
#include "rdpa_api.h"
#include "rdpa_int.h"
#include "rdpa_epon.h"
#include "rdpa_ag_epon.h"
#include "rdpa_platform.h"
#include "bcm_epon_common.h"
#ifdef BDMF_DRIVER_GPL_LAYER
extern bdmf_type_handle (*f_rdpa_epon_drv)(void);
extern void (*f_epon_get_mode)(rdpa_epon_mode* const mode);
#endif

bdmf_boolean rdpa_is_epon_mode(void);

typedef struct
{
    rdpa_epon_rate epon_rate;
    rdpa_epon_mode epon_mode;
    uint8_t max_link_count;
    uint8_t registered_links;
    rdpa_epon_mcast_link_t mcast_link[RDPA_EPON_MAX_LLID];
    rdpa_epon_fec_enable_t fec_enable[RDPA_EPON_MAX_LLID];
} epon_drv_priv_t;

/* epon laser tx mode enum values */
const bdmf_attr_enum_table_t rdpa_epon_laser_tx_mode_enum_table =
{
    .type_name = "rdpa_epon_laser_tx_mode", .help = "EPON laser tx mode",
    .values = {
        {"off", rdpa_epon_laser_tx_off},
        {"burst", rdpa_epon_laser_tx_burst},
        {"continuous", rdpa_epon_laser_tx_continuous},
        {NULL, 0}
    }
};

const bdmf_attr_enum_table_t rdpa_epon_link_mpcp_state_enum_table = 
{
    .type_name = "rdpa_epon_link_mpcp_state", .help = "Link MPCP state",
    .values = {
        {"unregistered", rdpa_epon_link_unregistered},
        {"registering", rdpa_epon_link_registering},
        {"awaiting_register", rdpa_epon_link_awaiting_register},
        {"in_service", rdpa_epon_link_in_service},
        {"awaiting_gate", rdpa_epon_link_awaiting_gate},
        {NULL, 0}
    }
};

const bdmf_attr_enum_table_t rdpa_epon_holdover_flags_enum_table = 
{
    .type_name = "rdpa_epon_holdover_flags", .help = "Holdover flags",
    .values = {
        {"noflags", rdpa_epon_holdover_noflags},
        {"rerange", rdpa_epon_holdover_rerange},
        {NULL, 0}
    }
};

static struct bdmf_object *epon_object;

int rdpa_epon_get(bdmf_object_handle *epon_obj)
{
    if (epon_object)
    {
        bdmf_get(epon_object);
        *epon_obj = epon_object;
        return 0;
    }
    return BDMF_ERR_NOENT;
}

const bdmf_attr_enum_table_t rdpa_epon_rate_enum_table =
{
    .type_name = "rdpa_epon_rate", .help = "EPON downstream/upstream rate",
    .values = {
        {"01/01", rdpa_epon_rate_1g1g},
        {"2.5/01", rdpa_epon_rate_2g1g},
        {"10/01", rdpa_epon_rate_10g1g},
        {"10/10", rdpa_epon_rate_10g10g},
        {NULL, 0}
    }
};

const bdmf_attr_enum_table_t rdpa_epon_mode_enum_table =
{
    .type_name = "rdpa_epon_mode", .help = "EPON OAM working mode",
    .values = {
        {"ctc", rdpa_epon_ctc},
        {"cuc", rdpa_epon_cuc},
        {"dpoe", rdpa_epon_dpoe},
        {"bcm", rdpa_epon_bcm},
        {NULL, 0}
    }
};

void (*p_OntDirEponReset)(void);
EXPORT_SYMBOL(p_OntDirEponReset);

static int epon_attr_init_write(struct bdmf_object *mo,
    struct bdmf_attr *ad, bdmf_index index, const void *val, uint32_t size)
{
    if (*(bdmf_boolean*)val)
    {
        BDMF_TRACE_INFO("init epon stack");
        p_OntDirEponReset();
    }
    return BDMF_ERR_OK;
}


static int epon_attr_epon_rate_read(struct bdmf_object *mo,
    struct bdmf_attr *ad, bdmf_index index, void *val, uint32_t size)
{
    epon_drv_priv_t *epon_priv = (epon_drv_priv_t *)bdmf_obj_data(mo);
    rdpa_epon_rate *rate = (rdpa_epon_rate *) val;

    *rate = epon_priv->epon_rate;
    return 0;
}


static int epon_attr_epon_rate_write(struct bdmf_object *mo,
    struct bdmf_attr *ad, bdmf_index index, const void *val, uint32_t size)
{
    epon_drv_priv_t *epon_priv = (epon_drv_priv_t *)bdmf_obj_data(mo);
    rdpa_epon_rate *epon_rate = (rdpa_epon_rate *)val;

    if (*epon_rate > rdpa_epon_rate_10g10g) 
    {
        BDMF_TRACE_RET_OBJ(BDMF_ERR_PERM, mo, "epon rate %u is not supported\n"\
            , *epon_rate);
    }

    epon_priv->epon_rate = *epon_rate;
    return 0;
}


static int epon_attr_epon_mode_write(struct bdmf_object *mo,
    struct bdmf_attr *ad, bdmf_index index, const void *val, uint32_t size)
{
    epon_drv_priv_t *epon_priv = (epon_drv_priv_t *)bdmf_obj_data(mo);
    rdpa_epon_mode *epon_mode = (rdpa_epon_mode *)val;

    if (*epon_mode > rdpa_epon_bcm) 
    {
        BDMF_TRACE_RET_OBJ(BDMF_ERR_PERM, mo, "epon mode %u is not supported\n"\
            , *epon_mode);
    }

    epon_priv->epon_mode = *epon_mode;
    return 0;
}

int (*p_OntDirNewLinkNumSet) (U8 links);
EXPORT_SYMBOL(p_OntDirNewLinkNumSet);

static int epon_attr_max_link_count_write(struct bdmf_object *mo,
    struct bdmf_attr *ad, bdmf_index index, const void *val, uint32_t size)
{
    epon_drv_priv_t *epon_priv = (epon_drv_priv_t *)bdmf_obj_data(mo);
    uint8_t max_link_count = *(uint8_t *)val;

    if ( p_OntDirNewLinkNumSet(max_link_count) )
        return BDMF_ERR_RANGE;

    epon_priv->max_link_count = max_link_count;
    return BDMF_ERR_OK;
}


int (*p_OntDirRegLinkNumGet) (U8 *max_links, U8 *registered_links);
EXPORT_SYMBOL(p_OntDirRegLinkNumGet);
static int epon_attr_registered_links_read(struct bdmf_object *mo,
    struct bdmf_attr *ad, bdmf_index index, void *val, uint32_t size)
{
    uint8_t max_link_count = 0; 
    int rc = BDMF_ERR_OK;

    rc =  p_OntDirRegLinkNumGet(&max_link_count, val);
    if(rc)
        rc = BDMF_ERR_INTERNAL;

    BDMF_TRACE_RET(rc,"epon stack returned registered links: %u\n",
        *(unsigned char*)val);
}

void (*p_OntDirLaserTxModeSet) (rdpa_epon_laser_tx_mode mode);
EXPORT_SYMBOL(p_OntDirLaserTxModeSet);

static int epon_attr_laser_tx_mode_write(struct bdmf_object *mo,
    struct bdmf_attr *ad, bdmf_index index, const void *val, uint32_t size)
{
    rdpa_epon_laser_tx_mode mode = *(rdpa_epon_laser_tx_mode*)val;

    p_OntDirLaserTxModeSet(mode);

    return BDMF_ERR_OK;
}


void (*p_OntDirLaserTxModeGet) (rdpa_epon_laser_tx_mode *mode);
EXPORT_SYMBOL(p_OntDirLaserTxModeGet);

static int epon_attr_laser_tx_mode_read(struct bdmf_object *mo,
    struct bdmf_attr *ad, bdmf_index index, void *val, uint32_t size)
{ 
    p_OntDirLaserTxModeGet((rdpa_epon_laser_tx_mode*)val);
    return BDMF_ERR_OK;
}

void (*p_OntDirLaserRxPowerSet) (bdmf_boolean on);
EXPORT_SYMBOL(p_OntDirLaserRxPowerSet);

static int epon_attr_laser_rx_enable_write(struct bdmf_object *mo,
    struct bdmf_attr *ad, bdmf_index index, const void *val, uint32_t size)
{
    bdmf_boolean laser_rx_enable = *(bdmf_boolean*)val;

    p_OntDirLaserRxPowerSet(laser_rx_enable); 

    return BDMF_ERR_OK;
}


void (*p_OntDirLaserRxPowerGet) (bdmf_boolean *enable);
EXPORT_SYMBOL(p_OntDirLaserRxPowerGet);

static int epon_attr_laser_rx_enable_read(struct bdmf_object *mo,
    struct bdmf_attr *ad, bdmf_index index, void *val, uint32_t size)
{ 
    p_OntDirLaserRxPowerGet((bdmf_boolean*)val);
    return BDMF_ERR_OK;
}

int (*p_PonMgrActToWanState)(LinkIndex link, BOOL enable);
EXPORT_SYMBOL(p_PonMgrActToWanState);

static int epon_attr_link_enable_write(struct bdmf_object *mo,
    struct bdmf_attr *ad, bdmf_index index, const void *val, uint32_t size)
{
    int rc = BDMF_ERR_OK;

    rc =  p_PonMgrActToWanState(index,*(bdmf_boolean*)val);
    if(rc)
        rc = BDMF_ERR_INTERNAL;
    
    BDMF_TRACE_RET(rc,
        "changing link %ld, to state %u\n", index, *(bdmf_boolean*)val);
}


void (*p_PonMgrUserTrafficGet)(LinkIndex link, BOOL *enable);
EXPORT_SYMBOL(p_PonMgrUserTrafficGet);

static int epon_attr_link_enable_read(struct bdmf_object *mo,
    struct bdmf_attr *ad, bdmf_index index, void *val, uint32_t size)
{
    p_PonMgrUserTrafficGet(index, (BOOL*)val);
    return BDMF_ERR_OK;
}

void (*p_OntDirMpcpStateGet) (LinkIndex link, rdpa_epon_link_mpcp_state
    *state);
EXPORT_SYMBOL(p_OntDirMpcpStateGet);

static int epon_attr_link_mpcp_state_read(struct bdmf_object *mo,
    struct bdmf_attr *ad, bdmf_index index, void *val, uint32_t size)
{
    p_OntDirMpcpStateGet(index, (rdpa_epon_link_mpcp_state*)val);
    return BDMF_ERR_OK;
}

void (*p_OntDirHoldoverGet)(rdpa_epon_holdover_t *);
EXPORT_SYMBOL(p_OntDirHoldoverGet);

static int epon_attr_holdover_read(struct bdmf_object *mo,
    struct bdmf_attr *ad, bdmf_index index, void *val, uint32_t size)
{
    p_OntDirHoldoverGet((rdpa_epon_holdover_t*)val);

    return BDMF_ERR_OK;
}

void (*p_OntDirHoldoverSet)(rdpa_epon_holdover_t *);
EXPORT_SYMBOL(p_OntDirHoldoverSet);

static int epon_attr_holdover_write(struct bdmf_object *mo,
    struct bdmf_attr *ad, bdmf_index index, const void *val, uint32_t size)
{
    p_OntDirHoldoverSet((rdpa_epon_holdover_t*) val);

    return BDMF_ERR_OK;
}

U16 (*p_PonLosCheckTimeGet)(void);
EXPORT_SYMBOL(p_PonLosCheckTimeGet);

U16 (*p_GateLosCheckTimeGet)(void);
EXPORT_SYMBOL(p_GateLosCheckTimeGet);

static int epon_attr_los_read(struct bdmf_object *mo,
    struct bdmf_attr *ad, bdmf_index index, void *val, uint32_t size)
{
    rdpa_epon_los_t *los = (rdpa_epon_los_t*)val;

    los->pon = p_PonLosCheckTimeGet();
    los->gate = p_GateLosCheckTimeGet();

    return BDMF_ERR_OK;
}

void (*p_PonLosCheckTimeSet)(uint16_t time);
EXPORT_SYMBOL(p_PonLosCheckTimeSet);

void (*p_GateLosCheckTimeSet)(uint16_t time);
EXPORT_SYMBOL(p_GateLosCheckTimeSet);

static int epon_attr_los_write(struct bdmf_object *mo,
    struct bdmf_attr *ad, bdmf_index index, const void *val, uint32_t size)
{
    rdpa_epon_los_t *los = (rdpa_epon_los_t*)val;
 
    p_PonLosCheckTimeSet(los->pon);
    p_GateLosCheckTimeSet(los->gate);

    return BDMF_ERR_OK;
}


BOOL (*p_OntmMpcpAssignMcast) (U8 flag, LinkIndex unicast_link_id, 
	rdpa_epon_mcast_link_t *mcast_link);
EXPORT_SYMBOL(p_OntmMpcpAssignMcast);

static int epon_attr_mcast_link_read(struct bdmf_object *mo,
    struct bdmf_attr *ad, bdmf_index index, void *val, uint32_t size)
{
    epon_drv_priv_t *epon_priv = (epon_drv_priv_t *)bdmf_obj_data(mo);
    rdpa_epon_mcast_link_t *mcast_link = (rdpa_epon_mcast_link_t *) val;

    memcpy(mcast_link, &(epon_priv->mcast_link[index]), 
		sizeof(rdpa_epon_mcast_link_t));
    p_OntmMpcpAssignMcast(0, index, mcast_link);
    memcpy(&(epon_priv->mcast_link[index]), mcast_link, 
		sizeof(rdpa_epon_mcast_link_t));

    return 0;
}

#define EPON_DEFAULT_MCAST_LINK_MASK	0x7FFF
static int epon_attr_mcast_link_write(struct bdmf_object *mo,
    struct bdmf_attr *ad, bdmf_index index, const void *val, uint32_t size)
{
    epon_drv_priv_t *epon_priv = (epon_drv_priv_t *)bdmf_obj_data(mo);
    rdpa_epon_mcast_link_t *mcast_link = (rdpa_epon_mcast_link_t *)val;
    
    BDMF_TRACE_INFO("index %d, llid %d, enable %d\n",(int)index, 
        mcast_link->llid, mcast_link->enable);
    
    if(index < RDPA_EPON_MAX_LLID)
    {
        p_OntmMpcpAssignMcast(1, index, mcast_link);
        epon_priv->mcast_link[index].llid = mcast_link->llid;
        epon_priv->mcast_link[index].enable = mcast_link->enable;
    }else
    {
        p_OntmMpcpAssignMcast(1, RDPA_EPON_MAX_LLID, mcast_link);
    }

    return 0;
}

struct bdmf_aggr_type rdpa_epon_mcast_link_type = {
    .name = "epon_mcast_link", .struct_name = "rdpa_epon_mcast_link_t",
    .help = "Epon multicast link properties",
    .fields = (struct bdmf_attr[]) {
        { .name = "llid", .help = "LLID. for example 0xffff",
            .type = bdmf_attr_number, .size = sizeof(uint32_t),
            .flags = BDMF_ATTR_MANDATORY | BDMF_ATTR_HEX_FORMAT,
            .offset = offsetof(rdpa_epon_mcast_link_t, llid),
            .min_val=0, .max_val=0xffff,
        },
        { .name = "ds_flow_index", .help = "Physical flow index mapping to Multi-link",
            .type = bdmf_attr_number, .size = sizeof(uint32_t),
            .offset = offsetof(rdpa_epon_mcast_link_t, flow_idx)
        },
        { .name = "enable", .help = "when enabled, EPON MAC is configured with\
            these parameters",
            .type = bdmf_attr_boolean, .size = sizeof(bdmf_boolean),
            .offset = offsetof(rdpa_epon_mcast_link_t, enable)
        },
        BDMF_ATTR_LAST
    }
};    
//DECLARE_BDMF_AGGREGATE_TYPE(rdpa_epon_mcast_link_type);
DECLARE_BDMF_AGGREGATE_TYPE_EXT(rdpa_epon_mcast_link_type);


struct bdmf_aggr_type rdpa_epon_holdover_type = {
    .name = "epon_holdover", .struct_name = "rdpa_epon_holdover_t",
    .help = "Epon holdover parameters",
    .fields = (struct bdmf_attr[]) {
        { .name = "time", .help = "Holdover time",
            .type = bdmf_attr_number, .size = sizeof(uint16_t),
            .flags = BDMF_ATTR_MANDATORY,
            .offset = offsetof(rdpa_epon_holdover_t, time),
            .min_val=0, .max_val=0xffff,
        },
        { .name = "flags", .help = "holdover flags",
            .type = bdmf_attr_enum,
            .flags = BDMF_ATTR_MANDATORY,
            .ts.enum_table = &rdpa_epon_holdover_flags_enum_table,
            .size = sizeof(rdpa_epon_holdover_flags),
            .offset = offsetof(rdpa_epon_holdover_t, flags)
        },
        BDMF_ATTR_LAST
    }
};    
//DECLARE_BDMF_AGGREGATE_TYPE(rdpa_epon_holdover_type);
DECLARE_BDMF_AGGREGATE_TYPE_EXT(rdpa_epon_holdover_type);


struct bdmf_aggr_type rdpa_epon_los_type = {
    .name = "epon_los", .struct_name = "rdpa_epon_los_t",
    .help = "Epon LOS threshold parameters",
    .fields = (struct bdmf_attr[]) {
        { .name = "pon", .help = "pon LOS time [milliseconds]",
            .type = bdmf_attr_number,
            .size = sizeof(uint16_t),
            .flags = BDMF_ATTR_MANDATORY,
            .offset = offsetof(rdpa_epon_los_t, pon),
            .min_val=0, .max_val=0xffff,
        },
        { .name = "gate", .help = "gate LOS time [milliseconds]",
            .type = bdmf_attr_number,
            .size = sizeof(uint16_t),
            .flags = BDMF_ATTR_MANDATORY,
            .offset = offsetof(rdpa_epon_los_t, gate),
            .min_val=0, .max_val=0xffff,
        },
        BDMF_ATTR_LAST
    }
};    
//DECLARE_BDMF_AGGREGATE_TYPE(rdpa_epon_los_type);
DECLARE_BDMF_AGGREGATE_TYPE_EXT(rdpa_epon_los_type);

BOOL (*p_OntDirFecModeGet) (LinkIndex link, BOOL *rx, BOOL *tx);
EXPORT_SYMBOL(p_OntDirFecModeGet);

static int epon_attr_fec_enable_read(struct bdmf_object *mo,
    struct bdmf_attr *ad, bdmf_index index, void *val, uint32_t size)
{
    rdpa_epon_fec_enable_t *fec_enable = (rdpa_epon_fec_enable_t *) val;
    BOOL rx, tx;
    int rc;

    rc = p_OntDirFecModeGet(index, &rx, &tx);
    
    if(!rc)
	BDMF_TRACE_RET_OBJ(BDMF_ERR_NOENT, mo, "epon driver fec state read failed\n");
    
    fec_enable->ds = rx;
    fec_enable->us = tx;
    
    return 0;
}

BOOL (*p_OntDirFecModeSet) (LinkIndex link, BOOL rx, BOOL tx);
EXPORT_SYMBOL(p_OntDirFecModeSet);

static int epon_attr_fec_enable_write(struct bdmf_object *mo,
    struct bdmf_attr *ad, bdmf_index index, const void *val, uint32_t size)
{
    epon_drv_priv_t *epon_priv = (epon_drv_priv_t *)bdmf_obj_data(mo);
    rdpa_epon_fec_enable_t *fec_enable = (rdpa_epon_fec_enable_t *) val;

    memcpy(&(epon_priv->fec_enable[index]), fec_enable, 
        sizeof(rdpa_epon_fec_enable_t));
    BDMF_TRACE_INFO("ds %u, us %u\n", fec_enable->ds, fec_enable->us);
    p_OntDirFecModeSet(index,fec_enable->ds,fec_enable->us);

    return 0;
}

static int burst_cap_struct_to_buf(const rdpa_epon_burst_cap_per_priority_t *bcap_struct,
    uint16_t *bcap_buf, uint8_t *count)
{
    if ( !bcap_struct || !bcap_buf)
        return -1;

    bcap_buf[0] = bcap_struct->priority_0;
    *count = 1;

    if (bcap_struct->priority_1 != 0)
    {
        *count = 8;
        bcap_buf[1] = bcap_struct->priority_1;
        bcap_buf[2] = bcap_struct->priority_2;
        bcap_buf[3] = bcap_struct->priority_3;
        bcap_buf[4] = bcap_struct->priority_4;
        bcap_buf[5] = bcap_struct->priority_5;
        bcap_buf[6] = bcap_struct->priority_6;
        bcap_buf[7] = bcap_struct->priority_7;
    }

    return 0;
}

static int buf_to_burst_cap_struct(rdpa_epon_burst_cap_per_priority_t *bcap_struct,
    const uint16_t *bcap_buf, uint8_t *count)
{
    if ( !bcap_struct || !bcap_buf)
        return -1;

    bcap_struct->priority_0 = bcap_buf[0]; 
    *count = 1;

    if (bcap_buf[1] != 0)
    {
        *count = 8;
        bcap_struct->priority_1= bcap_buf[1] ;
        bcap_struct->priority_2= bcap_buf[2] ;
        bcap_struct->priority_3= bcap_buf[3] ;
        bcap_struct->priority_4= bcap_buf[4] ;
        bcap_struct->priority_5= bcap_buf[5] ;
        bcap_struct->priority_6= bcap_buf[6] ;
        bcap_struct->priority_7= bcap_buf[7] ;
    }

    return 0;
}

void (*p_OntDirBurstCapSet)(LinkIndex link, const U16 * bcap);
EXPORT_SYMBOL(p_OntDirBurstCapSet);

static int epon_attr_burst_cap_write(struct bdmf_object *mo,
    struct bdmf_attr *ad, bdmf_index index, const void *val, uint32_t size)
{
   uint16_t buffer[8] = {0};
   uint8_t count = 0;

   burst_cap_struct_to_buf(val, buffer, &count);
   p_OntDirBurstCapSet(index, buffer);

   return BDMF_ERR_OK;
}

void (*p_OntDirBurstCapGet)(LinkIndex link,  U16 * bcap);
EXPORT_SYMBOL(p_OntDirBurstCapGet);

static int epon_attr_burst_cap_read(struct bdmf_object *mo,
    struct bdmf_attr *ad, bdmf_index index, void *val, uint32_t size)
{
    uint16_t buffer[8];
    uint8_t count = 0;

    p_OntDirBurstCapGet(index, buffer);
    buf_to_burst_cap_struct(val, buffer, &count);

    return BDMF_ERR_OK;
}

struct bdmf_aggr_type rdpa_epon_fec_enable_type = {
    .name = "epon_fec_enable", .struct_name = "rdpa_epon_fec_enable_t",
    .help = "enable EPON MAC FEC for us/ds or both", 
    .fields = (struct bdmf_attr[]) {
	{ .name = "ds", .help = "enable downstream traffic EPON MAC FEC",
	    .type = bdmf_attr_boolean, .size = sizeof(bdmf_boolean),
	    .offset = offsetof(rdpa_epon_fec_enable_t, ds)
	},
	{ .name = "us", .help = "enable upstream traffic EPON MAC FEC",
	    .type = bdmf_attr_boolean, .size = sizeof(bdmf_boolean),
	    .offset = offsetof(rdpa_epon_fec_enable_t, us)
	},
	BDMF_ATTR_LAST
    }
};
//DECLARE_BDMF_AGGREGATE_TYPE(rdpa_epon_fec_enable_type);
DECLARE_BDMF_AGGREGATE_TYPE_EXT(rdpa_epon_fec_enable_type);


struct bdmf_aggr_type rdpa_epon_burst_cap_per_priority_type = {
    .name = "burst_cap_per_priority", 
    .struct_name = "rdpa_epon_burst_cap_per_priority_t",
    .help = "each tx priority level has it's own burst cap value", 
    .fields = (struct bdmf_attr[]) {
	{ 
        .name = "prio_0", 
	    .type = bdmf_attr_number, 
        .size = sizeof(uint16_t),
	    .offset = offsetof(rdpa_epon_burst_cap_per_priority_t, priority_0),
	},
	{ 
        .name = "prio_1", 
	    .type = bdmf_attr_number, 
        .size = sizeof(uint16_t),
	    .offset = offsetof(rdpa_epon_burst_cap_per_priority_t, priority_1),
	},
	{ 
        .name = "prio_2", 
	    .type = bdmf_attr_number, 
        .size = sizeof(uint16_t),
	    .offset = offsetof(rdpa_epon_burst_cap_per_priority_t, priority_2),
	},
	{ 
        .name = "prio_3", 
	    .type = bdmf_attr_number, 
        .size = sizeof(uint16_t),
	    .offset = offsetof(rdpa_epon_burst_cap_per_priority_t, priority_3),
	},
	{ 
        .name = "prio_4", 
	    .type = bdmf_attr_number, 
        .size = sizeof(uint16_t),
	    .offset = offsetof(rdpa_epon_burst_cap_per_priority_t, priority_4),
	},
	{ 
        .name = "prio_5", 
	    .type = bdmf_attr_number, 
        .size = sizeof(uint16_t),
	    .offset = offsetof(rdpa_epon_burst_cap_per_priority_t, priority_5),
	},
    {
        .name = "prio_6", 
	    .type = bdmf_attr_number, 
        .size = sizeof(uint16_t),
	    .offset = offsetof(rdpa_epon_burst_cap_per_priority_t, priority_6),
	},
    {
        .name = "prio_7", 
	    .type = bdmf_attr_number, 
        .size = sizeof(uint16_t),
	    .offset = offsetof(rdpa_epon_burst_cap_per_priority_t, priority_7),
	},
	BDMF_ATTR_LAST
    }
};
//DECLARE_BDMF_AGGREGATE_TYPE(rdpa_epon_burst_cap_per_priority_type);
DECLARE_BDMF_AGGREGATE_TYPE_EXT(rdpa_epon_burst_cap_per_priority_type);

static struct bdmf_attr epon_attrs[] =
{
    { .name = "rate", .help = "downstream/upstream rate",
        .flags = BDMF_ATTR_READ | BDMF_ATTR_WRITE | BDMF_ATTR_CONFIG,
        .type = bdmf_attr_enum, .ts.enum_table = &rdpa_epon_rate_enum_table,
        .size = sizeof(rdpa_epon_mode), 
        .offset = offsetof(epon_drv_priv_t, epon_rate),
        .read = epon_attr_epon_rate_read,
        .write = epon_attr_epon_rate_write    
    },
    { .name = "mode", .help = "epon oam: ctc/dpoe/bcm",
        .flags = BDMF_ATTR_READ | BDMF_ATTR_WRITE | BDMF_ATTR_CONFIG,
        .type = bdmf_attr_enum, .ts.enum_table = &rdpa_epon_mode_enum_table,
        .size = sizeof(rdpa_epon_mode), 
        .offset = offsetof(epon_drv_priv_t, epon_mode),
        .write = epon_attr_epon_mode_write    
    },
    { .name = "init", .help = "one time EPON stack init",
        .flags = BDMF_ATTR_WRITE | BDMF_ATTR_CONFIG,
        .type = bdmf_attr_boolean,
        .write = epon_attr_init_write
    },
    { .name = "max_link_count", .help = "how many links can be used",
        .flags = BDMF_ATTR_READ | BDMF_ATTR_WRITE | BDMF_ATTR_CONFIG,
        .type = bdmf_attr_number,
        .size = sizeof(uint8_t), .offset = offsetof(epon_drv_priv_t,
            max_link_count),
        .write = epon_attr_max_link_count_write    
    },
    { .name = "registered_links", .help = "how many links are registered",
        .flags = BDMF_ATTR_READ | BDMF_ATTR_STAT,
        .type = bdmf_attr_number,
        .size = sizeof(uint8_t), .offset = offsetof(epon_drv_priv_t,
            registered_links),
        .read = epon_attr_registered_links_read
    },
    { .name = "laser_tx_mode", .help = "Set the laser TX mode",
        .flags = BDMF_ATTR_READ | BDMF_ATTR_WRITE | BDMF_ATTR_CONFIG,
        .type = bdmf_attr_enum, 
        .ts.enum_table = &rdpa_epon_laser_tx_mode_enum_table,
        .size = sizeof(rdpa_epon_laser_tx_mode), 
        .write = epon_attr_laser_tx_mode_write,    
        .read = epon_attr_laser_tx_mode_read    
    },
    { .name = "laser_rx_enable", .help = "Set the Rx laser enable",
        .flags = BDMF_ATTR_READ | BDMF_ATTR_WRITE | BDMF_ATTR_CONFIG,
        .type = bdmf_attr_boolean, .size = sizeof(bdmf_boolean),
        .write = epon_attr_laser_rx_enable_write,
        .read = epon_attr_laser_rx_enable_read
    },
    { .name = "holdover", .help = "EPON holdover properties",
        .type = bdmf_attr_aggregate, .ts.aggr_type_name = "epon_holdover",
        .flags = BDMF_ATTR_READ | BDMF_ATTR_WRITE | BDMF_ATTR_CONFIG,
        .read = epon_attr_holdover_read,
        .write = epon_attr_holdover_write
    },
    { .name = "los_threshold", .help = "PON and Gate LOS time threshold\
        before any action is taken. Time unit is [milliseconds]",
        .type = bdmf_attr_aggregate, .ts.aggr_type_name = "epon_los",
        .flags = BDMF_ATTR_READ | BDMF_ATTR_WRITE | BDMF_ATTR_CONFIG,
        .read = epon_attr_los_read,
        .write = epon_attr_los_write
    },
    { .name = "link_enable", .help = "Enable epon link",
        .flags = BDMF_ATTR_READ | BDMF_ATTR_WRITE | BDMF_ATTR_CONFIG,
        .type = bdmf_attr_boolean, 
        .read = epon_attr_link_enable_read,
        .write = epon_attr_link_enable_write,
        .array_size = RDPA_EPON_MAX_LLID
    },
    { .name = "link_mpcp_state", .help = "Get the link MPCP regitration state",
        .flags = BDMF_ATTR_READ | BDMF_ATTR_CONFIG,
        .type = bdmf_attr_enum, 
        .ts.enum_table = &rdpa_epon_link_mpcp_state_enum_table,
        .read = epon_attr_link_mpcp_state_read,
        .array_size = RDPA_EPON_MAX_LLID
    },
    { .name = "mcast_link", .help = "EPON multicast link properties",
        .type = bdmf_attr_aggregate, .ts.aggr_type_name = "epon_mcast_link",
        .flags = BDMF_ATTR_READ | BDMF_ATTR_WRITE | BDMF_ATTR_CONFIG,
        .offset = offsetof(epon_drv_priv_t, mcast_link),
        .write = epon_attr_mcast_link_write,
        .read = epon_attr_mcast_link_read,
        .array_size = RDPA_EPON_MAX_LLID   
    },
    {
	.name = "fec_enable", .help = "enable EPON MAC FEC for us/ds or both",
	.type = bdmf_attr_aggregate, .ts.aggr_type_name = "epon_fec_enable",
    .flags = BDMF_ATTR_READ | BDMF_ATTR_WRITE | BDMF_ATTR_CONFIG,
	.write = epon_attr_fec_enable_write,
	.read = epon_attr_fec_enable_read,
	.array_size = RDPA_EPON_MAX_LLID
    },
    {
	.name = "burst_cap_map", .help = "all links burst cap per priority level\
             the index is the link number",
	.type = bdmf_attr_aggregate,
    .ts.aggr_type_name = "burst_cap_per_priority", 
    .size = sizeof(rdpa_epon_burst_cap_per_priority_t),
    .flags = BDMF_ATTR_READ | BDMF_ATTR_WRITE | BDMF_ATTR_CONFIG,
	.write = epon_attr_burst_cap_write,
	.read = epon_attr_burst_cap_read,
	.array_size = RDPA_EPON_MAX_LLID
    },
    BDMF_ATTR_LAST
};

static int epon_drv_pre_init (struct bdmf_object *mo); 
static int epon_drv_init (struct bdmf_type *drv); 
static int epon_drv_post_init (struct bdmf_object *mo);
static void epon_drv_exit (struct bdmf_type *drv);
static void epon_drv_exit (struct bdmf_type *drv);
static void epon_destroy (struct bdmf_object *mo);

struct bdmf_type epon_drv = {
    .name = "epon",
    .parent = "system",
    .description = "EPON MAC object",
    .drv_init = epon_drv_init,
    .drv_exit = epon_drv_exit,
    .pre_init = epon_drv_pre_init,
    .post_init = epon_drv_post_init,
    .destroy = epon_destroy,
    .extra_size = sizeof(epon_drv_priv_t),
    .aattr = epon_attrs,
    .max_objs = 1,
};
//DECLARE_BDMF_TYPE(rdpa_epon, epon_drv);
DECLARE_BDMF_TYPE_EXT(rdpa_epon, epon_drv);

static int epon_drv_pre_init (struct bdmf_object *mo) 
{
    epon_drv_priv_t *epon = (epon_drv_priv_t *)bdmf_obj_data(mo);
    epon->epon_mode = rdpa_epon_ctc;
    epon_object = mo;
    return 0;
}

void rdpa_epon_get_mode(rdpa_epon_mode* const mode)
{
	epon_drv_priv_t *epon_priv;

	*mode = rdpa_epon_none;

	if (epon_object)
	{
		epon_priv = (epon_drv_priv_t*)bdmf_obj_data(epon_object);
		*mode = epon_priv->epon_mode;
	}
}

static int epon_drv_init (struct bdmf_type *drv) 
{
#ifdef BDMF_DRIVER_GPL_LAYER
    f_rdpa_epon_drv = rdpa_epon_drv;
    f_rdpa_epon_get = rdpa_epon_get;
    f_epon_get_mode = rdpa_epon_get_mode;
#endif

    return 0;
}

static int epon_drv_post_init (struct bdmf_object *mo)
{
    snprintf(mo->name, sizeof(mo->name), "epon");
    return 0;
}

static void epon_drv_exit (struct bdmf_type *drv)
{
#ifdef BDMF_DRIVER_GPL_LAYER
    f_rdpa_epon_drv = NULL;
    f_rdpa_epon_get = NULL;
    f_epon_get_mode = NULL;
#endif
    return;
}

static void epon_destroy (struct bdmf_object *mo)
{
    return;
}


static struct bdmf_aggr_type *epon_aggregates[] =
{
    &rdpa_epon_mcast_link_type,
	&rdpa_epon_holdover_type,
	&rdpa_epon_los_type,
	&rdpa_epon_fec_enable_type,
	&rdpa_epon_burst_cap_per_priority_type
};

int rdpa_epon_declare_and_register_to_bdmf(void)
{
    int rc, i;

    /* Initialize BDMF aggregates */
    for (i = 0; i < sizeof(epon_aggregates) / sizeof(epon_aggregates[0]); i++)
    {
        if (!epon_aggregates[i])
            continue;

        rc = bdmf_attr_aggregate_type_register(epon_aggregates[i]);
        if (rc < 0)
        {
            BDMF_TRACE_RET(BDMF_ERR_INTERNAL,
                "EPON Aggregates registration to BDMF failed: rc=%d", rc);
        }
    }

    rc = bdmf_type_register(&epon_drv);
    if (rc < 0)
        BDMF_TRACE_RET(BDMF_ERR_INTERNAL, "EPON object registration to BDMF failed: rc=%d", rc);

    return 0;
}

void rdpa_epon_bdmf_unregister(void)
{
    int i;

    for (i = 0; i < sizeof(epon_aggregates) / sizeof(epon_aggregates[0]); i++)
    {
        if (!epon_aggregates[i])
            continue;

        bdmf_attr_aggregate_type_unregister(epon_aggregates[i]);
    }

    bdmf_type_unregister(&epon_drv);
}
