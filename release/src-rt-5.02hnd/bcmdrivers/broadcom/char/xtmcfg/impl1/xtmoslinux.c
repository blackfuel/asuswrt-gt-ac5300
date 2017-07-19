/*
    <:copyright-BRCM:2011:proprietary:standard
    
       Copyright (c) 2011 Broadcom 
       All Rights Reserved
    
     This program is the proprietary software of Broadcom and/or its
     licensors, and may only be used, duplicated, modified or distributed pursuant
     to the terms and conditions of a separate, written license agreement executed
     between you and Broadcom (an "Authorized License").  Except as set forth in
     an Authorized License, Broadcom grants no license (express or implied), right
     to use, or waiver of any kind with respect to the Software, and Broadcom
     expressly reserves all rights in and to the Software and all intellectual
     property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE
     NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY
     BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.
    
     Except as expressly set forth in the Authorized License,
    
     1. This program, including its structure, sequence and organization,
        constitutes the valuable trade secrets of Broadcom, and you shall use
        all reasonable efforts to protect the confidentiality thereof, and to
        use this information only in connection with your use of Broadcom
        integrated circuit products.
    
     2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS"
        AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS OR
        WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
        RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND
        ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT,
        FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR
        COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE
        TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT OF USE OR
        PERFORMANCE OF THE SOFTWARE.
    
     3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
        ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
        INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY
        WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
        IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES;
        OR (ii) ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
        SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS
        SHALL APPLY NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY
        LIMITED REMEDY.
    :>    


*/
/***************************************************************************
 * File Name  : xtmoslinux.c
 *
 * Description: This file contains Linux operation system function calls.
 *              All operating system specific functions are isolated to a
 *              source file named XtmOs"xxx" where "xxx" is the operating
 *              system name.
 ***************************************************************************/


/* Includes. */
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/timer.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
#include <linux/semaphore.h>
#else
#include <asm/semaphore.h>
#endif
#include <asm/io.h>
#include <bcmtypes.h>
#include <atmosservices.h>
#include <bcm_map.h>
#include <bcm_intr.h>
#include <board.h>
#include <boardparms.h>
#include <bcmnetlink.h>


#define XTM_EVT_TRAFFIC_TYPE_MISMATCH              1   /* Also duplicated in xtmcfgimpl.h */
#define XTM_EVT_TRAFFIC_TYPE_MISMATCH_AND_RESTART  2   /* Also duplicated in xtmcfgimpl.h */

/* typedefs. */
typedef struct
{
    UINT32 ulInUse;
    UINT32 ulTimeout;
    struct tasklet_struct Task;
    struct timer_list TimerList;
} DEFERRED_INFO, *PDEFERRED_INFO;

typedef struct
{
    UINT32 ulBase;
    UINT32 ulOrder;
} PAGE_ALLOC_INFO, *PPAGE_ALLOC_INFO;

/* Defines. */
#define MAX_SEMS            50
#define MAX_PAGE_ALLOCS     50
#define MAX_INT_DATA_TIMERS 8

#define INTR_ATM            (1 << (INTERRUPT_ID_ATM - INTERNAL_ISR_TABLE_OFFSET))

#define index_of( array, elem )     ({                      \
                                        int i;              \
                                        i = (elem) >= (array) ? (long int)((char *)(elem)-(char *)(array)) / sizeof((array)[0]) : -1;            \
                                        i = (i!=-1) && (((long)((char *)(elem) - (char *)(array)) - sizeof((array)[0]) * i) == 0) ? i : -1; \
                                        i; \
                                    })


/* Globals. */
static struct InternalData
{
    struct semaphore Sems[MAX_SEMS];
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
    int    isSemTaken[MAX_SEMS];
#endif
    DEFERRED_INFO DeferredInfo;
    PAGE_ALLOC_INFO PageAllocInfo[MAX_PAGE_ALLOCS];
    int nDisabledBh;
    int nRequestCount;
    struct timer_list IntDataTimer[MAX_INT_DATA_TIMERS];
} InternalData;

static struct InternalData *g_pData = &InternalData;

/***************************************************************************
 * Function Name: XtmOsInitialize
 * Description  : Operating system specific function initialization.
 * Returns      : RTN_SUCCESS if successful or error status.
 ***************************************************************************/
void XtmOsInitialize( void )
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
    int i;
#endif

    memset( g_pData, 0x00, sizeof(struct InternalData) );

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
    for (i = 0; i < MAX_SEMS; i++)
        sema_init(&g_pData->Sems[i], 1);
#else
    /* Initialize semaphore structures to all bits on in order to determine
     * a not in use state.
     */
    memset( g_pData->Sems, 0xff, sizeof(g_pData->Sems) );
#endif

    memset( &g_pData->DeferredInfo, 0x00, sizeof(g_pData->DeferredInfo) );
    memset( g_pData->PageAllocInfo, 0x00, sizeof(g_pData->PageAllocInfo) );
    g_pData->nDisabledBh = 0;
    g_pData->nRequestCount = 0;
} /* XtmOsInitialize */


/***************************************************************************
 * Function Name: XtmOsAlloc
 * Description  : Allocates kernel memory.
 * Returns      : Address of allocated memory of NULL.
 ***************************************************************************/
char *XtmOsAlloc( UINT32 ulSize )
{
    char *pRet = NULL;
    int nGfpDma = GFP_DMA;

    /* The largest block that kmalloc can allocate is 128K.  If the size is
     * greater than 128K, allocate memory pages.
     */
    if( ulSize < (1024 * 128)  )
    {
        if( in_softirq() || in_irq() )
            pRet = (char *) kmalloc(ulSize, GFP_ATOMIC);
        else
            pRet = (char *) kmalloc(ulSize, GFP_KERNEL);
    }
    else
    {
        /* Memory pages need to be allocated.  The number of pages must be an
         * exponent of 2
         */
        PPAGE_ALLOC_INFO pPai;
        UINT32 i;
        for(i=0, pPai=g_pData->PageAllocInfo; i < MAX_PAGE_ALLOCS; i++, pPai++)
        {
            if( pPai->ulBase == 0 )
            {
                pPai->ulOrder = 0;
                while( ulSize > (PAGE_SIZE * (1 << pPai->ulOrder)))
                    pPai->ulOrder++;
                pPai->ulBase = __get_free_pages(GFP_KERNEL | nGfpDma,
                    pPai->ulOrder);
                pRet = (char *) pPai->ulBase;
                break;
            }
        }
    }

    return( pRet );
} /* XtmOsAlloc */


/***************************************************************************
 * Function Name: XtmOsFree
 * Description  : Frees memory.
 * Returns      : None.
 ***************************************************************************/
void XtmOsFree( char *pBuf )
{
    PPAGE_ALLOC_INFO pPai;
    UINT32 i;

    if( pBuf )
    {
        for(i=0, pPai=g_pData->PageAllocInfo; i < MAX_PAGE_ALLOCS; i++, pPai++)
        {
            if( pPai->ulBase == (UINT32) pBuf )
            {
                free_pages( pPai->ulBase, pPai->ulOrder );
                pPai->ulBase = pPai->ulOrder = 0;
                break;
            }
        }

        if( i == MAX_PAGE_ALLOCS )
            kfree( pBuf );
    }
}


/***************************************************************************
 * Function Name: XtmOsCreateSem
 * Description  : Finds an unused semaphore and initializes it.
 * Returns      : Semaphore handle if successful or NULL.
 ***************************************************************************/
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
UINT32 XtmOsCreateSem( UINT32 ulInitialState )
{
    int i;
    struct semaphore *pSem = NULL;
    
    for (i = 0; i < MAX_SEMS; i++)
    {
        if (g_pData->isSemTaken[i] == 0) // quick test outside of the lock to prevent unnecessary locking
        {
            pSem = &g_pData->Sems[i];
            // we're going to lock the actual semaphore to take it:
            if (down_trylock( pSem ) != 0)
                continue; //grabbing semaphore was unsuccesful -- go to next semaphore
            else
            {
                // Semaphore is locked.  We have to check taken again now that we're locked
                if ( unlikely(g_pData->isSemTaken[i] == 1))
                {
                    // semaphore already taken, move to next:
                    up(pSem);
                    continue;
                }
                else
                {
                    g_pData->isSemTaken[i] = 1;
                    // up(pSem); don't have to do this because of the sema_init
                    /* Note, no one else can take this semaphore at this point, so we're free to
                       reinitialize it */
                    sema_init(pSem, ulInitialState) ;                   
                    return (UINT32)pSem;
                }
            }
        }
    }

    // no free semaphores -- failing:
    return 0;
}

#else
/* Just a note: the following code is not actually concurrently safe... */
UINT32 XtmOsCreateSem( UINT32 ulInitialState )
{
    UINT32 i;
    struct semaphore *pSem = NULL;
    struct semaphore *pSemRet = NULL;

    /* Find an unused semaphore structure. */
    for( i = 0, pSem = g_pData->Sems; i < MAX_SEMS; i++, pSem++ )
    {
        if( (pSem->count.counter) == ~0 )
        {
            /* An unused semaphore is found.  Initialize it. */
            memset( pSem, 0x00, sizeof(struct semaphore) );
            sema_init( pSem, (int) ulInitialState );
            pSemRet = pSem;
            break;
        }
    }

    return( (UINT32) pSemRet );
} /* XtmOsCreateSem */
#endif


/***************************************************************************
 * Function Name: XtmOsDeleteSem
 * Description  : Makes semaphore available.
 * Returns      : None.
 ***************************************************************************/
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
void XtmOsDeleteSem( unsigned long ulSem )
{
    /* Mark semaphore unused. */
    int i;
    i = index_of(g_pData->Sems, (struct semaphore *) ulSem);
    
    if (i == -1 || i > MAX_SEMS)
    {
        printk("ERROR %s\n\n\n", __func__); // make sure someone notices!!
        return; // bad sem Id;
    }

    sema_init((struct semaphore *) ulSem, 1);
    g_pData->isSemTaken[i] = 0;

} /* XtmOsDeleteSem */
#else
void XtmOsDeleteSem( UINT32 ulSem )
{
    /* Mark semaphore unused. */
    memset( (char *) ulSem, 0xff, sizeof(struct semaphore) );
} /* XtmOsDeleteSem */
#endif

/***************************************************************************
 * Function Name: XtmOsRequestSemTo
 * Description  : Timeout handler for XtmOsRequestSem
 * Returns      : 0 if successful.
 ***************************************************************************/
void XtmOsRequestSemTo( unsigned long ulSem )
{
    up( (struct semaphore *) ulSem );
} /* XtmOsRequestSemTo */


/***************************************************************************
 * Function Name: XtmOsRequestSem
 * Description  : Requests ownership of the semaphore. ulTimeout is in ms.
 * Returns      : 0 if successful.
 ***************************************************************************/
UINT32 XtmOsRequestSem( unsigned long ulSem, UINT32 ulTimeout )
{
    int nRet = 0;

    /* A bottom half should run without any task context switches and also
     * should not block.  Therefore, just return if currently executing in a bh.
     */
    g_pData->nRequestCount++;
    if( !in_softirq() )
    {
        /* If the timeout is big, no need to start timer. */
        if( ulTimeout > 0x80000000 )
        {
            nRet = down_interruptible( (struct semaphore *) ulSem );

            /* At this point, the current thread is protected from other tasks
             * but not from a bottom half.  Disable bottom halves.  Doing this
             * assumes that the semaphore is being used as a mutex.  A future
             * change may be necessary to have both critical section functions
             * and synchronization functions that use semaphores.
             */
            if( nRet == 0 && g_pData->nDisabledBh == 0 )
            {
                g_pData->nDisabledBh = 1;
                local_bh_disable();
            }
        }
        else
        {
            struct timer_list Timer;
            int nTimerDeleted;

            /* Convert ms to jiffies.  If the timeout is less than the
             * granularity of the system clock, wait one jiffy.
             */
            if( (ulTimeout = (ulTimeout * HZ) / 1000) == 0 )
                ulTimeout = 1;

            init_timer (&Timer);
            Timer.expires = jiffies + ulTimeout;
            Timer.function = XtmOsRequestSemTo;
            Timer.data = ulSem;

            add_timer (&Timer);
            nRet = down_interruptible( (struct semaphore *) ulSem );
            nTimerDeleted = del_timer( &Timer );

            /* If the timer is still active and the semaphore was not
             * interrupted, then there was a timeout.
             */
            if( nTimerDeleted == 0 && !nRet )
                nRet = -1; /* Timed out. */
        }
    }

    return( nRet );
} /* XtmOsRequestSem */


/***************************************************************************
 * Function Name: XtmOsReleaseSem
 * Description  : Releases ownership of the semaphore.
 * Returns      : None.
 ***************************************************************************/
void XtmOsReleaseSem( unsigned long ulSem )
{
    if( --g_pData->nRequestCount <= 0 )
    {
        g_pData->nRequestCount = 0;

        /* If XtmOsRequestSem had disabled bottom havles, reenable here. */
        if( g_pData->nDisabledBh == 1 )
        {
            g_pData->nDisabledBh = 0;
            local_bh_enable();
        }
    }

    if( !in_softirq() )
        up( (struct semaphore *) ulSem );
} /* XtmOsReleaseSem */


/***************************************************************************
 * Function Name: XtmOsDisableInts
 * Description  : Disables interrupts.
 * Returns      : 1 for compatibility
 ***************************************************************************/
UINT32 XtmOsDisableInts( void )
{
    unsigned long flags;

    local_save_flags(flags);
    local_irq_disable();
    return( flags );
} /* XtmOsDisableInts */


/***************************************************************************
 * Function Name: XtmOsEnableInts
 * Description  : Enables interrupts.
 * Returns      : None.
 ***************************************************************************/
void XtmOsEnableInts( unsigned long flags )
{
    local_irq_restore(flags);
} /* XtmOsEnableInts */


/***************************************************************************
 * Function Name: XtmOsDelay
 * Description  : Delays a specified number of milliseconds.
 * Returns      : None.
 ***************************************************************************/
void XtmOsDelay( UINT32 ulTimeout )
{
    if( !in_softirq() && !in_irq() )
    {
        wait_queue_head_t wait;

        /* Convert ms to jiffies.  If the timeout is less than the granularity of
         * the system clock, wait one jiffy.
         */
        if( (ulTimeout = (ulTimeout * HZ) / 1000) == 0 )
            ulTimeout = 1;

        init_waitqueue_head(&wait);
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 0, 0)
        interruptible_sleep_on_timeout(&wait, (int) ulTimeout );
#else
        wait_event_interruptible_timeout(wait, 1 == 0, ulTimeout); 
#endif
    }
} /* XtmOsDelay */


/***************************************************************************
 * Function Name: XtmOsTickGet
 * Description  : Returns the current number of milliseconds since the board
 *                was booted.
 * Returns      : Current number of milliesconds.
 ***************************************************************************/
UINT32 XtmOsTickGet( void )
{
    return( jiffies * (1000 / HZ) );
} /* XtmOsTickGet */


/***************************************************************************
 * Function Name: XtmOsTickCheck
 * Description  : Calculates if the number of milliseconds has expired.
 * Returns      : 1 if the number of milliseconds has expired, 0 if not.
 ***************************************************************************/
UINT32 XtmOsTickCheck( unsigned long ulWaitBase, unsigned long ulMsToWait )
{
    return( time_before(jiffies, ((HZ * (ulWaitBase + ulMsToWait)) / 1000))
        ? 0 : 1 );
} /* XtmOsTickCheck */


/***************************************************************************
 * Function Name: XtmOsDeferredTo
 * Description  : Timeout handler for XtmOsInitDeferredHandler.
 * Returns      : 0 if successful.
 ***************************************************************************/
void XtmOsDeferredTo( unsigned long ulHandle )
{
    PDEFERRED_INFO pDeferredInfo = (PDEFERRED_INFO) ulHandle;
    if( pDeferredInfo->ulInUse == 1 )
    {
        /* Schedule the bottom half. */
        tasklet_schedule(&pDeferredInfo->Task);

        /* Restart the timer. */
        pDeferredInfo->TimerList.expires = jiffies + pDeferredInfo->ulTimeout;
        pDeferredInfo->TimerList.function = XtmOsDeferredTo;
        pDeferredInfo->TimerList.data = (UINT32) pDeferredInfo;
        add_timer( &pDeferredInfo->TimerList );
    }
} /* XtmOsDeferredTo */


/***************************************************************************
 * Function Name: XtmOsInitDeferredHandler
 * Description  : Sets up a function for post ISR processing.
 * Returns      : handle that is used in subsequent calls or 0.
 ***************************************************************************/
UINT32 XtmOsInitDeferredHandler( void *pFnEntry, UINT32 ulFnParm,
    UINT32 ulTimeout )
{
    UINT32 ulHandle = 0;

    if( g_pData->DeferredInfo.ulInUse == 0 )
    {
        g_pData->DeferredInfo.ulInUse = 1;

        /* Initialize a tasklet. */
        tasklet_init(&(g_pData->DeferredInfo.Task), (void *) pFnEntry, (unsigned long) ulFnParm);

        /* Start a timer.  Convert ms to jiffies.  If the timeout is less than
         * the granularity of the system clock, wait one jiffy.
         */
        if( (ulTimeout = (ulTimeout * HZ) / 1000) == 0 )
            g_pData->DeferredInfo.ulTimeout = 1;
        else
            g_pData->DeferredInfo.ulTimeout = ulTimeout;

        init_timer( &g_pData->DeferredInfo.TimerList );
        g_pData->DeferredInfo.TimerList.expires = jiffies + g_pData->DeferredInfo.ulTimeout;
        g_pData->DeferredInfo.TimerList.function = XtmOsDeferredTo;
        g_pData->DeferredInfo.TimerList.data = (UINT32) &g_pData->DeferredInfo;
        add_timer( &g_pData->DeferredInfo.TimerList );

        ulHandle = (UINT32) &g_pData->DeferredInfo;
    }
    return( ulHandle );
} /* XtmOsInitDeferredHandler */


/***************************************************************************
 * Function Name: XtmOsScheduleDeferred
 * Description  : Schedules the deferred processing function to run.
 * Returns      : None.
 ***************************************************************************/
void XtmOsScheduleDeferred( UINT32 ulHandle )
{
    PDEFERRED_INFO pDeferredInfo = (PDEFERRED_INFO) ulHandle;

    if( pDeferredInfo->ulInUse == 1 )
    {
        tasklet_schedule(&pDeferredInfo->Task);
    }
} /* XtmOsScheduleDeferred */


/***************************************************************************
 * Function Name: XtmOsUninitDeferredHandler
 * Description  : Uninitializes the deferred handler resources.
 * Returns      : handle that is used in subsequent calls or 0.
 ***************************************************************************/
void XtmOsUninitDeferredHandler( UINT32 ulHandle )
{
    PDEFERRED_INFO pDeferredInfo = (PDEFERRED_INFO) ulHandle;

    if( pDeferredInfo->ulInUse == 1 )
    {
        pDeferredInfo->ulInUse = 2;
        del_timer( &pDeferredInfo->TimerList );
        pDeferredInfo->ulInUse = 0;
    }
} /* XtmOsUninitDeferredHandler */


/***************************************************************************
 * Function Name: XtmOsStartTimer
 * Description  : Starts a timer.
 * Returns      : 0 if successful, -1 if not
 ***************************************************************************/
UINT32 XtmOsStartTimer( void *pFnEntry, UINT32 ulFnParm, UINT32 ulTimeout )
{
    UINT32 ulRet = (UINT32) -1;
    struct timer_list *pTimer;
    int i;

    for( i = 0, pTimer = &g_pData->IntDataTimer[0]; i < MAX_INT_DATA_TIMERS;
        i++, pTimer++ )
    {
        if( !timer_pending( pTimer ) )
        {
            /* Start a timer.  Convert ms to jiffies.  If the timeout is less than
             * the granularity of the system clock, wait one jiffy.
             */
            if( (ulTimeout = (ulTimeout * HZ) / 1000) == 0 )
                ulTimeout = 1;

            init_timer( pTimer );
            pTimer->expires = jiffies + ulTimeout;
            pTimer->function = pFnEntry;
            pTimer->data = ulFnParm;
            add_timer( pTimer );
            ulRet = 0;
            break;
        }
    }

    return( ulRet );
} /* XtmOsStartTimer */


void DummyXtmOsPrintf( char *pFmt, ... )
{
} /* XtmOsPrintf */

/***************************************************************************
 * Function Name: XtmOsPrintf
 * Description  : Outputs text to the console.
 * Returns      : None.
 ***************************************************************************/
void XtmOsPrintf( char *pFmt, ... )
{
    va_list args;
    char buf[256];

    va_start(args, pFmt);
    vsnprintf(buf, sizeof(buf), pFmt, args);
    va_end(args);

    printk(buf);
} /* XtmOsPrintf */


/***************************************************************************
 * Function Name: XtmOsChipRev
 * Description  : Returns the DSL chip revision.
 * Returns      : None.
 ***************************************************************************/
UINT32 XtmOsChipRev( void )
{
    return( PERF->RevID & 0xfffeffff );
} /* XtmOsChipRev */


void XtmOsGetTimeOfDay(UINT8 *vp, void *dummy)
{
	struct timeval *tvp = (struct timeval *) vp ;
	do_gettimeofday(tvp);
}

void XtmOsAddTimeOfDay(UINT8 *vp, UINT32 sec) 
{
	struct timeval *tvp = (struct timeval *) vp ;
	tvp->tv_sec += sec ;
}

UINT32 XtmOsGetTimeStamp(void)
{
  struct timeval tv;

  XtmOsGetTimeOfDay((UINT8 *) &tv, NULL);
  return (tv.tv_sec * 1000000 + tv.tv_usec);
}

//#define XtmOsCompareTimer(ap,bp,compare)  (((struct timeval*) (ap))->tv_sec != ((struct timeval*) (bp))->tv_sec ? ((struct timeval*) (ap))->tv_sec compare ((struct timeval*) (bp))->tv_sec : ((struct timeval*) (ap))->tv_usec compare ((struct timeval*) (bp))->tv_usec)
BOOL XtmOsIsTimerGreater(UINT8 *ap, UINT8 *bp)
{
	struct timeval*  tvp1 = (struct timeval*) ap;
	struct timeval*  tvp2 = (struct timeval*) bp;

	if (tvp1->tv_sec != tvp2->tv_sec) {
		if (tvp1->tv_sec > tvp2->tv_sec) {
			if (tvp1->tv_usec > tvp2->tv_usec) {
				return TRUE;
			}
		}
	} /* if equal */

	return FALSE;
}

UINT32 XtmOsTimeInUsecs (UINT8 *vp)
{
	struct timeval *tvp = (struct timeval *) vp ;
	return (tvp->tv_sec*1000000+tvp->tv_usec) ;
}

/* This function can be used to invoke monitor task, through XTM events */
void XtmOsSendSysEvent (int EventId)
{
	UINT32  msgData ;

	if (EventId == XTM_EVT_TRAFFIC_TYPE_MISMATCH_AND_RESTART) {
		msgData = 0x0 ;
		kerSysSendtoMonitorTask (MSG_NETLINK_BRCM_LINK_TRAFFIC_TYPE_MISMATCH,(UINT8 *) &msgData,
				sizeof (msgData)) ;
	}
	else {
		msgData = EventId ;
		kerSysSendtoMonitorTask (MSG_NETLINK_BRCM_LINK_TRAFFIC_TYPE_MISMATCH,(UINT8 *) &msgData,
				sizeof (msgData)) ;
	}

	return ;
}
