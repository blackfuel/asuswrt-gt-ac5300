#!/bin/sh

wget_timeout=`nvram get apps_wget_timeout`
wget_options="-q -t 2 -T $wget_timeout --no-check-certificate"

nvram set sig_state_update=0 # INITIALIZING
nvram set sig_state_flag=0   # 0: Don't do upgrade  1: Do upgrade	
nvram set sig_state_error=0
#nvram set sig_state_url=""

IS_SUPPORT_NOTIFICATION_CENTER=`nvram get rc_support|grep -i nt_center`
if [ "$IS_SUPPORT_NOTIFICATION_CENTER" != "" ]; then
. /tmp/nc/event.conf
fi

# current signature information
current_sig_ver=`nvram get bwdpi_sig_ver`
current_sig_ver=`echo $current_sig_ver | sed s/'\.'//g;`
echo $current_sig_ver
echo $current_sig_ver >> /tmp/sig_upgrade.log

#get FULL / PARTIAL signature
dut_mem_size=`grep MemTotal /proc/meminfo | awk '{print $2}'`
if [ "$dut_mem_size" -lt "128000" ]; then
	sig_type="PART";
else
	sig_type="FULL";
fi

#for SQ test
forsq=`nvram get apps_sq`

# get signature information
wl0_support=`nvram show | grep rc_support | grep 2.4G`

if [ "$wl0_support" != "" ]; then
	country_code=`nvram get wl0_country_code`_$sig_type
else
	country_code=`nvram get wl1_country_code`_$sig_type
fi

if [ "$forsq" == "1" ]; then
	echo "---- sig update sq normal----" >> /tmp/sig_upgrade.log
	wget $wget_options https://dlcdnets.asus.com/pub/ASUS/LiveUpdate/Release/Wireless_SQ/sig2nd_update.zip -O /tmp/sig_update.txt		
else
	echo "---- sig update real normal----" >> /tmp/sig_upgrade.log
	wget $wget_options https://dlcdnets.asus.com/pub/ASUS/LiveUpdate/Release/Wireless/sig2nd_update.zip -O /tmp/sig_update.txt
fi	

if [ "$?" != "0" ]; then
	echo "Download info Failure"
	echo "Download info Failure" >> /tmp/sig_upgrade.log
	nvram set sig_state_error=1
else
	echo "Download info OK"
	echo "Download info OK" >> /tmp/sig_upgrade.log
	# TODO get and parse information
	sig_ver=`grep $country_code /tmp/sig_update.txt | sed s/.*#//;`
	echo $sig_ver
	if [ "$sig_ver" == "" ]; then
		WW_type="WW"_$sig_type
		sig_ver=`grep $WW_type /tmp/sig_update.txt | sed s/.*#//;`
		nvram set SKU="WW"_$sig_type;
	else
		nvram set SKU=$country_code;
	fi
	echo `nvram get SKU`
	echo `nvram get SKU` >> /tmp/sig_upgrade.log
	echo $sig_ver
	echo $sig_ver >> /tmp/sig_upgrade.log
	
	sig_ver=`echo $sig_ver | sed s/'\.'//g;`
	nvram set sig_state_info=${sig_ver}
	rm -f /tmp/sig_update.*
fi

update_sig_state_info=`nvram get sig_state_info`
last_sig_state_info=`nvram get sig_last_info`

echo "---- $sig_ver ---" >> /tmp/sig_upgrade.log
if [ "$sig_ver" == "" ]; then
	nvram set sig_state_error=1	# exist no Info
else
	if [ "$current_sig_ver" -lt "$sig_ver" ]; then
		echo "---- sig_ver: $sig_ver ----" >> /tmp/sig_upgrade.log
		nvram set sig_state_flag=1	# Do upgrade
		if [ "$IS_SUPPORT_NOTIFICATION_CENTER" != "" ]; then
			if [ "$last_sig_state_info" != "$update_sig_state_info" ]; then
						Notify_Event2NC "$SYS_NEW_SIGNATURE_UPDATED_EVENT" "{\"fw_ver\":\"$update_sig_state_info\"}"	#Send Event to Notification Center
						nvram set sig_last_info="$update_webs_state_info"
			fi
		fi
	fi	
fi

nvram set sig_state_update=1
