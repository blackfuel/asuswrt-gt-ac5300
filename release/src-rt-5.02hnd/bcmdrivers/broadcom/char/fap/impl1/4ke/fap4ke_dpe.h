/***********************************************************
 * <:copyright-BRCM:2009:DUAL/GPL:standard
 * 
 *    Copyright (c) 2009 Broadcom 
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
 ************************************************************/

#ifndef __FAP4KE_DPE_H_INCLUDED__
#define __FAP4KE_DPE_H_INCLUDED__

/*
 *******************************************************************************
 * File Name  : fap4ke_dpe.h
 *
 * Description: This file contains the public API for the 4KE DPE driver
 *
 *******************************************************************************
 */

#define DPE_SUCCESS                 FAP_SUCCESS
#define DPE_ERROR                   FAP_ERROR
#define DPE_ERROR_CMD_FIFO_FULL     2
#define DPE_ERROR_STS_FIFO_2_EMPTY  3
#define DPE_ERROR_DMA_XFER_ERROR    4
      
#define DPE_CMD_DMA_OUT          0
#define DPE_CMD_DMA_IN           1
#define DPE_CMD_FFE_EXE          2
#define DPE_CMD_DMA_IN_FFE_EXE   3

#define DPE_CMD_FIFO_1           1
#define DPE_CMD_FIFO_2           2
#define DPE_STS_FIFO_1           3
#define DPE_STS_FIFO_2           4
#define DPE_DMA_FIFO             5
#define DPE_DMA_CTRL_FIFO        6

/* dpeInterruptType for registering an ISR with DPE driver */
#define DPE_INTERRUPT_DPE_FIFO   0
#define DPE_INTERRUPT_FFE        1

/* interrupt handler structure */
typedef struct {
   fap4keTsk_taskPriority_t taskPriority;
   fap4keTsk_task_t         task;
} fap4keDpe_handlerInfo_t;


/* DPE result structure returned by dpe_GetDmaResults */
typedef struct {
   uint8    statusFifoSlotsAvailable;
   uint8    pktMemSlotNum;
   uint8    nugMemSlotNum;
   uint16   ffeStatusWord0;
   uint32   ffeStatusWord1;
} fap4keDpe_Status_t;


/* DPE FIFO Status structure returned by dpe_GetFifoStatus */
typedef struct {
   uint8    WritePtr;
   uint8    ReadPtr;
   uint8    Depth;
   uint16   flags;
         #define DPE_FIFO_FLAGS_FULL         8
         #define DPE_FIFO_FLAGS_NOT_EMPTY    4
         #define DPE_FIFO_FLAGS_NOT_UNDERRUN 2
         #define DPE_FIFO_FLAGS_NOT_OVERRUN  1
} fap4keDpe_FifoStatus_t;


/*
 * FFE Memories
 */

#define FAP4KE_FFE_MEM_SLOTS       8
#define FAP4KE_FFE_NUG_TOTAL_SIZE  256  // words
#define FAP4KE_FFE_NUG_SLOT_SIZE   16   // words
#define FAP4KE_FFE_PKT_TOTAL_SIZE  2048 // bytes
#define FAP4KE_FFE_PKT_SLOT_SIZE   256  // bytes

typedef union {
    struct {
        uint32 debug[12];
        uint32 ipSa;
        uint32 ipDa;
        uint32 l4Ports;
        uint32 ipOffset;
    };
    uint32 u32[FAP4KE_FFE_NUG_SLOT_SIZE];
} __attribute__((packed)) fap4ke_ffeNugSlot_t;

typedef union {
    struct {
        fap4ke_ffeNugSlot_t slot[FAP4KE_FFE_MEM_SLOTS];
        const uint32 ffeStack[FAP4KE_FFE_NUG_TOTAL_SIZE/2];
    };
    uint32 u32[FAP4KE_FFE_NUG_TOTAL_SIZE];
} __attribute__((packed)) fap4ke_ffeNugMem_t;

typedef union {
    uint8  u8[FAP4KE_FFE_PKT_SLOT_SIZE];
    uint32 u32[FAP4KE_FFE_PKT_SLOT_SIZE/4];
} __attribute__((packed)) fap4ke_ffePktSlot_t;

typedef union {
    fap4ke_ffePktSlot_t slot[FAP4KE_FFE_MEM_SLOTS];
    uint8 u8[FAP4KE_FFE_PKT_TOTAL_SIZE];
} __attribute__((packed)) fap4ke_ffePktMem_t;

#define ffeInstMem_p ( (volatile uint32 *)(FAP_DPE_INST_MEM_BASE) )
#define ffeNugMem_p  ( (volatile fap4ke_ffeNugMem_t *)(FAP_DPE_NUG_MEM_BASE) )
#define ffePktMem_p  ( (volatile fap4ke_ffePktMem_t *)(FAP_DPE_PKT_MEM_BASE) )


/*
 * Prototypes
 */

fapRet dpe_Init(void);
fapRet dpe_SubmitDpeCommand( uint8 dpeCommand,
                             uint8 pktMemSlotNum,
                             uint8 nugMemSlotNum,
                             uint16 ffePcCounter,
                             uint32 *pDmaAddress,
                             uint16 dmaBufLength);
fapRet dpe_SubmitFfeExeCommand(uint32 ffeSlotNum,
                               uint32 ffePcCounter);
fapRet dpe_GetDpeStatus( fap4keDpe_Status_t *pDpeStatus);
fapRet dpe_GetDpeStatusWords(fap4keDpe_Status_t *pDpeStatus);
fapRet dpe_IrqHandler(uint32 arg);
fapRet dpe_RegisterIsrHandler( uint32 dpeInterruptType,
                               uint32 dpeInterruptMask,
                               fap4keTsk_task_t *tsk,
                               fap4keTsk_taskPriority_t priority);
fapRet dpe_GetFifoStatus( uint32 fifo, fap4keDpe_FifoStatus_t *pFifoStatus );
fapRet dpe_EnableFfeIrq(BOOL ffeIrqEnable);
fapRet dpe_Reset( BOOL dpeReset );
fapRet dpe_ffe_Reset( BOOL ffeReset );
void dpe_debugDumpRegs( void );

#endif /* __FAP4KE_DPE_H_INCLUDED__ */
