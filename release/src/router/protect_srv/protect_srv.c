 /*
 * Copyright 2017, ASUSTeK Inc.
 * All Rights Reserved.
 * 
 * THIS SOFTWARE IS OFFERED "AS IS", AND ASUS GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 *
 */
 
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <syslog.h>
/*--*/
#include <libptcsrv.h>
#include "nvram.cc"

#ifdef ASUSWRT_SDK /* ASUSWRT SDK */
#else /* DSL_ASUSWRT SDK */
#endif

#ifdef RTCONFIG_NOTIFICATION_CENTER
#include <libnt.h>
#endif

#define MyDBG(fmt,args...) \
	if(isFileExist(PROTECT_SRV_DEBUG) > 0) { \
		Debug2Console("[ProtectionSrv][%s:(%d)]"fmt, __FUNCTION__, __LINE__, ##args); \
	}
#define ErrorMsg(fmt,args...) \
	Debug2Console("[ProtectionSrv][%s:(%d)]"fmt, __FUNCTION__, __LINE__, ##args);

#define MUTEX pthread_mutex_t
#define MUTEXINIT(m) pthread_mutex_init(m, NULL)
#define MUTEXLOCK(m) pthread_mutex_lock(m)
#define MUTEXTRYLOCK(m) pthread_mutex_trylock(m)
#define MUTEXUNLOCK(m) pthread_mutex_unlock(m)
#define MUTEXDESTROY(m) pthread_mutex_destroy(m)


#define RETRY 5
#define PROTECTION_VALIDITY_TIME 5 /* minutes */

enum {
	F_OFF=0,
	F_ON
};

static int RECV_SIG_FLAG = 0;
static int WAN_LOCK_SSH_FLAG = 0;
static int LAN_LOCK_SSH_FLAG = 0;
static int LAN_LOCK_TELNET_FLAG = 0;
static int WAN_ERROR_SSH_CNT = 0;
static int LAN_ERROR_SSH_CNT = 0;
static int LAN_ERROR_TELNET_CNT = 0;
time_t WAN_LOCK_SSH_T;
time_t LAN_LOCK_SSH_T;
time_t LAN_LOCK_TELNET_T;
static int terminated = 1;

#ifdef RTCONFIG_NOTIFICATION_CENTER
static void SEND_FAIL_LOGIN_EVENT(int e, char *ip)
{
		char msg[100];
		snprintf(msg, sizeof(msg), "{\"IP\":\"%s\",\"msg\":\"\"}", ip);
		SEND_NT_EVENT(e, msg);
}
#endif

static uint32_t parseIpaddrstr(char const *ipAddress)
{
	unsigned int ip[4];
	
	if ( 4 != sscanf(ipAddress, "%u.%u.%u.%u", &ip[0], &ip[1], &ip[2], &ip[3]) ) {
		return 0;
	}
	
	return ip[3] + ip[2] * 0x100 + ip[1] * 0x10000ul + ip[0] * 0x1000000ul;
}
static int IsSameSubnet(uint32_t ip, uint32_t netIp, uint32_t netMask)
{
	if ((netIp & netMask) == (ip & netMask)) {
		/* on same subnet */
		return 1;
	}
	return 0;
	
}
static int IsPrivateAddr(uint32_t ip)
{
	uint8_t p1, p2, p3, p4;
	
	p1 = (uint8_t) (ip >> 24);
	p2 = (uint8_t)((ip >> 16) & 0x0ff);
	p3 = (uint8_t)((ip >> 8 ) & 0x0ff);
	p4 = (uint8_t) (ip & 0x0ff);
	
	
	if (GetWanIpaddr() != NULL &&
	    IsSameSubnet(ip, parseIpaddrstr(GetWanIpaddr()), parseIpaddrstr(GetWanNetmask()))) {
		return 0;
	}
	
	if (IsLoadBalanceMode() && GetSecWanIpaddr() != NULL) {
		if (IsSameSubnet(ip, parseIpaddrstr(GetSecWanIpaddr()), parseIpaddrstr(GetSecWanNetmask()))) {
			return 0;
		}
	}
	
	// 10.x.y.z
	if (p1 == 10)
		return 1;
	
	// 172.16.0.0 - 172.31.255.255
	if ((p1 == 172) && (p2 >= 16) && (p2 <= 31))
		return 1;
	
	// 192.168.0.0 - 192.168.255.255
	if ((p1 == 192) && (p2 == 168))
		return 1;
	
	return 0;
}
static int insert_lock_rule(char *wanType, int serType)
{
	FILE *fp;
	char chain[32];
	char cmd[64];
	
	if ((fp = fopen(PROTECT_SRV_RULE_FILE, "w")) == NULL) 
		return -1;
	
	snprintf(chain, sizeof(chain), "%s_%s", 
		 PROTECT_SRV_RULE_CHAIN, wanType);
	
	fprintf(fp, "*filter\n");
	
	fprintf(fp, "-A %s -p tcp --dport %d  -j DROP\n",
		chain, (serType == PROTECTION_SERVICE_SSH) ? GetSSHport() : 23);
	
	fprintf(fp, "COMMIT\n");
	fclose(fp);
	
	snprintf(cmd, sizeof(cmd), "iptables -F %s", chain);
	system(cmd);
	system("iptables-restore --noflush " PROTECT_SRV_RULE_FILE);
	
	MyDBG("Finish inser ruls to %s chain.\n", chain);
	
	return 0;
	
}
static void flush_rule(char *wanType, int serType)
{
	char chain[32];
	char cmd[64];
	
	snprintf(chain, sizeof(chain), "%s_%s", 
		 PROTECT_SRV_RULE_CHAIN, wanType);
	
	snprintf(cmd, sizeof(cmd), "iptables -D %s -p tcp --dport %d -j DROP",
		 chain, (serType == PROTECTION_SERVICE_SSH) ? GetSSHport() : 23);
	system(cmd);
	
	MyDBG("Flush %s chain of the %s rule.\n", chain, serType);
}
static void check_wanlan_lock()
{
	time_t now;
	
	if (RECV_SIG_FLAG) {
		
		RECV_SIG_FLAG = F_OFF;
		if (WAN_LOCK_SSH_FLAG) {
			insert_lock_rule("WAN", PROTECTION_SERVICE_SSH);
		}
		
		if (LAN_LOCK_SSH_FLAG) {
			insert_lock_rule("LAN", PROTECTION_SERVICE_SSH);
		} else if (LAN_LOCK_TELNET_FLAG) {
			insert_lock_rule("LAN", PROTECTION_SERVICE_TELNET);
		}
	}
	
	/* SSH */
	if (WAN_LOCK_SSH_FLAG) {
		if ((time(&now) - WAN_LOCK_SSH_T) >= PROTECTION_VALIDITY_TIME*60) {
			flush_rule("WAN", PROTECTION_SERVICE_SSH);
			WAN_LOCK_SSH_T = 0;
			WAN_LOCK_SSH_FLAG = F_OFF;
		}
	} else if (WAN_ERROR_SSH_CNT >= RETRY) {
		WAN_ERROR_SSH_CNT = 0;
		WAN_LOCK_SSH_FLAG = F_ON;
		WAN_LOCK_SSH_T = time(&now);
		insert_lock_rule("WAN", PROTECTION_SERVICE_SSH);
		
	}
	
	if (LAN_LOCK_SSH_FLAG) {
		if ((time(&now) - LAN_LOCK_SSH_T) >= PROTECTION_VALIDITY_TIME*60) {
			flush_rule("LAN", PROTECTION_SERVICE_SSH);
			LAN_LOCK_SSH_T = 0;
			LAN_LOCK_SSH_FLAG = F_OFF;
		}
	} else if (LAN_ERROR_SSH_CNT >= RETRY) {
		LAN_ERROR_SSH_CNT = 0;
		LAN_LOCK_SSH_FLAG = F_ON;
		LAN_LOCK_SSH_T = time(&now);
		insert_lock_rule("LAN", PROTECTION_SERVICE_SSH);
	}
	
	/* TELNET */
	if (LAN_LOCK_TELNET_FLAG) {
		if ((time(&now) - LAN_LOCK_TELNET_T) >= PROTECTION_VALIDITY_TIME*60) {
			flush_rule("LAN", PROTECTION_SERVICE_TELNET);
			LAN_LOCK_TELNET_T = 0;
			LAN_LOCK_TELNET_FLAG = F_OFF;
		}
	} else if (LAN_ERROR_TELNET_CNT >= RETRY) {
		LAN_ERROR_TELNET_CNT = 0;
		LAN_LOCK_TELNET_FLAG = F_ON;
		LAN_LOCK_TELNET_T = time(&now);
		insert_lock_rule("LAN", PROTECTION_SERVICE_TELNET);
	}
}

void receive_s(int newsockfd)
{

	PTCSRV_STATE_REPORT_T report;
	int n;
	
	bzero(&report,sizeof(PTCSRV_STATE_REPORT_T));
	
	n = read( newsockfd, &report, sizeof(PTCSRV_STATE_REPORT_T));
	if( n < 0 )
	{
	        MyDBG("ERROR reading from socket.\n");
	        return;
	}
	
	MyDBG("[receive report] addr:[%s] s_type:[%d] status:[%d] msg:[%s]\n",report.addr, report.s_type, report.status, report.msg);
	
	if (GetDebugValue(PROTECT_SRV_DEBUG)) {
		char info[200];
		sprintf(info, "echo \"[ProtectionSrv][receive report] addr:[%s] s_type:[%d] status:[%d] msg:[%s]\" >> %s",
			      report.addr, report.s_type, report.status, report.msg, PROTECT_SRV_LOG_FILE);
		system(info);
	}
	
	
	if (IsPrivateAddr(parseIpaddrstr(report.addr))) {
		
		if (report.s_type == PROTECTION_SERVICE_SSH) {
			if (report.status == RPT_FAIL) {
#ifdef RTCONFIG_NOTIFICATION_CENTER
				if ((LAN_ERROR_SSH_CNT + 1) == RETRY) {
					SEND_FAIL_LOGIN_EVENT(ADMIN_LOGIN_FAIL_SSH_EVENT, report.addr);
				}
#endif
				LAN_ERROR_SSH_CNT++;
			} else if (report.status == RPT_SUCCESS) {
				LAN_ERROR_SSH_CNT = 0;
				MyDBG("(LAN) SSH error count has been reset\n");
			}
		} else if (report.s_type == PROTECTION_SERVICE_TELNET) {
			if (report.status == RPT_FAIL) {
#ifdef RTCONFIG_NOTIFICATION_CENTER
				if ((LAN_ERROR_TELNET_CNT + 1) == RETRY) {
					SEND_FAIL_LOGIN_EVENT(ADMIN_LOGIN_FAIL_TELNET_EVENT, report.addr);
				}
#endif
				LAN_ERROR_TELNET_CNT++;
			} else if (report.status == RPT_SUCCESS) {
				LAN_ERROR_TELNET_CNT = 0;
				MyDBG("(LAN) TELNET error count has been reset\n");
			}
		}
		
	} else { /* WAN */
		
		if (report.s_type == PROTECTION_SERVICE_SSH) {
			if (report.status == RPT_FAIL) {
#ifdef RTCONFIG_NOTIFICATION_CENTER
				if ((WAN_ERROR_SSH_CNT + 1) == RETRY) {
					SEND_FAIL_LOGIN_EVENT(ADMIN_LOGIN_FAIL_SSH_EVENT, report.addr);
				}
#endif
				WAN_ERROR_SSH_CNT++;
			} else if (report.status == RPT_SUCCESS) {
				WAN_ERROR_SSH_CNT = 0;
				MyDBG("(WAN) SSH error count has been reset\n");
			}
		}
	}
	
}

static void start_local_socket(void)
{
	struct sockaddr_un addr;
	int sockfd, newsockfd;
	
	if ( (sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
		MyDBG("socket error\n");
		perror("socket error");
		exit(-1);
	}
	
	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, PROTECT_SRV_SOCKET_PATH, sizeof(addr.sun_path)-1);
	
	unlink(PROTECT_SRV_SOCKET_PATH);
	
	if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
		MyDBG("socket bind error\n");
		perror("socket bind error");
		exit(-1);
	}
	
	if (listen(sockfd, 3) == -1) {
		MyDBG("listen error\n");
		perror("listen error");
		exit(-1);
	}
	
	while (1) {
		if ( (newsockfd = accept(sockfd, NULL, NULL)) == -1) {
			MyDBG("accept error\n");
			perror("accept error");
			continue;
		}
		
		receive_s(newsockfd);
		close(newsockfd);
	}
}

static void local_socket_thread(void)
{
	pthread_t thread;
	pthread_attr_t attr;
	
	MyDBG("Start local socket thread.\n");
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&thread,NULL,(void *)&start_local_socket,NULL);
	pthread_attr_destroy(&attr);
}

void handlesignal(int signum)
{
	if (signum == SIGUSR1) {
		RECV_SIG_FLAG = F_ON;
	} else if (signum == SIGTERM) {
		terminated = 0;
	} else
		MyDBG("Unknown SIGNAL\n");
	
}

static void signal_register(void)
{
	struct sigaction sa;
	
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler =  &handlesignal;
	/* Handle Inser Firewall Rules via SIGUSR1 */
	sigaction(SIGUSR1, &sa, NULL);
	sigaction(SIGUSR2, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);    
}

int main(void)
{
	char cmd[20];
	int pid;
	
	MyDBG("[Start ProtectionSrv]\n");
	
	pid = getpid();
	sprintf(cmd,"echo %d > %s",pid, PROTECT_SRV_PID_PATH);
	system(cmd);
	
	/* Signal */
	signal_register();
	
	/* start unix socket */
	local_socket_thread();
	
	while (terminated) {
		check_wanlan_lock();
		sleep(1);
	}
	
	MyDBG("ProtectionSrv Terminated\n");
	
	return 0;
}
