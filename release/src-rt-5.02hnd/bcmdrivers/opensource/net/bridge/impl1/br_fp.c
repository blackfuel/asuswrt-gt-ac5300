/*
* <:copyright-BRCM:2012:DUAL/GPL:standard
* 
*    Copyright (c) 2012 Broadcom 
*    All Rights Reserved
* 
* Unless you and Broadcom execute a separate written software license
* agreement governing use of this software, this software is licensed
* to you under the terms of the GNU General Public License version 2
* (the "GPL"), available at http://www.broadcom.com/licenses/GPLv2.php,
* with the following added to such license:
* 
*    As a special exception, the copyright holders of this software give
*    you permission to link this software with independent modules, and
*    to copy and distribute the resulting executable under terms of your
*    choice, provided that you also meet, for each linked independent
*    module, the terms and conditions of the license of that module.
*    An independent module is a module which is not derived from this
*    software.  The special exception does not apply to any modifications
*    of the software.
* 
* Not withstanding the above, under no circumstances may you combine
* this software in any way with any other Broadcom software provided
* under a license other than the GPL, without Broadcom's express prior
* written consent.
* 
* :> 
*/

#include <linux/version.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/proc_fs.h>
#include <linux/device.h>
#include <linux/spinlock.h>
#include <linux/if_vlan.h>
#include <linux/if_bridge.h>
#include <asm/cpu.h>
#include <asm/uaccess.h>
#include <asm/string.h>
#include "bcm_OS_Deps.h"
#include "br_fp.h"
#include "br_private.h"
#include "rdpa_api.h"
#include "rdpa_mw_blog_parse.h"
#include "bcmenet_common.h"

static char init_br_name[IFNAMSIZ] = "br0";
static bdmf_object_handle init_br_obj = NULL;
module_param_string(init_br_name, init_br_name, IFNAMSIZ, 0);

static bdmf_fastlock bridge_lock;
static bdmf_mac_t zero_mac;

static int is_wlan_accl_enabled(void)
{
    bdmf_object_handle cpu_obj;
                
    if (!rdpa_cpu_get(rdpa_cpu_wlan0, &cpu_obj))
    {
        bdmf_put(cpu_obj);
        return 1;
    }
    return 0;
}

static rdpa_if dev2bridge_port_bitwise(struct net_device *dev)
{
    unsigned int physical_hw_port, hw_port_type;
    unsigned int hw_port = netdev_path_get_hw_port(dev);
    /* In case of external switch netdev_path_get_hw_port return logical port and not HW port. 
       It is assumed that hw port cvalues are 0-7 and logical port values are 8-15*/
    physical_hw_port = LOGICAL_PORT_TO_PHYSICAL_PORT(hw_port);   
    hw_port_type = netdev_path_get_hw_port_type(dev);

    switch (hw_port_type)
    {
    case BLOG_ENETPHY:
        return rdpa_port_map_from_hw_port(physical_hw_port, 1);
    case BLOG_GPONPHY:
    case BLOG_EPONPHY:        
	return rdpa_if_wan0;
    case BLOG_WLANPHY:
	return rdpa_if_ssid0 + hw_port;
    default:
        BDMF_TRACE_ERR("hw port type %d (port %d) is not supprorted\n",
	    hw_port_type, physical_hw_port);
        return rdpa_if_none;
    }
}

static void br_fp_rdpa_fdb_key_set(struct net_bridge_fdb_entry *fdb,
    rdpa_fdb_key_t *key, bdmf_object_handle br_obj)
{
#if defined(CONFIG_BCM_KF_VLAN_AGGREGATION) && defined(CONFIG_BCM_VLAN_AGGREGATION)
    rdpa_bridge_cfg_t br_cfg;
#endif

    memset(key, 0, sizeof(*key));
    memcpy(&key->mac, fdb->addr.addr, sizeof(key->mac));
#if defined(CONFIG_BCM_KF_VLAN_AGGREGATION) && defined(CONFIG_BCM_VLAN_AGGREGATION)
    rdpa_bridge_config_get(br_obj, &br_cfg);
    if (br_cfg.type == rdpa_bridge_802_1q)
        key->vid = GET_FDB_VLAN(fdb);
#endif
}

static bdmf_error_t br_fp_add_or_modify_mac_tbl_entry(struct net_bridge_fdb_entry *fdb, int is_add)
{
    bdmf_object_handle bridge;
    rdpa_fdb_key_t key;
    rdpa_fdb_data_t data;
    rdpa_if port;
    struct net_device *root_dev;
    bdmf_error_t rc;

    root_dev = netdev_path_get_root(fdb->dst->dev);
    if (!root_dev)
        BDMF_TRACE_RET(BDMF_ERR_NODEV, "Cannot find the physical device of %s", fdb->dst->dev->name);

    port = dev2bridge_port_bitwise(root_dev);
    if (port == rdpa_if_none)
    {
       	BDMF_TRACE_RET(BDMF_ERR_PARM, "%s MAC Table: invalid entry. src dev name[%s] mac[%pM]", is_add ? "ADD" : "MOD",
            root_dev->name, fdb->addr.addr);
    }

    bridge = (bdmf_object_handle)fdb->dst->br->fp_hooks.rdpa_priv;
    if (!bridge)
        return BDMF_ERR_NOENT;

    br_fp_rdpa_fdb_key_set(fdb, &key, bridge);

    bdmf_fastlock_lock(&bridge_lock);

    rc = rdpa_bridge_mac_get(bridge, &key, &data);
    if (is_add && !rc)
    {
        /*when using ingress classifier we have a usecase which there is an extra entry in RDPA which will not be deleted
         *thus we have to add it anyway with the real bridge port*/
        if (!(data.ports & rdpa_if_id(rdpa_if_cpu)))
        {
            rc = BDMF_ERR_ALREADY; /* The MAC entry already exists for this bridge. */
            goto exit;
        }
    }

    data.ports = rdpa_if_id(port);
    data.sa_action = rdpa_forward_action_forward;
    if (rdpa_if_is_wifi(port) && !is_wlan_accl_enabled())
        data.da_action = rdpa_forward_action_host;
    else
        data.da_action = rdpa_forward_action_forward;

    rc = rdpa_bridge_mac_set(bridge, &key, &data);

#if defined(CONFIG_BCM_KF_BRIDGE_COUNTERS)
    if ((rc == BDMF_ERR_NO_MORE) && !(fdb->is_static))
    {
        fdb->dst->br->mac_entry_discard_counter++;
    }
#endif

exit:
    bdmf_fastlock_unlock(&bridge_lock);
    return rc;
}

static int exists_in_other_bridge(struct net_device *dev_orig, unsigned char *addr)
{
    int i;
    struct hlist_node *h;
    struct net_bridge_fdb_entry *f;
    struct net_bridge *br;
    struct net_device *dev;

    for_each_netdev(&init_net, dev)
    {
        if (!(dev->priv_flags & IFF_EBRIDGE) || dev == dev_orig)
            continue;

        br = netdev_priv(dev);

        for (i = 0; i < BR_HASH_SIZE; i++)
        {
            HLIST_FOR_EACH_ENTRY_RCU(f, h, &br->hash[i], hlist)
            {
                if (f->is_local) 
                    continue;

                if (is_ether_addr_same(f->addr.addr, addr))
                    return 1;
            }
        }
    }

    return 0;
}

static int br_fp_remove_mac_tbl_entry(struct net_bridge_fdb_entry *fdb, int remove_dups)
{
    struct net_bridge_port *p;
    bdmf_object_handle bridge;
    rdpa_fdb_key_t key = {};
    bdmf_error_t rc;

    p = br_port_get_rcu(fdb->dst->dev);
    if (!p)
        return -1;

    if (!remove_dups &&
        exists_in_other_bridge(p->br->dev, fdb->addr.addr))
    {
        return 0;
    }

    bridge = (bdmf_object_handle)fdb->dst->br->fp_hooks.rdpa_priv;
    if (!bridge)
        return BDMF_ERR_NOENT;

    br_fp_rdpa_fdb_key_set(fdb, &key, bridge);
    rc = rdpa_bridge_mac_set(bridge, &key, NULL);
    if (rc)
    {
        BDMF_TRACE_RET(rc, "Failed to remove mac entry, src dev name[%s] mac[%pM]", fdb->dst->dev->name,
            fdb->addr.addr);
    }

    return 0;
}

static int br_fp_ageing_mac_tbl_entry(struct net_bridge_fdb_entry *fdb, int *age_valid)
{
    bdmf_error_t rc;
    bdmf_boolean _age_valid = 0;
    bdmf_object_handle bridge;
    rdpa_fdb_key_t key = {};

    bridge = (bdmf_object_handle)fdb->dst->br->fp_hooks.rdpa_priv;
    if (!bridge)
        return BDMF_ERR_NOENT;

    br_fp_rdpa_fdb_key_set(fdb, &key, bridge);
    rc = rdpa_bridge_mac_status_get(bridge, &key, &_age_valid);
    if (rc)
    {
    	BDMF_TRACE_ERR("Aging check failed(%d). src dev name[%s] mac[%pM]", rc, fdb->dst->dev->name, fdb->addr.addr);
        _age_valid = 0; /* return status, which will cause deletion of fdb entry in kernel bridge */
    }

    *age_valid = _age_valid;
    return 0;
}

static bdmf_object_handle dev2rdpa_port(struct net_device *dev)
{
#if defined(CONFIG_BLOG)
    bdmf_object_handle port_obj = NULL;
    rdpa_if port, wifi_ssid = rdpa_if_none;

    port = rdpa_mw_root_dev2rdpa_if(dev, &wifi_ssid);
    if (port == rdpa_if_none)
    {
        BDMF_TRACE_ERR("Can't find rdpa_port for dev '%s'\n", dev->name);
        return NULL;
    }

    /* PCI port/radio should not be enslaved to bridge, only SSIDs */
    if (wifi_ssid != rdpa_if_none)
        port = wifi_ssid;
    rdpa_port_get(port, &port_obj);
    return port_obj;
#else
    /* temporaliy fix before BLOG merge is finished. but can this driver run without blog? */
    printk(KERN_ERR "dev2rdpa_port not implemented yet for non blog config!!!\n");
    return NULL;
#endif
}

static int br_fp_rdpa_bridge_add(struct net_bridge *br)
{
    static int is_br0_init = 1;
    bdmf_object_handle br_obj = NULL;

    /* Init sequence of external switch requires the bridge object to exist before we have it created in
     * Linux. So we look for the first configured bridge in RDPA, and expect it to be a single bridge in the system.
     * If won't found, new bridge object is added. */
    if (!strcmp(br->dev->name, init_br_name) && is_br0_init)
    {
        br_obj = bdmf_get_next(rdpa_bridge_drv(), NULL, NULL);
        init_br_obj = br_obj;
        is_br0_init = 0;
    }
    if (!br_obj)
    {
        int rc = bdmf_new_and_set(rdpa_bridge_drv(), NULL, NULL, &br_obj);
        if (rc < 0)
        {
            BDMF_TRACE_ERR("Failed to add bridge, name %s, error %d\n", br->dev->name, rc);
            return -1;
        }
    }
    br->fp_hooks.rdpa_priv = br_obj;

    return 0;
}

static int br_fp_rdpa_bridge_del(struct net_bridge *br)
{
    if (!br->fp_hooks.rdpa_priv)
        return 0;

    if (br->fp_hooks.rdpa_priv == init_br_obj)
        bdmf_put((bdmf_object_handle)br->fp_hooks.rdpa_priv);
    else
        bdmf_destroy((bdmf_object_handle)br->fp_hooks.rdpa_priv);
    br->fp_hooks.rdpa_priv = NULL;
    return 0;
}

static int is_port_object(bdmf_object_handle o)
{
    static char *port_type_name = "port/";
    return !memcmp(bdmf_object_name(o), port_type_name, strlen(port_type_name));
}

static int br_fp_eligible_port_fwd_tbl_entry(void)
{
    struct net_bridge_port *p;
    struct net_bridge *br;
    struct net_device *dev;

    rcu_read_lock();

    for_each_netdev(&init_net, dev)
    {
        bdmf_object_handle bridge_object;
        bdmf_link_handle bridge_link;

        if (!(dev->priv_flags & IFF_EBRIDGE))
            continue;

        br = netdev_priv(dev);
        if (!br->fp_hooks.rdpa_priv)
        {
            BDMF_TRACE_ERR("\nCan't find RDPA object for bridge %s\n", dev->name);
            continue;
        }
        bridge_object = (bdmf_object_handle)br->fp_hooks.rdpa_priv;
        /* Remove all bridge links */
        bridge_link=bdmf_get_next_ds_link(bridge_object, NULL);
        while(bridge_link)
        {
            bdmf_object_handle o = bdmf_ds_link_to_object(bridge_link);
            bridge_link=bdmf_get_next_ds_link(bridge_object, bridge_link);
            if (is_port_object(o))
                bdmf_unlink(o, bridge_object);
        }

        list_for_each_entry(p, &br->port_list, list)
        {
            bdmf_object_handle port_object = dev2rdpa_port(netdev_path_get_root(p->dev));
            struct bdmf_link *plink;
            int rc;

            if (!port_object)
                continue; /* error message is already printed */

            /* In case of multiple SSID's under a bridge (e.g: wl0 and wl0.1 under br0), the port object will be the
             * same for all SSID's (the port object for rdpa_if_wlan0 or rdpa_if_wlan1). bdmf_link() won't permit two
             * objects to be linked again, so bdmf_is_linked() should protect us from that. */
            if (!bdmf_is_linked(port_object, bridge_object, &plink))
            {
                rc = bdmf_link(port_object, bridge_object, NULL);
                if (rc)
                {
                    BDMF_TRACE_ERR("Can't link objects %s - %s, error %s (%d)",
                        bdmf_object_name(port_object), bdmf_object_name(bridge_object),
                        bdmf_strerror(rc), rc);
                }
            }

            bdmf_put(port_object);
        }
    }

    rcu_read_unlock();

    return 0; 
}

static int br_fp_rdpa_hook(int cmd, void *in, void *out)
{
    struct net_bridge_fdb_entry *fdb = (struct net_bridge_fdb_entry *)in;

    switch (cmd)
    {
    case BR_FP_FDB_ADD:
        return br_fp_add_or_modify_mac_tbl_entry(fdb, 1);
    case BR_FP_FDB_MODIFY:
        return br_fp_add_or_modify_mac_tbl_entry(fdb, 0);
    case BR_FP_FDB_REMOVE:
        return br_fp_remove_mac_tbl_entry(fdb, 0);
    case BR_FP_PORT_ADD:
    case BR_FP_PORT_REMOVE:
        return br_fp_eligible_port_fwd_tbl_entry();
    case BR_FP_FDB_CHECK_AGE:
        return br_fp_ageing_mac_tbl_entry(fdb, (int *)out);
    default: 
        break;
    }
    return -1;
}

static void do_fdb_entries(struct net_device *brdev, int add)
{
    int i;
    struct hlist_node *h;
    struct net_bridge_fdb_entry *f;
    struct net_bridge *br;

    br = netdev_priv(brdev);

    rcu_read_lock();
    for (i = 0; i < BR_HASH_SIZE; i++)
    {
        HLIST_FOR_EACH_ENTRY_RCU(f, h, &br->hash[i], hlist)
        {
            if (f->is_local)
                continue;

            if (add)
                br_fp_add_or_modify_mac_tbl_entry(f, 1);
            else
            {
                /* always remove, even if exists in more then 1 bridge */
                br_fp_remove_mac_tbl_entry(f, 1);
            }
        }
    }
    rcu_read_unlock();
}

/* Return whether a given device is considered WAN VLAN interface. */
static int is_wan_vlan(struct net_device *dev)
{
    return (dev->priv_flags & IFF_BCM_VLAN) && (dev->priv_flags & IFF_WANDEV);
}

/* this function is registered to device events. we are interested in mac change of the bridge*/
static int br_fp_notifier_call(struct notifier_block *nb, unsigned long event, void *_br)
{
    struct net_device *dev = NETDEV_NOTIFIER_GET_DEV(_br);
    static int up_br_ref_cnt = 0;
    bdmf_error_t rc;
    struct net_bridge *br;

    switch (event)    
    {
    /* NETDEV_CHANGEADDR is not enough - a device may be granted a MAC address
     * on its way up without moving through NETDEV_CHANGEADDR. */
    case NETDEV_REGISTER:
    case NETDEV_UNREGISTER:
        {
            if (!(dev->priv_flags & IFF_EBRIDGE))
                break;

            br = (struct net_bridge *)netdev_priv(dev);
            if (event == NETDEV_REGISTER)
                return br_fp_rdpa_bridge_add(br);
            else
                return br_fp_rdpa_bridge_del(br);
        }
    case NETDEV_UP:
    case NETDEV_DOWN:
    case NETDEV_CHANGEADDR:
        if (dev->priv_flags & IFF_EBRIDGE)
        {
            bdmf_object_handle bridge = NULL;
            bdmf_mac_t lan_mac;

            if (event == NETDEV_UP)
            {
                up_br_ref_cnt++;
                if (up_br_ref_cnt > 1)
                {
                    /* This is not the first bridge that we add, skip LAN mac configuration */
                    break;
                }
            }
            else if (event == NETDEV_DOWN)
            {
                up_br_ref_cnt--;
                if (up_br_ref_cnt)
                {
                    /* This is not the single bridge remained, skip LAN mac configuration */
                    break;
                }
            }

            bdmf_lock();
            bridge = bdmf_get_next(rdpa_bridge_drv(), NULL, NULL);
            if (bridge)
            {
                memcpy(&lan_mac.b, event == NETDEV_DOWN ? zero_mac.b : dev->dev_addr, sizeof(lan_mac));
                rc = rdpa_bridge_lan_mac_set(bridge, &lan_mac);
                bdmf_put(bridge);
            }
            else
                rc = BDMF_ERR_NOENT;
            bdmf_unlock();

            if (rc < 0)
                BDMF_TRACE_ERR("unable to change LAN MAC address, rc %d\n", rc);
        }
        /* XXX wan_mac configuration should move to rdpa_mw when it is
         * available. See JIRA SWBCACPE-12904 */
        else if (is_wan_vlan(dev))
        {
            bdmf_object_handle ip_class = NULL;
            bdmf_mac_t wan_mac;

            bdmf_lock();
            rc = rdpa_ip_class_get(&ip_class);
            rc = rc ? rc : rdpa_ip_class_routed_mac_get(ip_class, 0, &wan_mac);
            memcpy(wan_mac.b, event == NETDEV_DOWN ? zero_mac.b : dev->dev_addr, sizeof(wan_mac));
            rc = rc ? rc : rdpa_ip_class_routed_mac_set(ip_class, 0, &wan_mac);
            if (ip_class)
                bdmf_put(ip_class);
            bdmf_unlock();

            if (rc)
                BDMF_TRACE_ERR("unable to change WAN MAC address, rc %d\n",rc);
        }
        break;
    default:
        break;
    }

    return 0;
}

static struct notifier_block nb =
{
    .notifier_call = br_fp_notifier_call,
    .priority = 0,
};

static struct br_fp_data gs_fp_hooks =
{
    .rdpa_hook = br_fp_rdpa_hook,
};

static int bl_lilac_br_fp_init(void)
{
    struct net_device *dev;

    printk("Bridge fastpath module\n");

    if (register_netdevice_notifier(&nb))
    {
        printk(KERN_ERR "register_netdevice_notifier() failed(%d)", 0);
        return -1;
    }

    for_each_netdev(&init_net, dev)
    {
        if (dev->priv_flags & IFF_EBRIDGE)
            do_fdb_entries(dev, 1);
    }

    br_fp_set_callbacks(&gs_fp_hooks);

    return 0;
}

static void bl_lilac_br_fp_cleanup(void)
{
    struct net_device *dev;

    br_fp_clear_callbacks();

    for_each_netdev(&init_net, dev)
    {
        if (dev->priv_flags & IFF_EBRIDGE)
            do_fdb_entries(dev, 0);
    }

    unregister_netdevice_notifier(&nb);
}

MODULE_LICENSE("GPL");
module_init(bl_lilac_br_fp_init);
module_exit(bl_lilac_br_fp_cleanup);
