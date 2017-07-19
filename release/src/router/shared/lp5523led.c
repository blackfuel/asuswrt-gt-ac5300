#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <bcmnvram.h>
#include <shutils.h>
#include <shared.h>
#include "lp5523led.h"

#if defined(RTCONFIG_LP5523)

#define MALLOC_STRLEN 256
static const char lp55xx_path[] = "/sys/class/leds/blue1/device/";

//	Voltage of LEDS
struct lp55xx_leds_pattern lp55xx_leds_col[] = {
	{ LP55XX_ALL_LEDS_ON,	"ff", "ff", "ff" },
	{ LP55XX_ALL_LEDS_OFF,	"00", "00", "00" },
	{ LP55XX_ALL_BLEDS_ON,	"ff", "00", "00" },
	{ LP55XX_ALL_GLEDS_ON,	"00", "ff", "00" },
	{ LP55XX_ALL_RLEDS_ON,	"00", "00", "ff" },
	{ LP55XX_ALL_BREATH_LEDS,	"00", "00", "00" },
	{ LP55XX_ALL_DOWN_LEDS,	"00", "00", "00" },
	{ LP55XX_WHITE_LEDS,	"ff", "ff", "8c" },
	{ LP55XX_RED_LEDS,	"00", "00", "c8" },
	{ LP55XX_LIGHT_CYAN_LEDS,	"ff", "ff", "1e" },
	{ LP55XX_ORANGE_LEDS,	"00", "99", "ff" },
	{ LP55XX_PURPLE_LEDS,	"ff", "00", "5a" },
	{ LP55XX_NIAGARA_BLUE_LEDS,	"ff", "41", "0f" },
	{ LP55XX_PALE_DOGWOOD_LEDS,	"6e", "c3", "ff" },
	{ LP55XX_GREENERY_LEDS,	"41", "ff", "32" },
	{ LP55XX_PRIMROSE_YELLOW_LEDS,	"0a", "ff", "ff" },
	{ LP55XX_END_COLOR,	"", "", "" }
};

/* Behavior of LEDS
 * SBLINK = on 499.2ms + off 499.2ms
 * 3 ON 1 OFF = on 748.8ms + off 249.6ms
 *
 * */
struct lp55xx_leds_pattern lp55xx_leds_beh[] = {
	{ LP55XX_ACT_NONE,	"",				"", "" },
	{ LP55XX_ACT_SBLINK,	"7e00420040007e004200",		"", "" },
	{ LP55XX_ACT_3ON1OFF,	"7e00620040006000",		"", "" },
	{ LP55XX_ACT_BREATH,	"44084258420043584508",		"", "" },
	{ LP55XX_END_BLINK,	"", 				"", "" }
};

void split(char **arr, char *str, char *del) {
	char *s = strtok(str, del);

	while(s != NULL) {
		*arr++ = s;
		s = strtok(NULL, del);
	}
}

void pattern_combine(char *initial, char *color, char *behavior, int brightness, char *output)
{
	char *result=malloc(MALLOC_STRLEN);
	int color_val = (int)strtol(color, NULL, 16);
	memset(result, '\0', MALLOC_STRLEN);

	color_val = color_val * brightness / 100;
	sprintf(result, "%s%s%x%s", initial, color_val<16?"0":"", color_val, behavior);

	memcpy(output, result, strlen(result));

	free(result);
	return;
}

/* combine the str and save
 * @engine_index: select engine 1, 2, 3
 * @str_type: mode, load, leds
 * @value:
 * @return:
*/
void engine_combine_save(int engine_index, char *str_type, char *value)
{
	char *str_final = malloc(MALLOC_STRLEN);

	memset(str_final, '\0', MALLOC_STRLEN);
	sprintf(str_final, "%sengine%d_%s", lp55xx_path, engine_index, str_type);

	f_write_string( str_final, value, 0, 0);

	free(str_final);
	return;
}

void lp55xx_set_pattern_led(int ptc_mode, int ptb_mode)
{
	char *initial[3] = {"9d8040", "9d8044", "9d8048"};
	struct lp55xx_leds_pattern *blnk_leds_col = lp55xx_leds_col;
	struct lp55xx_leds_pattern *blnk_leds_beh = lp55xx_leds_beh;
	char tmp1[MALLOC_STRLEN];
	char tmp2[MALLOC_STRLEN];
	char tmp3[MALLOC_STRLEN];
	int brightness=0;
	int i=0, set=0;

	memset(tmp1, '\0', MALLOC_STRLEN);
	memset(tmp2, '\0', MALLOC_STRLEN);
	memset(tmp3, '\0', MALLOC_STRLEN);
	if (nvram_match("lp55xx_lp5523_sch_enable", "2"))
		brightness = nvram_get_int("lp55xx_lp5523_sch_brightness");
	else if (nvram_match("lp55xx_lp5523_user_enable", "1") && ptc_mode==LP55XX_LIGHT_CYAN_LEDS)
	{
		ptc_mode = nvram_get_int("lp55xx_lp5523_user_col");
		ptb_mode = nvram_get_int("lp55xx_lp5523_user_beh");
		brightness = nvram_get_int("lp55xx_lp5523_user_brightness");
	}
	else
		brightness = 100;

	if (ptc_mode==LP55XX_ALL_BREATH_LEDS) ptb_mode=LP55XX_ACT_BREATH;

	for (; blnk_leds_beh->ptn_mode!=LP55XX_END_BLINK; blnk_leds_beh++) {
		if (ptb_mode == blnk_leds_beh->ptn_mode){
			if (ptc_mode==LP55XX_MANUAL_COL)
			{
				char lp55xx_lp5523_manual[MALLOC_STRLEN];
				char *arr[3];

				memset(lp55xx_lp5523_manual, '\0', MALLOC_STRLEN);
				strcpy(lp55xx_lp5523_manual, nvram_safe_get("lp55xx_lp5523_manual"));
				split(arr, lp55xx_lp5523_manual, "_");

				pattern_combine(initial[i], arr[0], blnk_leds_beh->ptn1, brightness, tmp1);
				if (ptc_mode==LP55XX_ALL_BREATH_LEDS) i++;
				pattern_combine(initial[i], arr[1], strlen(blnk_leds_beh->ptn2)>0?blnk_leds_beh->ptn2:blnk_leds_beh->ptn1, brightness, tmp2);
				if (ptc_mode==LP55XX_ALL_BREATH_LEDS) i++;
				pattern_combine(initial[i], arr[2], strlen(blnk_leds_beh->ptn3)>0?blnk_leds_beh->ptn3:blnk_leds_beh->ptn1, brightness, tmp3);
			}
			else
			{
				for (; blnk_leds_col->ptn_mode!=LP55XX_END_COLOR; blnk_leds_col++) {
					if (ptc_mode == blnk_leds_col->ptn_mode){
						pattern_combine(initial[i], blnk_leds_col->ptn1, blnk_leds_beh->ptn1, brightness, tmp1);
						if (ptc_mode==LP55XX_ALL_BREATH_LEDS) i++;
						pattern_combine(initial[i], blnk_leds_col->ptn2, strlen(blnk_leds_beh->ptn2)>0?blnk_leds_beh->ptn2:blnk_leds_beh->ptn1, brightness, tmp2);
						if (ptc_mode==LP55XX_ALL_BREATH_LEDS) i++;
						pattern_combine(initial[i], blnk_leds_col->ptn3, strlen(blnk_leds_beh->ptn3)>0?blnk_leds_beh->ptn3:blnk_leds_beh->ptn1, brightness, tmp3);
					}
				}
			}

			set=1;
			break;
		}
	}

	if (set)
	{
		for (i = LP55XX_ENGINE_1; i <= LP55XX_ENGINE_3; i++) {
			if (i==LP55XX_ENGINE_1)
			{
				engine_combine_save(i, "load", tmp1);
				engine_combine_save(i, "leds", "111000000");
			}
			else if (i==LP55XX_ENGINE_2)
			{
				engine_combine_save(i, "load", tmp2);
				engine_combine_save(i, "leds", "000111000");
			}
			else
			{
				engine_combine_save(i, "load", tmp3);
				engine_combine_save(i, "leds", "000000111");
			}
		}
	}
}

void lp55xx_blink_leds(int ptc_mode, int ptb_mode)
{
	int i;

	for (i = LP55XX_ENGINE_1; i <= LP55XX_ENGINE_3; i++)
		engine_combine_save(i, "mode", "disabled");

	for (i = LP55XX_ENGINE_1; i <= LP55XX_ENGINE_3; i++)
		engine_combine_save(i, "mode", "load");

	lp55xx_set_pattern_led(ptc_mode, ptb_mode);

	for (i = LP55XX_ENGINE_1; i <= LP55XX_ENGINE_3; i++)
		engine_combine_save(i, "mode", "run");
}

void lp55xx_leds_proc(int ptc_mode, int ptb_mode)
{
	int ptc_mode_tmp = ptc_mode;
	int ptb_mode_tmp = ptb_mode;

	switch (ptb_mode_tmp)
	{
		case LP55XX_PREVIOUS_STATE:
		case LP55XX_WPS_FAIL:
			ptc_mode_tmp = nvram_get_int("lp55xx_lp5523_col");
			ptb_mode_tmp = nvram_get_int("lp55xx_lp5523_beh");
			break;
		case LP55XX_WPS_TRIG:
			ptc_mode_tmp = nvram_get_int("lp55xx_lp5523_col");
			ptb_mode_tmp = LP55XX_ACT_3ON1OFF;
			break;
		case LP55XX_WPS_SUCCESS:
			ptc_mode_tmp = nvram_get_int("lp55xx_lp5523_col");
			ptb_mode_tmp = LP55XX_ACT_SBLINK;
			break;
		case LP55XX_SCH_ENABLE:
			ptc_mode_tmp = nvram_get_int("lp55xx_lp5523_sch_col");
			ptb_mode_tmp = nvram_get_int("lp55xx_lp5523_sch_beh");
			break;
		default:
			nvram_set_int("lp55xx_lp5523_col", ptc_mode_tmp);
			nvram_set_int("lp55xx_lp5523_beh", ptb_mode_tmp);
			if (nvram_match("x_Setting", "1"))
				nvram_commit();
			break;
	}

	if (nvram_match("lp55xx_lp5523_sch_enable", "2"))
	{
		if (ptb_mode!=LP55XX_SCH_ENABLE)
			return;
	}

	lp55xx_blink_leds(LP55XX_ALL_LEDS_OFF, LP55XX_ACT_NONE);
	if (ptc_mode_tmp != LP55XX_ALL_LEDS_OFF)
		lp55xx_blink_leds(ptc_mode_tmp, ptb_mode_tmp);

	return;
}

void lp55xx_leds_sch(int start, int end)
{
	char time_tmp[128];
	int flag=0;
	int start_day=0, end_day=0;
	int start_time=0, end_time=0;
	int week, hour;

	memset(time_tmp, '\0', 128);

	for (week=0;week<7;week++) {
		for (hour=0;hour<24;hour++) {
			if (start>end) {
				if ((hour>=0 && hour<end) || (hour>=start && hour<24)) {
					if (flag == 0) {
						flag = 1;
						start_day = week;
						start_time = hour;
					}
				}
				else {
					if (flag == 1) {
						flag = 0;
						end_day = week;
						end_time = hour;

						sprintf(time_tmp, "%s%s%d%d%s%d%s%d", time_tmp, strlen(time_tmp)?"<":"", start_day, end_day, start_time<10?"0":"", start_time, end_time<10?"0":"", end_time);
					}
				}
			}
			else {
				if (hour>=start && hour<end) {
					if (flag == 0) {
						flag = 1;
						start_day = week;
						start_time = hour;
					}
				}
				else {
					if (flag == 1) {
						flag = 0;
						end_day = week;
						end_time = hour;

						sprintf(time_tmp, "%s%s%d%d%s%d%s%d", time_tmp, strlen(time_tmp)?"<":"", start_day, end_day, start_time<10?"0":"", start_time, end_time<10?"0":"", end_time);
					}
				}
			}
		}
	}

	if (flag == 1)
		sprintf(time_tmp, "%s%s%d%s%s%d%s", time_tmp, strlen(time_tmp)?"<":"", start_day, "0", start_time<10?"0":"", start_time, "00");

	nvram_set("lp55xx_lp5523_sch", time_tmp);

	return;
}

#endif	/* RTCONFIG_LP5523 */
