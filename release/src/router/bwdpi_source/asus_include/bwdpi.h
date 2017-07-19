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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <syslog.h>
#include <getopt.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <net/if.h>
#include <bcmnvram.h>
#include <bcmparams.h>
#include <utils.h>
#include <shutils.h>
#include <shared.h>
#include <rtstate.h>
#include <stdarg.h>
#include <time.h>
#include <libgen.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <sys/time.h>
#include <signal.h>

#include <httpd.h>

// TrendMicro header
#include <TrendMicro.h>

// ASUS debug
#include <bwdpi_common.h>

// command
#define WRED            nvram_get_int("bwdpi_debug_path") ? "/jffs/TM/wred" : "wred"
#define AGENT           nvram_get_int("bwdpi_debug_path") ? "/jffs/TM/tdts_rule_agent" : "tdts_rule_agent"
#define DATACOLLD       nvram_get_int("bwdpi_debug_path") ? "/jffs/TM/dcd" : "dcd"
#define WRED_SET        nvram_get_int("bwdpi_debug_path") ? "/jffs/TM/wred_set_conf" : "wred_set_conf"  // white and black list setup conf
#define WRED_WBL        nvram_get_int("bwdpi_debug_path") ? "/jffs/TM/wred_set_wbl" : "wred_set_wbl"	// white and black list command
#define TCD             nvram_get_int("bwdpi_debug_path") ? "/jffs/TM/tcd" : "tcd"
#define SHN_CTRL        nvram_get_int("bwdpi_debug_path") ? "/jffs/TM/shn_ctrl" : "shn_ctrl"            // TrendMicro new sample command line

// config and folder path
#define DATABASE        nvram_get_int("bwdpi_debug_path") ? "/jffs/TM/rule.trf" : "/tmp/bwdpi/rule.trf"
#define QOS_CONF        nvram_get_int("bwdpi_debug_path") ? "/jffs/TM/qosd.conf" : "/tmp/bwdpi/qosd.conf"
#define WRS_CONF        nvram_get_int("bwdpi_debug_path") ? "/jffs/TM/wred.conf" : "/tmp/bwdpi/wred.conf"
#define APP_SET_CONF    nvram_get_int("bwdpi_debug_path") ? "/jffs/TM/app_patrol.conf" : "/tmp/bwdpi/app_patrol.conf"
#define TMP_BWDPI       nvram_get_int("bwdpi_debug_path") ? "/jffs/TM/" : "/tmp/bwdpi/"
#define DPI_CERT        nvram_get_int("bwdpi_debug_path") ? "/jffs/TM/ntdasus2014.cert" : "/tmp/bwdpi/ntdasus2014.cert"
#define DCD_EULA        nvram_get_int("bwdpi_debug_path") ? "/jffs/TM/dcd.conf" : "/tmp/bwdpi/dcd.conf"

// kernel module
#define TDTS            nvram_get_int("bwdpi_debug_path") ? "/jffs/TM/tdts.ko" : "/usr/bwdpi/tdts.ko"
#define UDB             nvram_get_int("bwdpi_debug_path") ? "/jffs/TM/tdts_udb.ko" : "/usr/bwdpi/tdts_udb.ko"
#define UDBFW           nvram_get_int("bwdpi_debug_path") ? "/jffs/TM/tdts_udbfw.ko" : "/usr/bwdpi/tdts_udbfw.ko"

// pid
#define WREDPID         nvram_get_int("bwdpi_debug_path") ? "/jffs/TM/wred.pid" : "/tmp/bwdpi/wred.pid"
#define DATAPID         nvram_get_int("bwdpi_debug_path") ? "/jffs/TM/dcd.pid" : "/tmp/bwdpi/dcd.pid"

// signature check
#define APPDB           nvram_get_int("bwdpi_debug_path") ? "/jffs/TM/bwdpi.app.db" : "/tmp/bwdpi/bwdpi.app.db"
#define CATDB           nvram_get_int("bwdpi_debug_path") ? "/jffs/TM/bwdpi.cat.db" : "/tmp/bwdpi/bwdpi.cat.db"
#define RULEV           nvram_get_int("bwdpi_debug_path") ? "/jffs/TM/rule.version" : "/tmp/bwdpi/rule.version"

// log and tmp file
#define WRS_FULL_LOG    "/tmp/wrs_full.txt"
#define VP_FULL_LOG     "/tmp/vp_full.txt"
#define TMP_WRS_LOG     "/tmp/tmp_wrs.txt"
#define TMP_VP_LOG      "/tmp/tmp_vp.txt"
#define WRS_VP_LOG      "/jffs/wrs_vp.txt"
#define SIG_VER         "/proc/nk_policy"
#define DPI_VER         "/proc/ips_info"
#define WAN_TMP         nvram_get_int("bwdpi_debug_path") ? "/jffs/TM/dev_wan" : "/tmp/bwdpi/dev_wan"

// module setting
#define TDTSFW_PARA     "/sys/module/tdts_udb/parameters/"
#define DEV_WAN         TDTSFW_PARA"dev_wan"
#define DEV_LAN         TDTSFW_PARA"dev_lan"
#define QOS_WAN         TDTSFW_PARA"qos_wan"
#define QOS_LAN         TDTSFW_PARA"qos_lan"
#define BW_DPI_SET      "/proc/bw_dpi_conf"

// database hidden path and function path
#define BWDPI_DB_DIR    "/jffs/.sys"
#define BWDPI_ANA_DIR   BWDPI_DB_DIR"/TrafficAnalyzer"
#define BWDPI_HIS_DIR   BWDPI_DB_DIR"/WebHistory"
#define BWDPI_MON_DIR   BWDPI_DB_DIR"/AiProtectionMonitor"

// Traffic Analyzer database
#define BWDPI_ANA_DB    (strcmp(nvram_safe_get("bwdpi_ana_path"), "")) ? nvram_safe_get("bwdpi_ana_path") : BWDPI_ANA_DIR"/TrafficAnalyzer.db"

// Web History database
#define BWDPI_HIS_DB    (strcmp(nvram_safe_get("bwdpi_his_path"), "")) ? nvram_safe_get("bwdpi_his_path") : BWDPI_HIS_DIR"/WebHistory.db"

// AiProtection Monitor database
#define BWDPI_MON_DB    (strcmp(nvram_safe_get("bwdpi_mon_path"), "")) ? nvram_safe_get("bwdpi_mon_path") : BWDPI_MON_DIR"/AiProtectionMonitor.db"
#define BWDPI_MON_CC    BWDPI_MON_DIR"/AiProtectionMonitorCCevent.txt"
#define BWDPI_MON_VP    BWDPI_MON_DIR"/AiProtectionMonitorVPevent.txt"
#define BWDPI_MON_MALS  BWDPI_MON_DIR"/AiProtectionMonitorMALSevent.txt"

// Avoid the trigger event loop issue, add a copy file for nt_center usage
#define NT_MON_CC    BWDPI_MON_DIR"/NT-AiMonitorCCevent.txt"
#define NT_MON_VP    BWDPI_MON_DIR"/NT-AiMonitorVPevent.txt"
#define NT_MON_MALS  BWDPI_MON_DIR"/NT-AiMonitorMALSevent.txt"

typedef struct cat_id cid_s;
struct cat_id{
	int id;
	cid_s *next;
};

typedef struct wrs wrs_s;
struct wrs{
	int enabled;
	char mac[18];
	cid_s *ids;
	wrs_s *next;
};

typedef struct mac_list mac_s;
struct mac_list{
	char mac[18];
	mac_s *next;
};

typedef struct mac_group mac_g;
struct mac_group{
	char group_name[128];
	mac_s *macs;
};

typedef struct bwdpi_client bwdpi_device;
struct bwdpi_client{
	char hostname[32];
	char vendor_name[100];
	char type_name[100];
	char device_name[100];
};

//iqos.c
extern void check_qosd_wan_setting(char *dev_wan, int len);
extern void setup_qos_conf();
extern void stop_tm_qos();
extern void start_tm_qos();
extern int tm_qos_main(char *cmd);
extern void start_qosd();
extern void stop_qosd();
extern void restart_qosd();
extern int qosd_main(char *cmd);

//wrs.c
extern void setup_wrs_conf();
extern void stop_wrs();
extern void start_wrs();
extern int wrs_main(char *cmd);
extern void setup_vp_conf();
void free_id_list(cid_s **target_list);
cid_s *get_id_list(cid_s **target_list, char *target_string);
void print_id_list(cid_s *id_list);
void free_wrs_list(wrs_s **target_list);
extern wrs_s *get_all_wrs_list(wrs_s **wrs_list, char *setting_string);
void print_wrs_list(wrs_s *wrs_list);
wrs_s *match_enabled_wrs_list(wrs_s *wrs_list, wrs_s **target_list, int enabled);
void free_mac_list(mac_s **target_list);
mac_s *get_mac_list(mac_s **target_list, const char *target_string);
void print_mac_list(mac_s *mac_list);
void free_group_mac(mac_g **target);
mac_g *get_group_mac(mac_g **mac_group, const char *target);
void print_group_mac(mac_g *mac_group);

//stat.c
extern int stat_main(char *mode, char *name, char *dura, char *date);
extern void get_traffic_stat(char *mode, char *name, char *dura, char *date);
extern void get_traffic_hook(char *mode, char *name, char *dura, char *date, int *retval, webs_t wp);
extern void get_device_hook(char *MAC, int *retval, webs_t wp);
extern void get_device_stat(char *MAC);
extern int device_main(char *MAC);
extern int bwdpi_client_info(char *MAC, bwdpi_device *device);
extern int device_info_main(char *MAC);
extern int wrs_url_main();
extern void redirect_page_status(int cat_id, int *retval, webs_t wp);
extern int get_anomaly_main(char *cmd);
extern int get_app_patrol_main();

//dpi.c
extern int check_daulwan_mode();
extern void stop_dpi_engine_service(int forced);
extern void run_dpi_engine_service();
extern void start_dpi_engine_service();
extern void save_version_of_bwdpi();
extern void setup_dev_wan();

//wrs_app.c
extern int wrs_app_main(char *cmd);
extern int wrs_app_service(int cmd);

//data_collect.c
extern void stop_dc();
extern void start_dc(char *path);
extern int data_collect_main(char *cmd, char *path);

//tools.c
extern void check_filesize(char *path, long int size);

//watchdog_check.c
extern void auto_sig_check();
extern void sqlite_db_check();
extern void tm_eula_check();
