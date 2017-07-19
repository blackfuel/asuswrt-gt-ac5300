 /*
 * Copyright 2016, ASUSTeK Inc.
 * All Rights Reserved.
 *
 * THIS SOFTWARE IS OFFERED "AS IS", AND ASUS GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 *
 */

/* DEBUG DEFINE */
#define BWDPI_DEBUG             "/tmp/BWDPI_DEBUG"
#define BWDPI_SIG_DEBUG         "/tmp/BWSIG_DEBUG"
#define BWDPI_SQLITE_DEBUG      "/tmp/BWSQL_DEBUG"
#define BWDPI_SQLITE_DELOG      "/tmp/BWSQL_LOG"
#define BWDPI_MON_DEBUG         "/tmp/BWMON_DEBUG"
#define BWDPI_MON_DELOG         "/tmp/BWMON_LOG"

/* DEBUG FUNCTION */
#define BWDPI_DBG(fmt,args...) \
	if(f_exists(BWDPI_DEBUG) > 0) { \
		dbg("[BWDPI][%s:(%d)]"fmt, __FUNCTION__, __LINE__, ##args); \
	}

#define BWSIG_DBG(fmt,args...) \
	if(f_exists(BWDPI_SIG_DEBUG) > 0) { \
		dbg("[BWSIG][%s:(%d)]"fmt, __FUNCTION__, __LINE__, ##args); \
	}

#define BWSQL_DBG(fmt,args...) \
	if(f_exists(BWDPI_SQLITE_DEBUG) > 0) { \
		dbg("[BWSQL][%s:(%d)]"fmt, __FUNCTION__, __LINE__, ##args); \
	}

#define BWSQL_LOG(fmt,args...) \
	if(f_exists(BWDPI_SQLITE_DELOG) > 0) { \
		char info[1024]; \
		snprintf(info, sizeof(info), "echo \"[BWDPI_SQLITE]"fmt"\" >> /tmp/BWSQL.log", ##args); \
		system(info); \
	}

#define BWMON_DBG(fmt,args...) \
	if(f_exists(BWDPI_MON_DEBUG) > 0) { \
		dbg("[BWMON][%s:(%d)]"fmt, __FUNCTION__, __LINE__, ##args); \
	}

#define BWMON_LOG(fmt,args...) \
	if(f_exists(BWDPI_MON_DELOG) > 0) { \
		char info[1024]; \
		snprintf(info, sizeof(info), "echo \"[BWMON]"fmt"\" >> /tmp/BWMON.log", ##args); \
		system(info); \
	}
