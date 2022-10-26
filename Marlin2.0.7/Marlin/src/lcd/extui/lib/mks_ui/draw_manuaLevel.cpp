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
#include "../../../../module/probe.h"
#include "../../../../feature/bltouch.h"
#include "../../../../feature/bedlevel/mbl/mesh_bed_leveling.h"
#include "../../../../feature/bedlevel/bedlevel.h"

#define ID_MLEVEL_ADD 1
#define ID_MLEVEL_DEC 2
#define ID_MLEVEL_CFM 3
#define ID_MLEVEL_RETURN 4
#define ID_MLEVEL_STEP 5
#define ID_MLEVEL_INIT	6

extern lv_group_t *g;
static lv_obj_t *scr;
static lv_obj_t *level_point_info, *leve_hight_value; 
static lv_obj_t *buttonStep, *labelStep, *buttonCfm;
static lv_obj_t *labBedTemp, *labExtraTemp;
static int8_t z_offset_value;
static bool flg_leveling_start, flg_confirm_opt;
static uint8_t flg_flash_id, z_offset_step;
static uint8_t manual_leveling_index;
static float z_offset_dist = MESH_EDIT_Z_STEP;
static float z_offset_back;

static void disp_move_step()
{
	if (z_offset_step == 1)
	{
		z_offset_dist = 0.01;
		lv_imgbtn_set_src(buttonStep, LV_BTN_STATE_REL, "F:/bmp_step_move0_1.bin");
		lv_imgbtn_set_src(buttonStep, LV_BTN_STATE_PR, "F:/bmp_clear.bin");
		if (gCfgItems.multiple_language != 0)
		{
			lv_label_set_text(labelStep, "0.01mm");
			lv_obj_align(labelStep, buttonStep, LV_ALIGN_IN_BOTTOM_MID, 0, BUTTON_TEXT_Y_OFFSET);
		}
	}
	else if (z_offset_step == 0)
	{
		z_offset_dist = 0.05;
		lv_imgbtn_set_src(buttonStep, LV_BTN_STATE_REL, "F:/bmp_step_move1.bin");
		lv_imgbtn_set_src(buttonStep, LV_BTN_STATE_PR, "F:/bmp_clear.bin");
		if (gCfgItems.multiple_language != 0)
		{
			lv_label_set_text(labelStep, "0.05mm");
			lv_obj_align(labelStep, buttonStep, LV_ALIGN_IN_BOTTOM_MID, 0, BUTTON_TEXT_Y_OFFSET);
		}
	}
	else if (z_offset_step == 2)
	{
		z_offset_dist = 0.5;
		lv_imgbtn_set_src(buttonStep, LV_BTN_STATE_REL, "F:/bmp_step_move10.bin");
		lv_imgbtn_set_src(buttonStep, LV_BTN_STATE_PR, "F:/bmp_clear.bin");
		if (gCfgItems.multiple_language != 0)
		{
			lv_label_set_text(labelStep, "0.5mm");
			lv_obj_align(labelStep, buttonStep, LV_ALIGN_IN_BOTTOM_MID, 0, BUTTON_TEXT_Y_OFFSET);
		}
	}
}

static void draw_current_zoffset(bool only_disp)
{
	if (only_disp == 0)
	{
		float z = current_position.z + float(z_offset_value * z_offset_dist);
		if (z > LCD_PROBE_Z_RANGE / 2)
			z = LCD_PROBE_Z_RANGE / 2;
		else if (z < -LCD_PROBE_Z_RANGE / 2)
			z = -LCD_PROBE_Z_RANGE / 2;
		current_position.z = constrain(z, -(LCD_PROBE_Z_RANGE)*0.5f, (LCD_PROBE_Z_RANGE)*0.5f);
		line_to_current_position(manual_feedrate_mm_s.z);
		feedrate_mm_s = 30.0; //return movespeed
	}

	const float v = current_position.z;
	ZERO(public_buf_l);
	sprintf_P(public_buf_l, PSTR("%s %s"), leveling_menu.manuallevelz, ftostr43sign(v + (v < 0 ? -0.0001f : 0.0001f), '+'));
	lv_label_set_text(leve_hight_value, public_buf_l);
}

static void event_handler(lv_obj_t *obj, lv_event_t event)
{
	if (event == LV_EVENT_RELEASED)
	{
		switch (obj->mks_obj_id)
		{
		case ID_MLEVEL_ADD:
		case ID_MLEVEL_DEC:
			if (lvgl_wait_for_move == 0 && flg_leveling_start == 0 
				&& uiCfg.manu_leveling_done_flag == 0)
			{
				if (obj->mks_obj_id == ID_MLEVEL_ADD)
					z_offset_value = 1;
				else
					z_offset_value = -1;
				
				if(uiCfg.manu_leveling_begin_flg) draw_current_zoffset(0);	
			}
			break;
		case ID_MLEVEL_CFM:
			if (lvgl_wait_for_move == 0 && flg_leveling_start == 0)
			{
				flg_confirm_opt = true;
				uiCfg.manu_leveling_begin_flg = true;
				
				if (manual_leveling_index < GRID_MAX_POINTS)
				{
					ZERO(public_buf_l);
					sprintf_P(public_buf_l, PSTR("%s %i/%u"), leveling_menu.levelnext, int(manual_leveling_index + 1), GRID_MAX_POINTS);
					lv_label_set_text(level_point_info, public_buf_l);
				}
			}
			break;
		case ID_MLEVEL_RETURN:
			if (lvgl_wait_for_move == 0 || uiCfg.manu_leveling_done_flag == true)
			{
			#if 0//def EN_LEVELING_PREHEAT
				thermalManager.temp_hotend[uiCfg.curSprayerChoose].target = uiCfg.desireSprayerTempBak;
				thermalManager.start_watching_hotend(uiCfg.curSprayerChoose);

#if HAS_HEATED_BED
				thermalManager.temp_bed.target = uiCfg.desireBedTempBak;
				thermalManager.start_watching_bed();
#endif
			#endif

				#if HAS_TFT_LVGL_UI
				uiCfg.ledSwitch = 2;
				bltouch._colorLedOn();
				#endif	

				// restore leveling mode
				if(uiCfg.manu_leveling_begin_flg == false)
					auto_manu_level_sel = uiCfg.leveling_type_save;

				//restore probe z offset
				if (uiCfg.manu_leveling_done_flag == false)
				{
					probe.offset.z = z_offset_back;	
					leveling_data_restore(0);
					auto_manu_level_sel = uiCfg.leveling_type_save;
				}

				if(leveling_is_valid()){
					planner.leveling_active = true;
					set_bed_leveling_enabled(true);
				}
				
				lv_clear_manual_level();
				draw_return_ui();
			}
			break;
		case ID_MLEVEL_STEP:
			z_offset_step++;
			z_offset_step %= 3;
			disp_move_step();		
			break;
		}
	}
}

void lv_flash_manual_level()
{
	if (uiCfg.manu_leveling_heat_flg)
	{
	#if 0//def EN_LEVELING_PREHEAT
		if (((abs((int)((int)thermalManager.temp_hotend[uiCfg.curSprayerChoose].celsius - BLTOUCH_HEAT_EXTRUDE_TEMP)) <= 3) /*|| ((int)thermalManager.temp_hotend[uiCfg.curSprayerChoose].celsius > BLTOUCH_HEAT_EXTRUDE_TEMP)*/)
			#if HAS_HEATED_BED
			&& ((abs((int)((int)thermalManager.temp_bed.celsius - BLTOUCH_HEAT_BED_TEMP)) <= 1) || ((int)thermalManager.temp_bed.celsius > BLTOUCH_HEAT_BED_TEMP))
			#endif
		)
	#endif	
		{
			uiCfg.manu_leveling_heat_flg = 0;
			set_all_unhomed();
			lv_label_set_text(level_point_info, leveling_menu.levelhome);
			queue.inject_P(PSTR("G28"));
		}
	}
	else
	{
		if (flg_leveling_start)
		{
			if (all_axes_homed())
			{
				flg_leveling_start = false;
				lv_label_set_text(level_point_info, leveling_menu.manuallevelwait);
			}
		}

		if (flg_confirm_opt)
		{
			flg_confirm_opt = false;
				
			if (manual_leveling_index < GRID_MAX_POINTS)
			{
				lvgl_wait_for_move = true;
				queue.inject_P(manual_leveling_index ? PSTR("G29 S2") : PSTR("G29 S1"));
				flg_flash_id = 1; // zoffset value information
			}
			else if(manual_leveling_index == GRID_MAX_POINTS)
			{
				//update probe z offset
			  //probe.offset.set(0, 0, 0);
			  auto_manu_level_sel = 0;
				uiCfg.leveling_type_save = 0;
				uiCfg.manu_leveling_done_flag = true;
				lvgl_wait_for_move = true;
				queue.inject_P(PSTR("G29 S2\nG28"));
				flg_flash_id = 0; // leveling done information
				lv_label_set_text(level_point_info, leveling_menu.leveldone);
				lv_imgbtn_set_src(buttonCfm, LV_BTN_STATE_REL, "F:/bmp_confirm_dis.bin");
				lv_obj_set_click(buttonCfm, 0);
			}
			else
				manual_leveling_index--;
			manual_leveling_index++;
		}

		if (lvgl_wait_for_move == false)
		{
			if (flg_flash_id == 1)
			{
				draw_current_zoffset(1);
				flg_flash_id = 0;
			}
		}
	}

	lv_draw_sprayer_temp(labExtraTemp);
	lv_draw_bed_temp(labBedTemp);
}

void lv_draw_manual_level(void)
{
	lv_obj_t *buttonAdd, *buttonDec, *buttonBack;

	if (disp_state_stack._disp_state[disp_state_stack._disp_index] != LEVELING_UI)
	{
		disp_state_stack._disp_index++;
		disp_state_stack._disp_state[disp_state_stack._disp_index] = LEVELING_UI;
	}
	disp_state = LEVELING_UI;

	scr = lv_obj_create(NULL, NULL);

	// static lv_style_t tool_style;

	lv_obj_set_style(scr, &tft_style_scr);
	lv_scr_load(scr);
	lv_obj_clean(scr);

	lv_obj_t *title = lv_label_create(scr, NULL);
	lv_obj_set_style(title, &tft_style_label_rel);
	lv_obj_set_pos(title, TITLE_XPOS, TITLE_YPOS);
	lv_label_set_text(title, creat_title_text());

	lv_refr_now(lv_refr_get_disp_refreshing());

	buttonStep = lv_imgbtn_create(scr, NULL);
	buttonAdd = lv_imgbtn_create(scr, NULL);
	buttonDec = lv_imgbtn_create(scr, NULL);
	buttonCfm = lv_imgbtn_create(scr, NULL);
	buttonBack = lv_imgbtn_create(scr, NULL);

	// Create image buttons
	lv_obj_set_event_cb_mks(buttonAdd, event_handler, ID_MLEVEL_ADD, NULL, 0);
	lv_imgbtn_set_src(buttonAdd, LV_BTN_STATE_REL, "F:/bmp_zAdd.bin");
	lv_imgbtn_set_src(buttonAdd, LV_BTN_STATE_PR, "F:/bmp_clear.bin");
	lv_imgbtn_set_style(buttonAdd, LV_BTN_STATE_PR, &tft_style_label_pre);
	lv_imgbtn_set_style(buttonAdd, LV_BTN_STATE_REL, &tft_style_label_rel);
	lv_obj_clear_protect(buttonAdd, LV_PROTECT_FOLLOW);

	lv_obj_set_event_cb_mks(buttonDec, event_handler, ID_MLEVEL_DEC, NULL, 0);
	lv_imgbtn_set_src(buttonDec, LV_BTN_STATE_REL, "F:/bmp_zDec.bin");
	lv_imgbtn_set_src(buttonDec, LV_BTN_STATE_PR, "F:/bmp_clear.bin");
	lv_imgbtn_set_style(buttonDec, LV_BTN_STATE_PR, &tft_style_label_pre);
	lv_imgbtn_set_style(buttonDec, LV_BTN_STATE_REL, &tft_style_label_rel);

	lv_obj_set_event_cb_mks(buttonCfm, event_handler, ID_MLEVEL_CFM, NULL, 0);
	lv_imgbtn_set_src(buttonCfm, LV_BTN_STATE_REL, "F:/bmp_confirm.bin");
	lv_imgbtn_set_src(buttonCfm, LV_BTN_STATE_PR, "F:/bmp_confirm_dis.bin");
	lv_imgbtn_set_style(buttonCfm, LV_BTN_STATE_PR, &tft_style_label_pre);
	lv_imgbtn_set_style(buttonCfm, LV_BTN_STATE_REL, &tft_style_label_rel);

	lv_obj_set_event_cb_mks(buttonBack, event_handler, ID_MLEVEL_RETURN, NULL, 0);
	lv_imgbtn_set_src(buttonBack, LV_BTN_STATE_REL, "F:/bmp_return.bin");
	lv_imgbtn_set_src(buttonBack, LV_BTN_STATE_PR, "F:/bmp_clear.bin");
	lv_imgbtn_set_style(buttonBack, LV_BTN_STATE_PR, &tft_style_label_pre);
	lv_imgbtn_set_style(buttonBack, LV_BTN_STATE_REL, &tft_style_label_rel);

	lv_obj_set_event_cb_mks(buttonStep, event_handler, ID_MLEVEL_STEP, NULL, 0);
	lv_imgbtn_set_style(buttonStep, LV_BTN_STATE_PR, &tft_style_label_pre);
	lv_imgbtn_set_style(buttonStep, LV_BTN_STATE_REL, &tft_style_label_rel);

	lv_obj_set_pos(buttonAdd, BTN_ROW2_COL2_POS);
	lv_obj_set_pos(buttonDec, BTN_ROW2_COL1_POS);
	lv_obj_set_pos(buttonCfm, BTN_ROW1_COL4_POS);
	lv_obj_set_pos(buttonStep, BTN_ROW1_COL1_POS);
	lv_obj_set_pos(buttonBack, BTN_ROW2_COL4_POS);

	// Create labels on the image buttons
	lv_btn_set_layout(buttonAdd, LV_LAYOUT_OFF);
	lv_btn_set_layout(buttonDec, LV_LAYOUT_OFF);
	lv_btn_set_layout(buttonCfm, LV_LAYOUT_OFF);
	lv_btn_set_layout(buttonBack, LV_LAYOUT_OFF);
	lv_btn_set_layout(buttonStep, LV_LAYOUT_OFF);

	lv_obj_t *labelAdd = lv_label_create(buttonAdd, NULL);
	lv_obj_t *labelDec = lv_label_create(buttonDec, NULL);
	lv_obj_t *labelCfm = lv_label_create(buttonCfm, NULL);
	lv_obj_t *labelBack = lv_label_create(buttonBack, NULL);
	labelStep = lv_label_create(buttonStep, NULL);

	if (gCfgItems.multiple_language != 0)
	{
		lv_label_set_text(labelAdd, move_menu.z_add);
		lv_obj_align(labelAdd, buttonAdd, LV_ALIGN_IN_BOTTOM_MID, 0, BUTTON_TEXT_Y_OFFSET);

		lv_label_set_text(labelDec, move_menu.z_dec);
		lv_obj_align(labelDec, buttonDec, LV_ALIGN_IN_BOTTOM_MID, 0, BUTTON_TEXT_Y_OFFSET);

		lv_label_set_text(labelCfm, common_menu.dialog_confirm_title);
		lv_obj_align(labelCfm, buttonCfm, LV_ALIGN_IN_BOTTOM_MID, 0, BUTTON_TEXT_Y_OFFSET);

		lv_label_set_text(labelBack, common_menu.text_back);
		lv_obj_align(labelBack, buttonBack, LV_ALIGN_IN_BOTTOM_MID, 0, BUTTON_TEXT_Y_OFFSET);
	}

#if HAS_ROTARY_ENCODER
	if (gCfgItems.encoder_enable)
	{
		lv_group_add_obj(g, buttonAdd);
		lv_group_add_obj(g, buttonDec);
		lv_group_add_obj(g, buttonCfm);
		lv_group_add_obj(g, buttonBack);
		lv_group_add_obj(g, buttonStep);
	}
#endif
	labExtraTemp = lv_label_create(scr, NULL);
	lv_label_set_long_mode(labExtraTemp, LV_LABEL_LONG_BREAK);
	lv_obj_set_style(labExtraTemp, &tft_style_label_rel);
	lv_obj_set_size(labExtraTemp, 240, 0);
	lv_label_set_align(labExtraTemp, LV_LABEL_ALIGN_CENTER);
	lv_obj_align(labExtraTemp, NULL, LV_ALIGN_IN_TOP_MID, 0, 100);

#if HAS_HEATED_BED
	labBedTemp = lv_label_create(scr, NULL);
	lv_label_set_long_mode(labBedTemp, LV_LABEL_LONG_BREAK);
	lv_obj_set_style(labBedTemp, &tft_style_label_rel);
	lv_obj_set_size(labBedTemp, 240, 0);
	lv_label_set_align(labBedTemp, LV_LABEL_ALIGN_CENTER);
	lv_obj_align(labBedTemp, NULL, LV_ALIGN_IN_TOP_MID, 0, 130);
#endif

	level_point_info = lv_label_create(scr, NULL);
	lv_label_set_long_mode(level_point_info, LV_LABEL_LONG_BREAK);
	lv_obj_set_size(level_point_info, 250, 0);
	lv_obj_set_style(level_point_info, &style_yellow_label);
	lv_label_set_align(level_point_info, LV_LABEL_ALIGN_CENTER);
	lv_obj_align(level_point_info, NULL, LV_ALIGN_IN_TOP_MID, 0, 40);
	lv_label_set_text(level_point_info, leveling_menu.levelhome);

	leve_hight_value = lv_label_create(scr, NULL);
	lv_label_set_long_mode(leve_hight_value, LV_LABEL_LONG_BREAK);
	lv_obj_set_size(leve_hight_value, 240, 0);
	lv_obj_set_style(leve_hight_value, &style_yellow_label);
	lv_label_set_align(leve_hight_value, LV_LABEL_ALIGN_CENTER);
	lv_obj_align(leve_hight_value, NULL, LV_ALIGN_IN_TOP_MID, 0, 70);
	lv_label_set_text(leve_hight_value, " ");

	lv_draw_sprayer_temp(labExtraTemp);
	lv_draw_bed_temp(labBedTemp);
	z_offset_step = 2;
	disp_move_step();

	uiCfg.desireSprayerTempBak = thermalManager.temp_hotend[uiCfg.curSprayerChoose].target;
	uiCfg.desireBedTempBak = thermalManager.temp_bed.target;

	#if 0//def EN_LEVELING_PREHEAT
		thermalManager.temp_hotend[uiCfg.curSprayerChoose].target = BLTOUCH_HEAT_EXTRUDE_TEMP;
		thermalManager.start_watching_hotend(uiCfg.curSprayerChoose);
		#if HAS_HEATED_BED
		thermalManager.temp_bed.target = BLTOUCH_HEAT_BED_TEMP;
		thermalManager.start_watching_bed();
		#endif
		lv_label_set_text(level_point_info, leveling_menu.level_heat);
	#else
		lv_label_set_text(level_point_info, leveling_menu.levelhome);
	#endif
	
	uiCfg.manu_leveling_heat_flg = 1;
	flg_leveling_start = true;
	flg_confirm_opt = false;
	flg_flash_id = false;
	manual_leveling_index = 0;
	z_offset_value = 0;
	
	uiCfg.manu_leveling_begin_flg = false;
	uiCfg.manu_leveling_done_flag = false;
	
	//backup probe z offset
	z_offset_back = probe.offset.z;
	probe.offset.z = 0;

	#if HAS_TFT_LVGL_UI
	uiCfg.ledSwitch = 1;
	bltouch._whiteLedOn();
	#endif	
}

void lv_clear_manual_level()
{
#if HAS_ROTARY_ENCODER
	if (gCfgItems.encoder_enable)
		lv_group_remove_all_objs(g);
#endif
	lv_obj_del(scr);
}

#endif // HAS_TFT_LVGL_UI
