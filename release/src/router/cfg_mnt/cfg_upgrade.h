#ifndef __CFG_UPGRADE_H__
#define __CFG_UPGRADE_H__

#define TIMES_FW_CHECK	30

enum firmwareStatus {
	FW_NONE = 0,
	FW_START,
	FW_FAIL_RETRIEVE,
	FW_FAIL_DOWNLOAD,
	FW_IS_WRONG,
	FW_IS_DOWNLOADING,
	FW_SUCCESS_DOWNLOAD,
	FW_NO_NEED_UPGRADE,
	FW_DO_UPGRADE,
	FW_MAX
};

extern void cm_doFirmwareCheck();
extern void cm_doFirmwareStatusReport();
extern void cm_upgradeFirmware();
extern void cm_cancelFirmwareUpgrade();

#endif /* __CFG_UPGRADE_H__ */
/* End of cfg_upgrade.h */
