/**
 * Marlin 3D Printer Firmware
 * Copyright (c) 2020 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
 *
 * Based on Sprinter and grbl.
 * Copyright (c) 2011 Camiel Gubbels / Erik van der Zalm
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */
#include "../../../../inc/MarlinConfigPre.h"

#if HAS_TFT_LVGL_UI

#include "../../../../MarlinCore.h"
#include "lv_conf.h"
#include "draw_ui.h"
#include "../../../../module/motion.h"
#include "../../../../gcode/queue.h"
#include "../../../../module/planner.h"
#include "../../../../libs/numtostr.h"
#include "../../../../module/temperature.h"
#include "../../../../feature/bltouch.h"
#include "../../../../feature/bedlevel/bedlevel.h"
#include "../../../../module/settings.h"
#include "../../../../module/probe.h"

#define ID_ALEVEL_RETURN 1
#define ID_ALEVEL_STOP 2

extern lv_group_t *g;
static lv_obj_t *scr;
static lv_obj_t *auto_level_info;
static lv_obj_t *labExtraTemp;
static float z_offset_back;
#if HAS_HEATED_BED
static lv_obj_t *labBedTemp;
#endif

static lv_obj_t *labPoint1, *labPoint2, *labPoint3, *labPoint4, *labPoint5, *labPoint6, *labPoint7, *labPoint8, *labPoint9;
static lv_obj_t *labPoint10, *labPoint11, *labPoint12, *labPoint13, *labPoint14, *labPoint15, *labPoint16;
static lv_obj_t *buttonStop, *labelStop, *buttonReturn, *labelReturn;

static lv_style_t level_style_point_pre;
static lv_style_t level_style_point_rel;

static lv_style_t level_style_startbtn_enable;
static lv_style_t level_style_stopbtn_enable;
static lv_style_t level_style_backbtn_enable;
static lv_style_t level_style_btn_disable;

static bool fDispPointFlash = 0;

static void draw_autolevel_bedtemp()
{
	char buf[20] = {0};

	public_buf_l[0] = '\0';
	strcat(public_buf_l, preheat_menu.hotbed);
	sprintf(buf, preheat_menu.value_state, (int)thermalManager.temp_bed.celsius, (int)thermalManager.temp_bed.target);
	strcat_P(public_buf_l, PSTR(": "));
	strcat(public_buf_l, buf);
	lv_label_set_text(labBedTemp, public_buf_l);
}

static void event_handler(lv_obj_t *obj, lv_event_t event)
{
	if (event == LV_EVENT_RELEASED)
	{
		switch (obj->mks_obj_id)
		{
		case ID_ALEVEL_RETURN:
			thermalManager.temp_hotend[uiCfg.curSprayerChoose].target = uiCfg.desireSprayerTempBak;
			thermalManager.start_watching_hotend(uiCfg.curSprayerChoose);
#if HAS_HEATED_BED
			thermalManager.temp_bed.target = uiCfg.desireBedTempBak;
			thermalManager.start_watching_bed();
#endif
			// restore leveling mode
			if (uiCfg.auto_leveling_point_status < leveling_home)
				auto_manu_level_sel = uiCfg.leveling_type_save;

			// restore probe z offset
			if (uiCfg.auto_leveling_done_flag == false)
				probe.offset.z = z_offset_back;

			clear_cur_ui();
			draw_return_ui();
			break;
		case ID_ALEVEL_STOP:
			if (uiCfg.auto_leveling_force_stop == false)
			{
				uiCfg.auto_leveling_force_stop = true;
				// queue.clear();
				// quickstop_stepper();
				// disable_all_steppers();
#if HAS_TFT_LVGL_UI
                uiCfg.ledSwitch = 2;
                bltouch._colorLedOn();
#endif
			}
			break;
		}
	}
}

void lv_flash_auto_level()
{
	if (uiCfg.auto_leveling_point_status == leveling_start)
		return;
	// button enable or disable select
	if (uiCfg.auto_leveling_point_status == leveling_stop || uiCfg.auto_leveling_point_status == leveling_heat || uiCfg.auto_leveling_point_status == leveling_saved)
	{
		lv_btn_set_style(buttonReturn, LV_BTN_STYLE_REL, &level_style_backbtn_enable);
		lv_obj_set_click(buttonReturn, 1);
	}
	else
	{
		lv_btn_set_style(buttonReturn, LV_BTN_STYLE_REL, &level_style_btn_disable);
		lv_obj_set_click(buttonReturn, 0);
	}

	if (uiCfg.auto_leveling_point_status > leveling_heat && uiCfg.auto_leveling_point_status < leveling_done)
	{
		lv_btn_set_style(buttonStop, LV_BTN_STYLE_REL, &level_style_stopbtn_enable);
		lv_obj_set_click(buttonStop, 1);
	}
	else
	{
		lv_btn_set_style(buttonStop, LV_BTN_STYLE_REL, &level_style_btn_disable);
		lv_obj_set_click(buttonStop, 0);
	}

	// leveling status control and display
	if (uiCfg.auto_leveling_point_status == leveling_heat)
	{
#ifdef EN_LEVELING_PREHEAT
		if (((abs((int)((int)thermalManager.temp_hotend[uiCfg.curSprayerChoose].celsius - BLTOUCH_HEAT_EXTRUDE_TEMP)) <= 3) /*|| ((int)thermalManager.temp_hotend[uiCfg.curSprayerChoose].celsius > BLTOUCH_HEAT_EXTRUDE_TEMP)*/)
#if HAS_HEATED_BED
			&& ((abs((int)((int)thermalManager.temp_bed.celsius - BLTOUCH_HEAT_BED_TEMP)) <= 1) || ((int)thermalManager.temp_bed.celsius > BLTOUCH_HEAT_BED_TEMP))
#endif
		)
#endif
		{
			uiCfg.auto_leveling_point_status = leveling_home;
			SERIAL_ECHO_MSG("Auto Leveling Home.");
			set_all_unhomed();
			bltouch._reset();
			queue.inject_P(PSTR("G28\nG29"));
		}
	}
	else
	{
		if (uiCfg.auto_leveling_force_stop)
		{
			if (uiCfg.auto_leveling_point_status == leveling_stop)
				lv_label_set_text(auto_level_info, leveling_menu.levelstopover);
			else
			{
				lv_label_set_text(auto_level_info, leveling_menu.levelforcestop);
                lv_label_set_style(labPoint1, LV_LABEL_STYLE_MAIN, &level_style_point_rel);
                lv_label_set_style(labPoint2, LV_LABEL_STYLE_MAIN, &level_style_point_rel);
                lv_label_set_style(labPoint3, LV_LABEL_STYLE_MAIN, &level_style_point_rel);
                lv_label_set_style(labPoint4, LV_LABEL_STYLE_MAIN, &level_style_point_rel);
                lv_label_set_style(labPoint5, LV_LABEL_STYLE_MAIN, &level_style_point_rel);
                lv_label_set_style(labPoint6, LV_LABEL_STYLE_MAIN, &level_style_point_rel);
                lv_label_set_style(labPoint7, LV_LABEL_STYLE_MAIN, &level_style_point_rel);
                lv_label_set_style(labPoint8, LV_LABEL_STYLE_MAIN, &level_style_point_rel);
                lv_label_set_style(labPoint9, LV_LABEL_STYLE_MAIN, &level_style_point_rel);
                lv_label_set_style(labPoint10, LV_LABEL_STYLE_MAIN, &level_style_point_rel);
                lv_label_set_style(labPoint11, LV_LABEL_STYLE_MAIN, &level_style_point_rel);
                lv_label_set_style(labPoint12, LV_LABEL_STYLE_MAIN, &level_style_point_rel);
                lv_label_set_style(labPoint13, LV_LABEL_STYLE_MAIN, &level_style_point_rel);
                lv_label_set_style(labPoint14, LV_LABEL_STYLE_MAIN, &level_style_point_rel);
                lv_label_set_style(labPoint15, LV_LABEL_STYLE_MAIN, &level_style_point_rel);
                lv_label_set_style(labPoint16, LV_LABEL_STYLE_MAIN, &level_style_point_rel);
			}
		}
		else
		{
			if (uiCfg.auto_leveling_point_status == leveling_home)
				lv_label_set_text(auto_level_info, leveling_menu.levelhome);
			else if (uiCfg.auto_leveling_point_status == leveling_done)
			{
				// constexpr float dpo[] = NOZZLE_TO_PROBE_OFFSET;
				// probe.offset.set(0, 0, dpo[Z_AXIS]);
				lv_label_set_text(auto_level_info, leveling_menu.leveldone);

				uiCfg.auto_leveling_point_num = leveling_point_end;
				auto_manu_level_sel = 1;
				uiCfg.leveling_type_save = auto_manu_level_sel;

				// settings.save(); //place "settings.save()" in the G29.cpp
				uiCfg.auto_leveling_point_status = leveling_saved;
				SERIAL_ECHO_MSG("Auto Leveling Save.");
				uiCfg.auto_leveling_done_flag = true;
	
#if HAS_TFT_LVGL_UI
                uiCfg.ledSwitch = 2;
                bltouch._colorLedOn();
#endif
                lv_label_set_style(labPoint1, LV_LABEL_STYLE_MAIN, &level_style_point_pre);
                lv_label_set_style(labPoint2, LV_LABEL_STYLE_MAIN, &level_style_point_pre);
                lv_label_set_style(labPoint3, LV_LABEL_STYLE_MAIN, &level_style_point_pre);
                lv_label_set_style(labPoint4, LV_LABEL_STYLE_MAIN, &level_style_point_pre);
                lv_label_set_style(labPoint5, LV_LABEL_STYLE_MAIN, &level_style_point_pre);
                lv_label_set_style(labPoint6, LV_LABEL_STYLE_MAIN, &level_style_point_pre);
                lv_label_set_style(labPoint7, LV_LABEL_STYLE_MAIN, &level_style_point_pre);
                lv_label_set_style(labPoint8, LV_LABEL_STYLE_MAIN, &level_style_point_pre);
                lv_label_set_style(labPoint9, LV_LABEL_STYLE_MAIN, &level_style_point_pre);
                lv_label_set_style(labPoint10, LV_LABEL_STYLE_MAIN, &level_style_point_pre);
                lv_label_set_style(labPoint11, LV_LABEL_STYLE_MAIN, &level_style_point_pre);
                lv_label_set_style(labPoint12, LV_LABEL_STYLE_MAIN, &level_style_point_pre);
                lv_label_set_style(labPoint13, LV_LABEL_STYLE_MAIN, &level_style_point_pre);
                lv_label_set_style(labPoint14, LV_LABEL_STYLE_MAIN, &level_style_point_pre);
                lv_label_set_style(labPoint15, LV_LABEL_STYLE_MAIN, &level_style_point_pre);
                lv_label_set_style(labPoint16, LV_LABEL_STYLE_MAIN, &level_style_point_pre);
			}

			if (uiCfg.auto_leveling_point_num > leveling_point_begin && uiCfg.auto_leveling_point_num < leveling_point_end)
			{
				lv_label_set_text(auto_level_info, leveling_menu.levelnext);

				if (uiCfg.auto_leveling_point_num == leveling_point_1)
				{
					if (fDispPointFlash)
                        lv_label_set_style(labPoint1, LV_LABEL_STYLE_MAIN, &level_style_point_pre);
					else
                        lv_label_set_style(labPoint1, LV_LABEL_STYLE_MAIN, &level_style_point_rel);
					fDispPointFlash = !fDispPointFlash;
                } else if(uiCfg.auto_leveling_point_num == leveling_point_2) {
                    lv_label_set_style(labPoint1, LV_LABEL_STYLE_MAIN, &level_style_point_pre);
                    if(fDispPointFlash)
                        lv_label_set_style(labPoint2, LV_LABEL_STYLE_MAIN, &level_style_point_pre);
                    else
                        lv_label_set_style(labPoint2, LV_LABEL_STYLE_MAIN, &level_style_point_rel);
                    fDispPointFlash = !fDispPointFlash;
                } else if(uiCfg.auto_leveling_point_num == leveling_point_3) {
                    lv_label_set_style(labPoint2, LV_LABEL_STYLE_MAIN, &level_style_point_pre);
                    if(fDispPointFlash)
                        lv_label_set_style(labPoint3, LV_LABEL_STYLE_MAIN, &level_style_point_pre);
                    else
                        lv_label_set_style(labPoint3, LV_LABEL_STYLE_MAIN, &level_style_point_rel);
                    fDispPointFlash = !fDispPointFlash;
                } else if(uiCfg.auto_leveling_point_num == leveling_point_4) {
                    lv_label_set_style(labPoint3, LV_LABEL_STYLE_MAIN, &level_style_point_pre);
                    if(fDispPointFlash)
                        lv_label_set_style(labPoint4, LV_LABEL_STYLE_MAIN, &level_style_point_pre);
                    else
                        lv_label_set_style(labPoint4, LV_LABEL_STYLE_MAIN, &level_style_point_rel);
                    fDispPointFlash = !fDispPointFlash;
                } else if(uiCfg.auto_leveling_point_num == leveling_point_5) {
                    lv_label_set_style(labPoint4, LV_LABEL_STYLE_MAIN, &level_style_point_pre);
                    if(fDispPointFlash)
                        lv_label_set_style(labPoint5, LV_LABEL_STYLE_MAIN, &level_style_point_pre);
                    else
                        lv_label_set_style(labPoint5, LV_LABEL_STYLE_MAIN, &level_style_point_rel);
                    fDispPointFlash = !fDispPointFlash;
                } else if(uiCfg.auto_leveling_point_num == leveling_point_6) {
                    lv_label_set_style(labPoint5, LV_LABEL_STYLE_MAIN, &level_style_point_pre);
                    if(fDispPointFlash)
                        lv_label_set_style(labPoint6, LV_LABEL_STYLE_MAIN, &level_style_point_pre);
                    else
                        lv_label_set_style(labPoint6, LV_LABEL_STYLE_MAIN, &level_style_point_rel);
                    fDispPointFlash = !fDispPointFlash;
                } else if(uiCfg.auto_leveling_point_num == leveling_point_7) {
                    lv_label_set_style(labPoint6, LV_LABEL_STYLE_MAIN, &level_style_point_pre);
                    if(fDispPointFlash)
                        lv_label_set_style(labPoint7, LV_LABEL_STYLE_MAIN, &level_style_point_pre);
                    else
                        lv_label_set_style(labPoint7, LV_LABEL_STYLE_MAIN, &level_style_point_rel);
                    fDispPointFlash = !fDispPointFlash;
                } else if(uiCfg.auto_leveling_point_num == leveling_point_8) {
                    lv_label_set_style(labPoint7, LV_LABEL_STYLE_MAIN, &level_style_point_pre);
                    if(fDispPointFlash)
                        lv_label_set_style(labPoint8, LV_LABEL_STYLE_MAIN, &level_style_point_pre);
                    else
                        lv_label_set_style(labPoint8, LV_LABEL_STYLE_MAIN, &level_style_point_rel);
                    fDispPointFlash = !fDispPointFlash;
                } else if(uiCfg.auto_leveling_point_num == leveling_point_9) {
                    lv_label_set_style(labPoint8, LV_LABEL_STYLE_MAIN, &level_style_point_pre);
                    if(fDispPointFlash)
                        lv_label_set_style(labPoint9, LV_LABEL_STYLE_MAIN, &level_style_point_pre);
                    else
                        lv_label_set_style(labPoint9, LV_LABEL_STYLE_MAIN, &level_style_point_rel);
                    fDispPointFlash = !fDispPointFlash;
                } else if(uiCfg.auto_leveling_point_num == leveling_point_10) {
                    lv_label_set_style(labPoint9, LV_LABEL_STYLE_MAIN, &level_style_point_pre);
                    if(fDispPointFlash)
                        lv_label_set_style(labPoint10, LV_LABEL_STYLE_MAIN, &level_style_point_pre);
                    else
                        lv_label_set_style(labPoint10, LV_LABEL_STYLE_MAIN, &level_style_point_rel);
                    fDispPointFlash = !fDispPointFlash;
                } else if(uiCfg.auto_leveling_point_num == leveling_point_11) {
                    lv_label_set_style(labPoint10, LV_LABEL_STYLE_MAIN, &level_style_point_pre);
                    if(fDispPointFlash)
                        lv_label_set_style(labPoint11, LV_LABEL_STYLE_MAIN, &level_style_point_pre);
                    else
                        lv_label_set_style(labPoint11, LV_LABEL_STYLE_MAIN, &level_style_point_rel);
                    fDispPointFlash = !fDispPointFlash;
                } else if(uiCfg.auto_leveling_point_num == leveling_point_12) {
                    lv_label_set_style(labPoint11, LV_LABEL_STYLE_MAIN, &level_style_point_pre);
                    if(fDispPointFlash)
                        lv_label_set_style(labPoint12, LV_LABEL_STYLE_MAIN, &level_style_point_pre);
                    else
                        lv_label_set_style(labPoint12, LV_LABEL_STYLE_MAIN, &level_style_point_rel);
                    fDispPointFlash = !fDispPointFlash;
                } else if(uiCfg.auto_leveling_point_num == leveling_point_13) {
                    lv_label_set_style(labPoint12, LV_LABEL_STYLE_MAIN, &level_style_point_pre);
                    if(fDispPointFlash)
                        lv_label_set_style(labPoint13, LV_LABEL_STYLE_MAIN, &level_style_point_pre);
                    else
                        lv_label_set_style(labPoint13, LV_LABEL_STYLE_MAIN, &level_style_point_rel);
                    fDispPointFlash = !fDispPointFlash;
                } else if(uiCfg.auto_leveling_point_num == leveling_point_14) {
                    lv_label_set_style(labPoint13, LV_LABEL_STYLE_MAIN, &level_style_point_pre);
                    if(fDispPointFlash)
                        lv_label_set_style(labPoint14, LV_LABEL_STYLE_MAIN, &level_style_point_pre);
                    else
                        lv_label_set_style(labPoint14, LV_LABEL_STYLE_MAIN, &level_style_point_rel);
                    fDispPointFlash = !fDispPointFlash;
                } else if(uiCfg.auto_leveling_point_num == leveling_point_15) {
                    lv_label_set_style(labPoint14, LV_LABEL_STYLE_MAIN, &level_style_point_pre);
                    if(fDispPointFlash)
                        lv_label_set_style(labPoint15, LV_LABEL_STYLE_MAIN, &level_style_point_pre);
                    else
                        lv_label_set_style(labPoint15, LV_LABEL_STYLE_MAIN, &level_style_point_rel);
                    fDispPointFlash = !fDispPointFlash;
                } else if(uiCfg.auto_leveling_point_num == leveling_point_16) {
                    lv_label_set_style(labPoint15, LV_LABEL_STYLE_MAIN, &level_style_point_pre);
                    if(fDispPointFlash)
                        lv_label_set_style(labPoint16, LV_LABEL_STYLE_MAIN, &level_style_point_pre);
                    else
                        lv_label_set_style(labPoint16, LV_LABEL_STYLE_MAIN, &level_style_point_rel);
                    fDispPointFlash = !fDispPointFlash;
                }
			}
		}
	}

	lv_draw_sprayer_temp(labExtraTemp);
#if HAS_HEATED_BED
	draw_autolevel_bedtemp();
#endif
}

void lv_draw_auto_level(void)
{
	if (disp_state_stack._disp_state[disp_state_stack._disp_index] != AUTOLEVELING_UI)
	{
		disp_state_stack._disp_index++;
		disp_state_stack._disp_state[disp_state_stack._disp_index] = AUTOLEVELING_UI;
	}
	disp_state = AUTOLEVELING_UI;

    scr = lv_obj_create(NULL, NULL);

    // static lv_style_t tool_style;
    lv_style_copy(&level_style_point_pre, &tft_style_label_pre);
    lv_style_copy(&level_style_point_rel, &tft_style_label_rel);
    level_style_point_rel.body.main_color = LV_COLOR_MAKE(0x80, 0x80, 0x80);
    level_style_point_rel.body.grad_color = LV_COLOR_MAKE(0x80, 0x80, 0x80);
    level_style_point_rel.text.line_space = 2;
    level_style_point_rel.body.padding.left = 5;
    level_style_point_rel.body.padding.right = 5;
    level_style_point_rel.body.padding.top = 8;
    level_style_point_rel.body.padding.bottom = 8;

    level_style_point_pre.body.main_color = LV_COLOR_GREEN;
    level_style_point_pre.body.grad_color = LV_COLOR_GREEN;
    level_style_point_pre.text.line_space = 2;
    level_style_point_pre.text.line_space = 2;
    level_style_point_pre.body.padding.left = 5;
    level_style_point_pre.body.padding.right = 5;
    level_style_point_pre.body.padding.top = 8;
    level_style_point_pre.body.padding.bottom = 8;

	lv_style_copy(&level_style_startbtn_enable, &style_para_back);
	level_style_startbtn_enable.body.main_color = LV_COLOR_MAKE(255, 77, 31);
	level_style_startbtn_enable.body.grad_color = LV_COLOR_MAKE(255, 77, 31);
	lv_style_copy(&level_style_stopbtn_enable, &style_para_back);
	level_style_stopbtn_enable.body.main_color = LV_COLOR_MAKE(255, 77, 31); //(0, 202, 228);
	level_style_stopbtn_enable.body.grad_color = LV_COLOR_MAKE(255, 77, 31); //(0, 202, 228);
	lv_style_copy(&level_style_backbtn_enable, &style_para_back);
	level_style_backbtn_enable.body.main_color = LV_COLOR_MAKE(255, 77, 31); //(117, 99, 199);
	level_style_backbtn_enable.body.grad_color = LV_COLOR_MAKE(255, 77, 31); //(117, 99, 199);
	lv_style_copy(&level_style_btn_disable, &style_para_back);
	level_style_btn_disable.body.main_color = LV_COLOR_MAKE(0x80, 0x80, 0x80);
	level_style_btn_disable.body.grad_color = LV_COLOR_MAKE(0x80, 0x80, 0x80);

    lv_obj_set_style(scr, &tft_style_scr);
    lv_scr_load(scr);
    lv_obj_clean(scr);

    lv_obj_t *title = lv_label_create(scr, NULL);
    lv_obj_set_style(title, &level_style_point_rel);
    lv_obj_set_pos(title, TITLE_XPOS, TITLE_YPOS);
    lv_label_set_text(title, creat_title_text());

    lv_refr_now(lv_refr_get_disp_refreshing());

    labPoint1 = lv_label_create(scr, NULL);
    lv_obj_set_pos(labPoint1, 166, 190);
    lv_obj_set_size(labPoint1, 30, 30);
    lv_label_set_text(labPoint1, " 1 ");
    lv_label_set_style(labPoint1, LV_LABEL_STYLE_MAIN, &level_style_point_rel);
    lv_label_set_body_draw(labPoint1, true);
    lv_label_set_align(labPoint1, LV_LABEL_ALIGN_CENTER);

    labPoint2 = lv_label_create(scr, NULL);
    lv_obj_set_pos(labPoint2, 120, 190);
    lv_obj_set_size(labPoint2, 30, 30);
    lv_label_set_text(labPoint2, " 2 ");
    lv_label_set_style(labPoint2, LV_LABEL_STYLE_MAIN, &level_style_point_rel);
    lv_label_set_body_draw(labPoint2, true);
    lv_label_set_align(labPoint2, LV_LABEL_ALIGN_CENTER);

    labPoint3 = lv_label_create(scr, NULL);
    lv_obj_set_pos(labPoint3, 74, 190);
    lv_obj_set_size(labPoint3, 30, 30);
    lv_label_set_text(labPoint3, " 3 ");
    lv_label_set_style(labPoint3, LV_LABEL_STYLE_MAIN, &level_style_point_rel);
    lv_label_set_body_draw(labPoint3, true);
    lv_label_set_align(labPoint3, LV_LABEL_ALIGN_CENTER);

    labPoint4 = lv_label_create(scr, NULL);
    lv_obj_set_pos(labPoint4, 28, 190);
    lv_obj_set_size(labPoint4, 30, 30);
    lv_label_set_text(labPoint4, " 4 ");
    lv_label_set_style(labPoint4, LV_LABEL_STYLE_MAIN, &level_style_point_rel);
    lv_label_set_body_draw(labPoint4, true);
    lv_label_set_align(labPoint4, LV_LABEL_ALIGN_CENTER);
////////////////////////////////////////////////////////////////////////////////////////
    labPoint5 = lv_label_create(scr, NULL);
    lv_obj_set_pos(labPoint5, 28, 150);
    lv_obj_set_size(labPoint5, 30, 30);
    lv_label_set_text(labPoint5, " 5 ");
    lv_label_set_style(labPoint5, LV_LABEL_STYLE_MAIN, &level_style_point_rel);
    lv_label_set_body_draw(labPoint5, true);
    lv_label_set_align(labPoint5, LV_LABEL_ALIGN_CENTER);

    labPoint6 = lv_label_create(scr, NULL);
    lv_obj_set_pos(labPoint6, 74, 150);
    lv_obj_set_size(labPoint6, 30, 30);
    lv_label_set_text(labPoint6, " 6 ");
    lv_label_set_style(labPoint6, LV_LABEL_STYLE_MAIN, &level_style_point_rel);
    lv_label_set_body_draw(labPoint6, true);
    lv_label_set_align(labPoint6, LV_LABEL_ALIGN_CENTER);

    labPoint7 = lv_label_create(scr, NULL);
    lv_obj_set_pos(labPoint7, 120, 150);
    lv_obj_set_size(labPoint7, 30, 30);
    lv_label_set_text(labPoint7, " 7 ");
    lv_label_set_style(labPoint7, LV_LABEL_STYLE_MAIN, &level_style_point_rel);
    lv_label_set_body_draw(labPoint7, true);
    lv_label_set_align(labPoint7, LV_LABEL_ALIGN_CENTER);

    labPoint8 = lv_label_create(scr, NULL);
    lv_obj_set_pos(labPoint8, 166, 150);
    lv_obj_set_size(labPoint8, 30, 30);
    lv_label_set_text(labPoint8, " 8 ");
    lv_label_set_style(labPoint8, LV_LABEL_STYLE_MAIN, &level_style_point_rel);
    lv_label_set_body_draw(labPoint8, true);
    lv_label_set_align(labPoint8, LV_LABEL_ALIGN_CENTER);
//////////////////////////////////////////////////////////////////////////////////////
    labPoint9 = lv_label_create(scr, NULL);
    lv_obj_set_pos(labPoint9, 166, 110);
    lv_obj_set_size(labPoint9, 30, 30);
    lv_label_set_text(labPoint9, " 9 ");
    lv_label_set_style(labPoint9, LV_LABEL_STYLE_MAIN, &level_style_point_rel);
    lv_label_set_body_draw(labPoint9, true);
    lv_label_set_align(labPoint9, LV_LABEL_ALIGN_CENTER);

    labPoint10 = lv_label_create(scr, NULL);
    lv_obj_set_pos(labPoint10, 120, 110);
    lv_obj_set_size(labPoint10, 30, 30);
    lv_label_set_text(labPoint10, "10 ");
    lv_label_set_style(labPoint10, LV_LABEL_STYLE_MAIN, &level_style_point_rel);
    lv_label_set_body_draw(labPoint10, true);
    lv_label_set_align(labPoint10, LV_LABEL_ALIGN_CENTER);

    labPoint11 = lv_label_create(scr, NULL);
    lv_obj_set_pos(labPoint11, 74, 110);
    lv_obj_set_size(labPoint11, 30, 30);
    lv_label_set_text(labPoint11, "11 ");
    lv_label_set_style(labPoint11, LV_LABEL_STYLE_MAIN, &level_style_point_rel);
    lv_label_set_body_draw(labPoint11, true);
    lv_label_set_align(labPoint11, LV_LABEL_ALIGN_CENTER);

    labPoint12 = lv_label_create(scr, NULL);
    lv_obj_set_pos(labPoint12, 28, 110);
    lv_obj_set_size(labPoint12, 30, 30);
    lv_label_set_text(labPoint12, "12 ");
    lv_label_set_style(labPoint12, LV_LABEL_STYLE_MAIN, &level_style_point_rel);
    lv_label_set_body_draw(labPoint12, true);
    lv_label_set_align(labPoint12, LV_LABEL_ALIGN_CENTER);
/////////////////////////////////////////////////////////////////////////////////////
    labPoint13 = lv_label_create(scr, NULL);
    lv_obj_set_pos(labPoint13, 28, 70);
    lv_obj_set_size(labPoint13, 30, 30);
    lv_label_set_text(labPoint13, "13 ");
    lv_label_set_style(labPoint13, LV_LABEL_STYLE_MAIN, &level_style_point_rel);
    lv_label_set_body_draw(labPoint13, true);
    lv_label_set_align(labPoint13, LV_LABEL_ALIGN_CENTER);

    labPoint14 = lv_label_create(scr, NULL);
    lv_obj_set_pos(labPoint14, 74, 70);
    lv_obj_set_size(labPoint14, 30, 30);
    lv_label_set_text(labPoint14, "14 ");
    lv_label_set_style(labPoint14, LV_LABEL_STYLE_MAIN, &level_style_point_rel);
    lv_label_set_body_draw(labPoint14, true);
    lv_label_set_align(labPoint14, LV_LABEL_ALIGN_CENTER);

    labPoint15 = lv_label_create(scr, NULL);
    lv_obj_set_pos(labPoint15, 120, 70);
    lv_obj_set_size(labPoint15, 30, 30);
    lv_label_set_text(labPoint15, "15 ");
    lv_label_set_style(labPoint15, LV_LABEL_STYLE_MAIN, &level_style_point_rel);
    lv_label_set_body_draw(labPoint15, true);
    lv_label_set_align(labPoint15, LV_LABEL_ALIGN_CENTER);

    labPoint16 = lv_label_create(scr, NULL);
    lv_obj_set_pos(labPoint16, 166, 70);
    lv_obj_set_size(labPoint16, 30, 30);
    lv_label_set_text(labPoint16, "16 ");
    lv_label_set_style(labPoint16, LV_LABEL_STYLE_MAIN, &level_style_point_rel);
    lv_label_set_body_draw(labPoint16, true);
    lv_label_set_align(labPoint16, LV_LABEL_ALIGN_CENTER);
///////////////////////////////////////////////////////////////////////////////////
	// Create image buttons
	buttonStop = lv_btn_create(scr, NULL);
	lv_obj_set_event_cb_mks(buttonStop, event_handler, ID_ALEVEL_STOP, NULL, 0);
	lv_btn_set_style(buttonStop, LV_BTN_STYLE_REL, &level_style_stopbtn_enable);
	lv_btn_set_style(buttonStop, LV_BTN_STYLE_PR, &level_style_stopbtn_enable);
	lv_obj_set_size(buttonStop, 225, 60);
	lv_obj_align(buttonStop, scr, LV_ALIGN_IN_BOTTOM_LEFT, 10, -5);
	labelStop = lv_label_create(buttonStop, NULL);

	buttonReturn = lv_btn_create(scr, NULL);
	lv_obj_set_event_cb_mks(buttonReturn, event_handler, ID_ALEVEL_RETURN, NULL, 0);
	lv_btn_set_style(buttonReturn, LV_BTN_STYLE_REL, &level_style_backbtn_enable);
	lv_btn_set_style(buttonReturn, LV_BTN_STYLE_PR, &level_style_backbtn_enable);
	lv_obj_set_size(buttonReturn, 225, 60);
	lv_obj_align(buttonReturn, scr, LV_ALIGN_IN_BOTTOM_RIGHT, -10, -5);
	labelReturn = lv_label_create(buttonReturn, NULL);

	auto_level_info = lv_label_create(scr, NULL);
	lv_label_set_long_mode(auto_level_info, LV_LABEL_LONG_BREAK);
	lv_obj_set_pos(auto_level_info, 240, 70);
	lv_obj_set_size(auto_level_info, 250, 0);
	lv_obj_set_style(auto_level_info, &style_yellow_label);
	lv_label_set_align(auto_level_info, LV_LABEL_ALIGN_LEFT);
	lv_label_set_text(auto_level_info, leveling_menu.autolevelstart);

	labExtraTemp = lv_label_create(scr, NULL);
    lv_obj_set_style(labExtraTemp, &level_style_point_rel);
	lv_obj_set_pos(labExtraTemp, 240, 140);
	lv_label_set_align(labExtraTemp, LV_LABEL_ALIGN_LEFT);

#if HAS_HEATED_BED
	labBedTemp = lv_label_create(scr, NULL);
    lv_obj_set_style(labBedTemp, &level_style_point_rel);
	lv_obj_set_pos(labBedTemp, 240, 180);
	lv_label_set_align(labBedTemp, LV_LABEL_ALIGN_LEFT);
#endif

	if (gCfgItems.multiple_language != 0)
	{
		lv_label_set_text(labelStop, print_file_dialog_menu.stop);
		lv_obj_align(labelStop, buttonStop, LV_ALIGN_IN_BOTTOM_MID, 0, BUTTON_TEXT_Y_OFFSET);

		lv_label_set_text(labelReturn, common_menu.text_back);
		lv_obj_align(labelReturn, buttonReturn, LV_ALIGN_IN_BOTTOM_MID, 0, BUTTON_TEXT_Y_OFFSET);
	}

	lv_draw_sprayer_temp(labExtraTemp);
#if HAS_HEATED_BED
	draw_autolevel_bedtemp();
#endif
	uiCfg.auto_leveling_point_num = leveling_point_begin;
	uiCfg.auto_leveling_point_status = leveling_start;
	lv_btn_set_style(buttonStop, LV_BTN_STYLE_REL, &level_style_btn_disable);
	lv_obj_set_click(buttonStop, 0);

	uiCfg.desireSprayerTempBak = thermalManager.temp_hotend[uiCfg.curSprayerChoose].target;
	uiCfg.desireBedTempBak = thermalManager.temp_bed.target;
	uiCfg.auto_leveling_force_stop = false;
	uiCfg.auto_leveling_done_flag = false;
	uiCfg.auto_leveling_point_num = leveling_point_begin;

	// backup probe z offset
	z_offset_back = probe.offset.z;
	probe.offset.z = 0;

#ifdef EN_LEVELING_PREHEAT
	thermalManager.temp_hotend[uiCfg.curSprayerChoose].target = BLTOUCH_HEAT_EXTRUDE_TEMP;
	thermalManager.start_watching_hotend(uiCfg.curSprayerChoose);
#if HAS_HEATED_BED
	thermalManager.temp_bed.target = BLTOUCH_HEAT_BED_TEMP;
	thermalManager.start_watching_bed();
#endif
#endif
	lv_label_set_text(auto_level_info, leveling_menu.level_heat);
	uiCfg.auto_leveling_point_num = leveling_point_begin;
	uiCfg.auto_leveling_point_status = leveling_heat;
	SERIAL_ECHO_MSG("Auto Leveling Heat.");
}

void lv_clear_auto_level()
{
    lv_obj_del(scr);
    if(disp_state_stack._disp_index > 0)
        disp_state_stack._disp_index--;
}

#endif // HAS_TFT_LVGL_UI
