#ifndef __CFG_SLAVELIST_H__
#define __CFG_SLAVELIST_H__

#define REPORT_TIME_INTERVAL	30
#define OFFLINE_THRESHOLD	(REPORT_TIME_INTERVAL * 3)

#define CFG_FILE_LOCK		"cfg_mnt"
#define KEY_SHM_CFG		2001
#define CFG_CLIENT_NUM		32
#define MAC_LIST_JSON_FILE	"/tmp/maclist.json"
typedef struct _CM_CLIENT_TABLE {
	char alias[CFG_CLIENT_NUM][33];
        unsigned char ipAddr[CFG_CLIENT_NUM][4];
        unsigned char macAddr[CFG_CLIENT_NUM][6];
	unsigned char realMacAddr[CFG_CLIENT_NUM][6];
	time_t reportStartTime[CFG_CLIENT_NUM];
	unsigned char pap2g[CFG_CLIENT_NUM][6];
	unsigned char pap5g[CFG_CLIENT_NUM][6];
	int rssi2g[CFG_CLIENT_NUM];
	int rssi5g[CFG_CLIENT_NUM];
	unsigned char ap2g[CFG_CLIENT_NUM][6];
	unsigned char ap5g[CFG_CLIENT_NUM][6];
	unsigned char ap5g1[CFG_CLIENT_NUM][6];
	int level[CFG_CLIENT_NUM];
	int maxLevel;
	int count;
} CM_CLIENT_TABLE, *P_CM_CLIENT_TABLE;

#endif /* __CFG_SLAVELIST_H__ */
/* End of cfg_slavelist.h */
