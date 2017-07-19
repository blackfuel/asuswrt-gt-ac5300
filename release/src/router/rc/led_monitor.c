#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <shared.h>
#include <rc.h>
#include <time.h>
#include <sys/time.h>

#define NORMAL_PERIOD           1               /* second */
#define DELAY_0			0		/* No flash */
#define DELAY_1S		1000000		/* 1 second */
#define DELAY_0_5S		500000		/* 0.5 second */
#define DELAY_0_25S		250000		/* 0.25 second */
#define DELAY_0_2S		200000		/* 0.2 second */
#define DELAY_0_1S		100000		/* 0.2 second */

#ifdef RPAC87
#define link_quality_level1  0
#define link_quality_level2  20
#define link_quality_level3  40
#define link_quality_level4  70
#endif

#ifdef RPAC66
#define link_quality_level1  0
#define link_quality_level2  70
#define link_quality_level3  80
#endif

#ifdef RPAC53
#define link_quality_level1  0
#define link_quality_level2  30
#define link_quality_level3  60
#endif

static enum LED_STATUS status;
static enum LED_STATUS pre_status;

#ifdef RPAC53 /* RP-AC53 */
#define LEDS_COUNT 4
led_state_t leds[LEDS_COUNT] = {
	[0] = {
		.id = LED_POWER
	},
	[1] = {
		.id = LED_2G
	},
	[2] = {
		.id = LED_5G
	},
	[3] = {
		.id = LED_LAN
	},
};
#else /* RP-AC66 */
#define LEDS_COUNT 3
led_state_t leds[LEDS_COUNT] = {
	[0] = {
		.id = LED_POWER
	},
	[1] = {
		.id = LED_2G
	},
	[2] = {
		.id = LED_5G
	},
};
#endif

static void update_led(led_state_t *led, int color, unsigned long flash_interval);

static struct itimerval itv;

static int sw_mode;

static void alarmtimer(unsigned long sec, unsigned long usec)
{
	itv.it_value.tv_sec  = sec;
        itv.it_value.tv_usec = usec;
        itv.it_interval = itv.it_value;
        setitimer(ITIMER_REAL, &itv, NULL);
}

static unsigned long get_us_timer()
{
	return itv.it_value.tv_usec + itv.it_value.tv_sec * 1000000;
}

static void update_interval_timer()
{
	int i = 0;
	int update_flag = 0;
	int no_flash_flag = 1;

	while (i < LEDS_COUNT){
		if (leds[i].flash_interval > 0 && leds[i].flash_interval < get_us_timer()) {
			itv.it_value.tv_sec  = leds[i].flash_interval / 1000000;
		        itv.it_value.tv_usec = leds[i].flash_interval % 1000000;
			update_flag = 1;
		}
		else if (leds[i].flash_interval > 0)
			no_flash_flag = 0;
		i++;
	}

	if (no_flash_flag) {
		alarmtimer(NORMAL_PERIOD, 0);
	}
	else if (update_flag) {
		itv.it_interval = itv.it_value;
		setitimer(ITIMER_REAL, &itv, NULL);
	}
}

static void led_flash(led_state_t *led)
{
	if (led->next_switch_time <= get_us_timer()) {
		if (led->state == LED_ON) { // ON
			set_off_led(led);
		}
		else {	//OFF
			set_on_led(led);
		}
		led->next_switch_time = led->flash_interval;
	}
	else
		led->next_switch_time -= get_us_timer();
}

enum WiFi_QUALITY {
	WIFI_2G_HIGH_QUALITY,
	WIFI_2G_MEDIUM_QUALITY,
	WIFI_2G_MEDIUM2_QUALITY,
	WIFI_2G_LOW_QUALITY,
	WIFI_2G_DOWN,
	WIFI_5G_HIGH_QUALITY,
	WIFI_5G_MEDIUM_QUALITY,
	WIFI_5G_MEDIUM2_QUALITY,
	WIFI_5G_LOW_QUALITY,
	WIFI_5G_DOWN
};

enum {
	GW_LINK_DOWN = 0,
	GW_LINK_UP
};

static void detect_eth_link()
{
	static int p_gw_status = -1;
	static int skip_time = 3000000;
	/* Stop detect conn link */
	if (sw_mode != SW_MODE_AP
#ifdef RTCONFIG_REALTEK
		|| mediabridge_mode()
#endif
	) // Only for AP mode
		return;

	if (nvram_get_int("wps_cli_state") == 1) // WPS processing...
		return;

	if (status == LED_BOOTING || status == LED_WPS_PROCESSING || status == LED_WPS_RESTART_WL)
		return;

	if (status == LED_BOOTED_APMODE && skip_time > 0) { // Special case. Wait for eth ready.
		skip_time -= get_us_timer();
		return;
	}
	/* Stop detect coon link end */

	int gw_status = GW_LINK_DOWN;
	if (nvram_get_int("dnsqmode") == 1)
		gw_status = GW_LINK_UP;

	if (gw_status != p_gw_status) {
		if (gw_status == GW_LINK_DOWN) {
#ifdef RPAC53
			update_led(&leds[3], LED_GREEN, DELAY_0_5S);
			update_gpiomode(14, 1);
#else
			update_led(&leds[0], LED_GREEN, DELAY_0_5S);
#endif
		}
		else {
#ifdef RPAC53
			update_led(&leds[3], LED_GREEN, DELAY_0);
			update_gpiomode(14, 0);
#else
			update_led(&leds[0], LED_GREEN, DELAY_0);
#endif
		}
		p_gw_status = gw_status;
	}
}

static void detect_conn_link_quality()
{
	/* Stop detect conn link */
	if (sw_mode != SW_MODE_REPEATER
#ifdef RTCONFIG_REALTEK
		&& !mediabridge_mode()
#endif
	   ) // Only for repeater mode
		return;

	if (nvram_get_int("wps_cli_state") == 1 || nvram_get_int("restore_defaults") == 1) // WPS processing and Restore defaults
		return;

	if (status == LED_BOOTING || status == LED_WPS_PROCESSING || status == LED_WPS_RESTART_WL || status == LED_RESTART_WL || status == LED_FIRMWARE_UPGRADE)
		return;
	/* Stop detect coon link end */


	static int link_quality_2g = WIFI_2G_DOWN, link_quality_5g = WIFI_5G_DOWN;
	int wlc0_state = 0, wlc1_state = 0;
	int link_quality = 0;

	if (wlc0_state = nvram_get_int("wlc0_state") == WLC_STATE_CONNECTED) {
		link_quality = get_conn_link_quality(0);
#ifdef RPAC87
		if (link_quality >= link_quality_level4) {
			if (link_quality_2g != WIFI_2G_HIGH_QUALITY) {
				update_led(&leds[1], LED_SL4, DELAY_0);
				link_quality_2g = WIFI_2G_HIGH_QUALITY;
			}
		}
		else if	(link_quality >= link_quality_level3)
#else
		if (link_quality >= link_quality_level3)
#endif
		{
			if (link_quality_2g != WIFI_2G_MEDIUM2_QUALITY) {
#ifdef RPAC87
				update_led(&leds[1], LED_SL3, DELAY_0);
#endif
#if defined(RPAC66) || defined(RPAC53)
				update_led(&leds[1], LED_GREEN, DELAY_0);
#endif

				link_quality_2g = WIFI_2G_MEDIUM2_QUALITY;

			}
		}
		else if (link_quality >= link_quality_level2) {
			if (link_quality_2g != WIFI_2G_MEDIUM_QUALITY) {
#ifdef RPAC87
				update_led(&leds[1], LED_SL2, DELAY_0);
#endif
#if defined(RPAC66) || defined(RPAC53)
				update_led(&leds[1], LED_ORANGE, DELAY_0);
#endif
				link_quality_2g = WIFI_2G_MEDIUM_QUALITY;
			}
		}
		else if (link_quality > link_quality_level1) {
			if (link_quality_2g != WIFI_2G_LOW_QUALITY) {
#ifdef RPAC87
				update_led(&leds[1], LED_SL1, DELAY_0);
#endif
#if defined(RPAC66) || defined(RPAC53)
				update_led(&leds[1], LED_RED, DELAY_0);
#endif
				link_quality_2g = WIFI_2G_LOW_QUALITY;
			}
		}
		else {
			if (link_quality_2g != WIFI_2G_DOWN) {
				update_led(&leds[1], LED_NONE, DELAY_0);
				link_quality_2g = WIFI_2G_DOWN;
			}
		}
	}
	else {
		if (link_quality_2g != WIFI_2G_DOWN) {
			update_led(&leds[1], LED_NONE, DELAY_0);
			link_quality_2g = WIFI_2G_DOWN;
		}
	}

	if (wlc1_state = nvram_get_int("wlc1_state") == WLC_STATE_CONNECTED) {
		link_quality = get_conn_link_quality(1);
#ifdef RPAC87
		if (link_quality >= link_quality_level4) {
			if (link_quality_5g != WIFI_5G_HIGH_QUALITY) {
				update_led(&leds[2], LED_SL4, DELAY_0);
				link_quality_5g = WIFI_5G_HIGH_QUALITY;
			}
		}
		else if (link_quality >= link_quality_level3)
#else
		if (link_quality >= link_quality_level3)
#endif
		{
			if (link_quality_5g != WIFI_5G_MEDIUM2_QUALITY) {
#if defined(RPAC87) || defined(RPAC53)
				update_led(&leds[2], LED_SL3, DELAY_0);
#endif
#if defined(RPAC66) || defined(RPAC53)
				update_led(&leds[2], LED_GREEN, DELAY_0);
#endif
				link_quality_5g = WIFI_5G_MEDIUM2_QUALITY;
			}
		}
		else if (link_quality >= link_quality_level2) {
			if (link_quality_5g != WIFI_5G_MEDIUM_QUALITY) {
#ifdef RPAC87
				update_led(&leds[2], LED_SL2, DELAY_0);

#endif
#if defined(RPAC66) || defined(RPAC53)
				update_led(&leds[2], LED_ORANGE, DELAY_0);
#endif
				link_quality_5g = WIFI_5G_MEDIUM_QUALITY;
			}
		}
		else if (link_quality > link_quality_level1) {
			if (link_quality_5g != WIFI_5G_LOW_QUALITY) {
#ifdef RPAC87
				update_led(&leds[2], LED_SL1, DELAY_0);
#endif
#if defined(RPAC66) || defined(RPAC53)
				update_led(&leds[2], LED_RED, DELAY_0);
#endif
				link_quality_5g = WIFI_5G_LOW_QUALITY;
			}
		}
		else {
			if (link_quality_5g != WIFI_5G_DOWN) {
				update_led(&leds[2], LED_NONE, DELAY_0);
				link_quality_5g = WIFI_5G_DOWN;
			}
		}
	}
	else {
		if (link_quality_5g != WIFI_5G_DOWN) {
			update_led(&leds[2], LED_NONE, DELAY_0);
			link_quality_5g = WIFI_5G_DOWN;
		}
	}
}

static void process_leds()
{
	int count = 0;
	while (count < LEDS_COUNT) {

		if (leds[count].changed) {

			/* Set off the LED's GPIO */
			set_off_led(&leds[count]);

			/* Set On the LED's GPIO */

			if (leds[count].flash_interval == 0 && leds[count].color != LED_NONE) //ON
				set_on_led(&leds[count]);

			leds[count].changed = 0;
		}

		if (leds[count].flash_interval > 0)
			led_flash(&leds[count]);
		count++;
	}
}

static void process_status(int sig)
{

	status = nvram_get_int("led_status");

	if (status != pre_status) {
		switch (status) {

			case LED_BOOTING:

				update_led(&leds[0], LED_GREEN, DELAY_1S);
#ifdef RPAC87
				update_led(&leds[1], ALL_LED, DELAY_1S);
				update_led(&leds[2], ALL_LED, DELAY_1S);
#endif
#ifdef RPAC66
				update_led(&leds[1], LED_GREEN, DELAY_1S);
				update_led(&leds[2], LED_GREEN, DELAY_1S);
#endif
#ifdef RPAC53
				update_led(&leds[0], LED_GREEN, DELAY_1S);
				update_led(&leds[1], LED_GREEN, DELAY_1S);
				update_led(&leds[2], LED_GREEN, DELAY_1S);
				update_led(&leds[3], LED_GREEN, DELAY_1S);
#endif
				break;

			case LED_BOOTED:
				update_led(&leds[0], LED_GREEN, DELAY_0);
				update_led(&leds[1], LED_NONE, DELAY_0);
				update_led(&leds[2], LED_NONE, DELAY_0);
#ifdef RPAC53
				update_led(&leds[3], LED_NONE, DELAY_0);
				update_gpiomode(14, 0);
#endif

				break;

			case LED_BOOTED_APMODE:

				update_led(&leds[0], LED_GREEN, DELAY_0);
#if defined(RPAC53)
				update_led(&leds[3], LED_GREEN, DELAY_0);
#endif
				/* check the status of radio for 2G */
#ifdef RPAC87
				update_led(&leds[1], (nvram_get_int("wl0_radio") ? ALL_LED : LED_NONE), DELAY_0);
#endif
#if defined(RPAC66) || defined(RPAC53)
				update_led(&leds[1], (nvram_get_int("wl0_radio") ? LED_GREEN : LED_NONE), DELAY_0);
#endif
#ifdef RTCONFIG_HAS_5G
#ifdef RPAC87
				update_led(&leds[2], (nvram_get_int("wl1_radio") ? ALL_LED : LED_NONE), DELAY_0);
#endif
#if defined(RPAC66) || defined(RPAC53)
				/* check the status of radio for 5G */
				update_led(&leds[2], (nvram_get_int("wl1_radio") ? LED_GREEN : LED_NONE), DELAY_0);
#endif
#endif

				break;
			case LED_WIFI_2G_DOWN:

				update_led(&leds[1], LED_NONE, DELAY_0);

				break;
			case LED_WIFI_5G_DOWN:

				update_led(&leds[2], LED_NONE, DELAY_0);

				break;
			case LED_WPS_START:
				update_led(&leds[0], LED_GREEN, DELAY_0_5S);
#ifdef RPAC87
				update_led(&leds[1], ALL_LED, DELAY_0_5S);
				update_led(&leds[2], ALL_LED, DELAY_0_5S);
#endif
#if defined(RPAC66) || defined(RPAC53)
				update_led(&leds[1], LED_GREEN, DELAY_0_5S);
				update_led(&leds[2], LED_GREEN, DELAY_0_5S);
#endif
				break;
			case LED_WPS_SCANNING:
				update_led(&leds[0], LED_GREEN, DELAY_0_5S);
#ifdef RPAC87
				update_led(&leds[1], ALL_LED, DELAY_0_5S);
				update_led(&leds[2], ALL_LED, DELAY_0_5S);
#endif
#if defined(RPAC66) || defined(RPAC53)
				update_led(&leds[1], LED_GREEN, DELAY_0_5S);
				update_led(&leds[2], LED_GREEN, DELAY_0_5S);
#endif
				break;
			case LED_WPS_2G_SCANNING:
				update_led(&leds[0], LED_GREEN, DELAY_0_5S);
#ifdef RPAC87
				update_led(&leds[1], ALL_LED, DELAY_0_5S);
				update_led(&leds[2], ALL_LED, DELAY_0_5S);
#endif
#if defined(RPAC66) || defined(RPAC53)
				update_led(&leds[1], LED_GREEN, DELAY_0_5S);
				update_led(&leds[2], LED_GREEN, DELAY_0_5S);
#endif
				break;
			case LED_WPS_5G_SCANNING:
				update_led(&leds[0], LED_GREEN, DELAY_0_5S);
#ifdef RPAC87
				update_led(&leds[1], ALL_LED, DELAY_0_5S);
				update_led(&leds[2], ALL_LED, DELAY_0_5S);
#endif
#if defined(RPAC66) || defined(RPAC53)
				update_led(&leds[1], LED_GREEN, DELAY_0_5S);
				update_led(&leds[2], LED_GREEN, DELAY_0_5S);
#endif
				break;
			case LED_WPS_FAIL:

				if (sw_mode == SW_MODE_AP) {
					/* Check gw_status */
					if (nvram_get_int("dnsqmode") == 1) {
#ifdef RPAC53
						update_led(&leds[3], LED_GREEN, DELAY_0);
						update_gpiomode(14, 0);
#else
						update_led(&leds[0], LED_GREEN, DELAY_0);
#endif
					}
					else {
#ifdef RPAC53
						update_led(&leds[3], LED_GREEN, DELAY_0_5S);
						update_gpiomode(14, 1);
#else
						update_led(&leds[0], LED_GREEN, DELAY_0_5S);
#endif
					}
#ifdef RPAC87
					update_led(&leds[1], (nvram_get_int("wl0_radio") ? ALL_LED : LED_NONE), DELAY_0);
#endif
#if defined(RPAC66) || defined(RPAC53)
					update_led(&leds[1], LED_GREEN, DELAY_0);
#endif
#ifdef RTCONFIG_HAS_5G
#ifdef RPAC87
					update_led(&leds[2], (nvram_get_int("wl1_radio") ? ALL_LED : LED_NONE), DELAY_0);
#endif
#if defined(RPAC66) || defined(RPAC53)

					update_led(&leds[2], LED_GREEN, DELAY_0);
#endif
#endif
				}
				else {
					update_led(&leds[0], LED_GREEN, DELAY_0);
					update_led(&leds[1], LED_NONE, DELAY_0);
					update_led(&leds[2], LED_NONE, DELAY_0);
				}

				break;
			case LED_WPS_PROCESSING:

				update_led(&leds[0], LED_GREEN, DELAY_0_2S);
#ifdef RPAC87
				update_led(&leds[1], ALL_LED, DELAY_0_2S);
				update_led(&leds[2], ALL_LED, DELAY_0_2S);
#endif
#if defined(RPAC66) || defined(RPAC53)
				update_led(&leds[1], LED_GREEN, DELAY_0_2S);
				update_led(&leds[2], LED_GREEN, DELAY_0_2S);
#endif
				break;
			case LED_WPS_RESTART_WL:

				update_led(&leds[0], LED_GREEN, DELAY_0_5S);
#ifdef RPAC87
				update_led(&leds[1], ALL_LED, DELAY_0_5S);
				update_led(&leds[2], ALL_LED, DELAY_0_5S);
#endif
#if defined(RPAC66) || defined(RPAC53)
				update_led(&leds[1], LED_GREEN, DELAY_0_5S);
				update_led(&leds[2], LED_GREEN, DELAY_0_5S);
#endif
				break;
			case LED_RESTART_WL:

				update_led(&leds[0], LED_GREEN, DELAY_0_5S);
#ifdef RPAC87
				update_led(&leds[1], ALL_LED, DELAY_0_5S);
				update_led(&leds[2], ALL_LED, DELAY_0_5S);
#endif
#if defined(RPAC66) || defined(RPAC53)
				update_led(&leds[1], LED_GREEN, DELAY_0_5S);
				update_led(&leds[2], LED_GREEN, DELAY_0_5S);
#endif
				break;
			case LED_RESTART_WL_DONE:

				if (access_point_mode()) {
					/* Check gw_status */
					if (nvram_get_int("dnsqmode") == 1)
						update_led(&leds[0], LED_GREEN, DELAY_0);
					else
						update_led(&leds[0], LED_GREEN, DELAY_0_5S);

					/* check the status of radio for 2G */
#ifdef RPAC87
					update_led(&leds[1], (nvram_get_int("wl0_radio") ? ALL_LED : LED_NONE), DELAY_0);
#endif
#if defined(RPAC66) || defined(RPAC53)
					update_led(&leds[1], (nvram_get_int("wl0_radio") ? LED_GREEN : LED_NONE), DELAY_0);
#endif
#ifdef RTCONFIG_HAS_5G
					/* check the status of radio for 5G */
#ifdef RPAC87
					update_led(&leds[2], (nvram_get_int("wl1_radio") ? ALL_LED : LED_NONE), DELAY_0);
#endif
#if defined(RPAC66) || defined(RPAC53)
					update_led(&leds[2], (nvram_get_int("wl1_radio") ? LED_GREEN : LED_NONE), DELAY_0);
#endif
#endif
				}
				else {
					update_led(&leds[0], LED_GREEN, DELAY_0);
					update_led(&leds[1], LED_NONE, DELAY_0);
					update_led(&leds[2], LED_NONE, DELAY_0);
				}

				break;
			case LED_FIRMWARE_UPGRADE:

#ifdef RPAC87
				update_led(&leds[0], LED_GREEN, DELAY_1S);
				update_led(&leds[1], ALL_LED, DELAY_1S);
				update_led(&leds[2], ALL_LED, DELAY_1S);
#endif
#ifdef RPAC66
				update_led(&leds[0], LED_GREEN, DELAY_1S);
				update_led(&leds[1], LED_GREEN, DELAY_1S);
				update_led(&leds[2], LED_GREEN, DELAY_1S);
#endif
#if defined(RPAC53)
				update_led(&leds[0], LED_RED, DELAY_1S);
				update_led(&leds[1], LED_NONE, DELAY_0);
				update_led(&leds[2], LED_NONE, DELAY_0);
				update_led(&leds[3], LED_NONE, DELAY_0);
				update_gpiomode(14, 1);
#endif
				break;
			case LED_FACTORY_RESET:

#ifdef RPAC87
				update_led(&leds[0], LED_GREEN, DELAY_1S);
				update_led(&leds[1], ALL_LED, DELAY_1S);
				update_led(&leds[2], ALL_LED, DELAY_1S);
#endif
#ifdef RPAC66
				update_led(&leds[0], LED_GREEN, DELAY_1S);
				update_led(&leds[1], LED_GREEN, DELAY_1S);
				update_led(&leds[2], LED_GREEN, DELAY_1S);
#endif
#if defined(RPAC53)
				update_led(&leds[0], LED_GREEN, DELAY_1S);
				update_led(&leds[1], LED_GREEN, DELAY_1S);
				update_led(&leds[2], LED_GREEN, DELAY_1S);
				update_led(&leds[3], LED_GREEN, DELAY_1S);
				update_gpiomode(14, 1);
#endif
				break;
			case LED_AP_WPS_START:
#ifdef RPAC87
				update_led(&leds[0], LED_GREEN, DELAY_0);
				update_led(&leds[1], ALL_LED, DELAY_1S);
				update_led(&leds[2], ALL_LED, DELAY_1S);
#endif
#ifdef RPAC53
				update_led(&leds[0], LED_GREEN, DELAY_0);
				update_led(&leds[1], LED_GREEN, DELAY_0_2S);
				update_led(&leds[2], LED_GREEN, DELAY_0_2S);

#endif
				break;
			default:
				TRACE_PT("status error.\n");
		}
		pre_status = status;

		update_interval_timer();
	}
	process_leds();
	detect_conn_link_quality(); // Check and change WiFi LED in Real time mode.
	detect_eth_link(); // Check and change Power LED in Real time mode.

}

static void led_monitor_exit(int sig)
{
	remove("/var/run/led_monitor.pid");
	exit(0);
}

static void update_led(led_state_t *led, int color, unsigned long flash_interval)
{
	led->color = color;
	led->flash_interval = flash_interval;
	led->next_switch_time = 0;
	led->changed = 1;
}

int
led_monitor_main(int argc, char *argv[])
{
	FILE *fp;
	sigset_t sigs_to_catch;

	sw_mode = sw_mode();

	if ((fp=fopen("/var/run/led_monitor.pid", "w")) != NULL)
	{
		fprintf(fp, "%d", getpid());
		fclose(fp);
	}

	sigemptyset(&sigs_to_catch);
	sigaddset(&sigs_to_catch, SIGTERM);
	sigaddset(&sigs_to_catch, SIGALRM);
	sigprocmask(SIG_UNBLOCK, &sigs_to_catch, NULL);
	signal(SIGTERM, led_monitor_exit);
	signal(SIGALRM, process_status);

	pre_status = -1;

	alarmtimer(NORMAL_PERIOD, 0);

	while (1)
	{
		pause();
	}

	return 0;
}
