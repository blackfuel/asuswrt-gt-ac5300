#ifndef __CFG_EVENT_H__
#define __CFG_EVENT_H__

#define WEVENT_PREFIX	"WEVENT"
#define HTTPD_PREFIX	"HTTPD"
#define EVENT_ID	"EID"
#define SLAVE_MAC	"MAC"
#define LOGIN_IP	"IP"

enum httpdEventType {
	EID_HTTPD_FW_CHECK = 1,
	EID_HTTPD_FW_UPGRADE,
	EID_HTTPD_REMOVE_SLAVE,
	EID_HTTPD_RESET_DEFAULT,
	EID_HTTPD_START_WPS
};

enum wEventType {
        EID_WEVENT_DEVICE_CONNECTED = 1,
	EID_WEVENT_DEVICE_DISCONNECTED
};

#endif /* __CFG_EVENT_H__ */
/* End of cfg_event.h */
