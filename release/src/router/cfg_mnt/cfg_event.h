#ifndef __CFG_EVENT_H__
#define __CFG_EVENT_H__

#define WEVENT_PREFIX	"WEVENT"
#define HTTPD_PREFIX	"HTTPD"
#define EVENT_ID	"EID"

enum httpdEventType {
        EID_HTTPD_FW_UPGRADE = 1
};

enum wEventType {
        EID_WEVENT_DEVICE_CONNECTED = 1,
	EID_WEVENT_DEVICE_DISCONNECTED
};

#endif /* __CFG_EVENT_H__ */
/* End of cfg_event.h */
