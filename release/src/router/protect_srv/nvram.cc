#include <stdio.h>
#include <stdlib.h>
#include <ptcsrv_nvram.h>

int IsLoadBalanceMode(void)
{
#ifdef ASUSWRT_SDK /* ASUSWRT SDK */
	if (nvram_match("wans_mode", "lb"))
		return 1;
#else /* DSL_ASUSWRT SDK */
#endif
	return 0;
}

char *GetWanIpaddr(void)
{
#ifdef ASUSWRT_SDK /* ASUSWRT SDK */
	char *wan_ipaddr;
	char prefix[16], tmp[100];
	
	sprintf(prefix, "wan%d_", wan_primary_ifunit());
	wan_ipaddr = nvram_safe_get(strcat_r(prefix, "ipaddr", tmp));
	
	if (!strcmp(wan_ipaddr, "0.0.0.0"))
		return NULL;
	
	return wan_ipaddr;
	
#else /* DSL_ASUSWRT SDK */
#endif
}

char *GetWanNetmask(void)
{
#ifdef ASUSWRT_SDK /* ASUSWRT SDK */
	char *wan_netmask;
	char prefix[16], tmp[100];
	
	sprintf(prefix, "wan%d_", wan_primary_ifunit());
	wan_netmask = nvram_safe_get(strcat_r(prefix, "netmask", tmp));
	
	if (!strcmp(wan_netmask, "0.0.0.0"))
		return NULL;
	
	return wan_netmask;
	
#else /* DSL_ASUSWRT SDK */
#endif
}


char *GetSecWanIpaddr(void)
{
	if (!IsLoadBalanceMode())
		return NULL;
#ifdef ASUSWRT_SDK /* ASUSWRT SDK */
	char *wan_ipaddr;
	char prefix[16], tmp[100];
	
 #if defined(RTCONFIG_DUALWAN) || defined(RTCONFIG_USB_MODEM)
	sprintf(prefix, "wan%d_", WAN_UNIT_SECOND);
	wan_ipaddr = nvram_safe_get(strcat_r(prefix, "ipaddr", tmp));
	
	if (!strcmp(wan_ipaddr, "0.0.0.0"))
		return NULL;
	
	return wan_ipaddr;
 #endif
	return NULL;

#else /* DSL_ASUSWRT SDK */
#endif
}

char *GetSecWanNetmask(void)
{
#ifdef ASUSWRT_SDK /* ASUSWRT SDK */
	char *wan_netmask;
	char prefix[16], tmp[100];
	
 #if defined(RTCONFIG_DUALWAN) || defined(RTCONFIG_USB_MODEM)
	sprintf(prefix, "wan%d_", WAN_UNIT_SECOND);
	wan_netmask = nvram_safe_get(strcat_r(prefix, "netmask", tmp));
	
	if (!strcmp(wan_netmask, "0.0.0.0"))
		return NULL;
	
	return wan_netmask;
 #endif
	return NULL;
	
#else /* DSL_ASUSWRT SDK */
#endif
}

int GetSSHport(void)
{
#ifdef ASUSWRT_SDK /* ASUSWRT SDK */
	return nvram_get_int("sshd_port") ? : 22;
#else /* DSL_ASUSWRT SDK */
#endif
}

