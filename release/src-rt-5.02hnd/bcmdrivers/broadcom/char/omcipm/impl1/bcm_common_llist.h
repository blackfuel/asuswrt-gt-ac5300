/*
<:copyright-broadcom 
 
 Copyright (c) 2002 Broadcom Corporation 
 All Rights Reserved 
 No portions of this material may be reproduced in any form without the 
 written permission of: 
          Broadcom Corporation 
          16215 Alton Parkway 
          Irvine, California 92619 
 All information contained in this document is Broadcom Corporation 
 company private, proprietary, and trade secret. 
 
:>
*/

#ifndef _BCM_COMMON_LLIST_H_
#define _BCM_COMMON_LLIST_H_

#include "bcmtypes.h"
#include <linux/bcm_log.h>

/*
 * Macros
 */

/* Linked List API */

#define BCM_COMMON_DECLARE_LL(_name) BCM_COMMON_LLIST _name

#define BCM_COMMON_DECLARE_LL_ENTRY() BCM_COMMON_LLIST_ENTRY llEntry

#define BCM_COMMON_LL_INIT(_linkedList)                         \
    do {                                                        \
        (_linkedList)->head = NULL;                             \
        (_linkedList)->tail = NULL;                             \
    } while(0)

/* #define BCM_COMMON_LL_IS_EMPTY(_linkedList)                               \ */
/*     ( ((_linkedList)->head == NULL) && ((_linkedList)->tail == NULL) ) */

#define BCM_COMMON_LL_IS_EMPTY(_linkedList) ( (_linkedList)->head == NULL )

#define BCM_COMMON_LL_INSERT(_linkedList, _newObj, _pos, _currObj)      \
    do {                                                                \
        if(BCM_COMMON_LL_IS_EMPTY(_linkedList))                         \
        {                                                               \
            (_linkedList)->head = (void *)(_newObj);                    \
            (_linkedList)->tail = (void *)(_newObj);                    \
            (_newObj)->llEntry.prev = NULL;                             \
            (_newObj)->llEntry.next = NULL;                             \
            BCM_LOG_DEBUG(BCM_LOG_ID_OMCIPM, "Inserted FIRST object in %s", #_linkedList); \
        }                                                               \
        else                                                            \
        {                                                               \
            if((_pos) == LL_POSITION_APPEND)                    \
            {                                                           \
                typeof(*(_newObj)) *_tailObj = (_linkedList)->tail;     \
                _tailObj->llEntry.next = (_newObj);                     \
                (_newObj)->llEntry.prev = _tailObj;                     \
                (_newObj)->llEntry.next = NULL;                         \
                (_linkedList)->tail = (_newObj);                        \
                BCM_LOG_DEBUG(BCM_LOG_ID_OMCIPM, "APPENDED object in %s", #_linkedList); \
            }                                                           \
            else                                                        \
            {                                                           \
                if((_pos) == LL_POSITION_BEFORE)                \
                {                                                       \
                    typeof(*(_newObj)) *_prevObj = (_currObj)->llEntry.prev; \
                    (_currObj)->llEntry.prev = (_newObj);               \
                    (_newObj)->llEntry.prev = _prevObj;                 \
                    (_newObj)->llEntry.next = (_currObj);               \
                    if(_prevObj != NULL)                                \
                    {                                                   \
                        _prevObj->llEntry.next = (_newObj);             \
                        BCM_LOG_DEBUG(BCM_LOG_ID_OMCIPM, "Inserted INNER object (BEFORE) in %s", #_linkedList); \
                    }                                                   \
                    if((_linkedList)->head == (_currObj))               \
                    {                                                   \
                        (_linkedList)->head = (_newObj);                \
                        BCM_LOG_DEBUG(BCM_LOG_ID_OMCIPM, "Inserted HEAD object in %s", #_linkedList); \
                    }                                                   \
                }                                                       \
                else                                                    \
                {                                                       \
                    typeof(*(_newObj)) *_nextObj = (_currObj)->llEntry.next; \
                    (_currObj)->llEntry.next = (_newObj);               \
                    (_newObj)->llEntry.prev = (_currObj);               \
                    (_newObj)->llEntry.next = _nextObj;                 \
                    if(_nextObj != NULL)                                \
                    {                                                   \
                        _nextObj->llEntry.prev = (_newObj);             \
                        BCM_LOG_DEBUG(BCM_LOG_ID_OMCIPM, "Inserted INNER object (AFTER) in %s", #_linkedList); \
                    }                                                   \
                    if((_linkedList)->tail == (_currObj))               \
                    {                                                   \
                        (_linkedList)->tail = (_newObj);                \
                        BCM_LOG_DEBUG(BCM_LOG_ID_OMCIPM, "Inserted TAIL object in %s", #_linkedList); \
                    }                                                   \
                }                                                       \
            }                                                           \
        }                                                               \
    } while(0)

#define BCM_COMMON_LL_APPEND(_linkedList, _obj)                         \
    BCM_COMMON_LL_INSERT(_linkedList, _obj, LL_POSITION_APPEND, _obj)

#define BCM_COMMON_LL_REMOVE(_linkedList, _obj)                         \
    do {                                                                \
        BCM_ASSERT(!BCM_COMMON_LL_IS_EMPTY(_linkedList));               \
        if((_linkedList)->head == (_obj) && (_linkedList)->tail == (_obj)) \
        {                                                               \
            (_linkedList)->head = NULL;                                 \
            (_linkedList)->tail = NULL;                                 \
            BCM_LOG_DEBUG(BCM_LOG_ID_OMCIPM, "Removed LAST object from %s", #_linkedList); \
        }                                                               \
        else                                                            \
        {                                                               \
            if((_linkedList)->head == (_obj))                           \
            {                                                           \
                typeof(*(_obj)) *_nextObj = (_obj)->llEntry.next;       \
                (_linkedList)->head = _nextObj;                         \
                _nextObj->llEntry.prev = NULL;                          \
                BCM_LOG_DEBUG(BCM_LOG_ID_OMCIPM, "Removed HEAD object from %s", #_linkedList); \
            }                                                           \
            else if((_linkedList)->tail == (_obj))                      \
            {                                                           \
                typeof(*(_obj)) *_prevObj = (_obj)->llEntry.prev;       \
                (_linkedList)->tail = _prevObj;                         \
                _prevObj->llEntry.next = NULL;                          \
                BCM_LOG_DEBUG(BCM_LOG_ID_OMCIPM, "Removed TAIL object from %s", #_linkedList); \
            }                                                           \
            else                                                        \
            {                                                           \
                typeof(*(_obj)) *_prevObj = (_obj)->llEntry.prev;       \
                typeof(*(_obj)) *_nextObj = (_obj)->llEntry.next;       \
                _prevObj->llEntry.next = (_obj)->llEntry.next;          \
                _nextObj->llEntry.prev = (_obj)->llEntry.prev;          \
                BCM_LOG_DEBUG(BCM_LOG_ID_OMCIPM, "Removed INNER object from %s", #_linkedList); \
            }                                                           \
        }                                                               \
    } while(0)

#define BCM_COMMON_LL_GET_HEAD(_linkedList) (_linkedList).head

#define BCM_COMMON_LL_GET_NEXT(_obj) (_obj)->llEntry.next

/*
 * Type definitions
 */

typedef enum {
    LL_POSITION_BEFORE=0,
    LL_POSITION_AFTER,
    LL_POSITION_APPEND,
    LL_POSITION_MAX
} LL_INSERT_POSITION;

typedef struct {
    void* head;
    void* tail;
} BCM_COMMON_LLIST, *PBCM_COMMON_LLIST;

typedef struct {
    void* prev;
    void* next;
} BCM_COMMON_LLIST_ENTRY, *PBCM_COMMON_LLIST_ENTRY;

#endif /* _BCM_COMMON_LLIST_H_ */
