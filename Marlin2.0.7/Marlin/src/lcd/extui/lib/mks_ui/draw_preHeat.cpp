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

#include "lv_conf.h"
#include "draw_ui.h"

#include "../../../../MarlinCore.h"
#include "../../../../module/temperature.h"

static lv_obj_t * scr;
extern lv_group_t*  g;

#define ID_P_RETURN 	1

static lv_obj_t *hotendTempText;
#if HAS_HEATED_BED
static lv_obj_t *bedTempText;
#endif
static lv_obj_t *preHotInfo;
uint8_t preHotCurOpt = ID_P_COOL;

static void draw_operation_type()
{
	if(preHotCurOpt == ID_P_PLA)
		sprintf(public_buf_l, "%s%s", preheat_menu.operation, preheat_menu.pla);
	else if(preHotCurOpt == ID_P_BAS)
		sprintf(public_buf_l, "%s%s", preheat_menu.operation, preheat_menu.bas);
	else if(preHotCurOpt == ID_P_CUS)
		sprintf(public_buf_l, "%s%s", preheat_menu.operation, preheat_menu.custom);
	else 
		sprintf(public_buf_l, "%s%s", preheat_menu.operation, preheat_menu.off);
	lv_label_set_text(preHotInfo, public_buf_l);
}

static void event_handler(lv_obj_t * obj, lv_event_t event) {
  switch (obj->mks_obj_id) {
		case ID_P_COOL:
			if (event == LV_EVENT_CLICKED) {
        // nothing to do
      }
      else if (event == LV_EVENT_RELEASED) {
				set_preheat_temp(0);	
				preHotCurOpt = ID_P_COOL;
      }
			break;
		case ID_P_PLA:
			if (event == LV_EVENT_CLICKED) {
        // nothing to do
      }
      else if (event == LV_EVENT_RELEASED) {
				set_preheat_temp(1);	
				preHotCurOpt = ID_P_PLA;
      }
			break;
		case ID_P_BAS:
			if (event == LV_EVENT_CLICKED) {
        // nothing to do
      }
      else if (event == LV_EVENT_RELEASED) {
				set_preheat_temp(2);	
				preHotCurOpt = ID_P_BAS;
      }
			break;
		case ID_P_CUS:
			if (event == LV_EVENT_CLICKED) {
        // nothing to do
      }
      else if (event == LV_EVENT_RELEASED) {
				clear_cur_ui();
        lv_draw_handHeat();	
      }
			break;
    case ID_P_RETURN:
      if (event == LV_EVENT_CLICKED) {
        // nothing to do
      }
      else if (event == LV_EVENT_RELEASED) {
        clear_cur_ui();
        draw_return_ui();
      }
      break;
  }
}

void lv_draw_preHeat(void) {
  lv_obj_t *buttonBack, *buttonBAS, *buttonPLA, *buttonCUS, *buttonCool;

  if (disp_state_stack._disp_state[disp_state_stack._disp_index] != PRE_HEAT_UI) {
    disp_state_stack._disp_index++;
    disp_state_stack._disp_state[disp_state_stack._disp_index] = PRE_HEAT_UI;
  }
  disp_state = PRE_HEAT_UI;

  scr = lv_obj_create(NULL, NULL);

  lv_obj_set_style(scr, &tft_style_scr);
  lv_scr_load(scr);
  lv_obj_clean(scr);

  lv_obj_t * title = lv_label_create(scr, NULL);
  lv_obj_set_style(title, &tft_style_label_rel);
  lv_obj_set_pos(title, TITLE_XPOS, TITLE_YPOS);
  lv_label_set_text(title, creat_title_text());

  lv_refr_now(lv_refr_get_disp_refreshing());

  // Create image buttons
  buttonPLA   = lv_imgbtn_create(scr, NULL);
  buttonBAS   = lv_imgbtn_create(scr, NULL);
  buttonCUS   = lv_imgbtn_create(scr, NULL);
  buttonCool = lv_imgbtn_create(scr, NULL);
  buttonBack = lv_imgbtn_create(scr, NULL);

	lv_obj_set_event_cb_mks(buttonPLA, event_handler, ID_P_PLA, NULL, 0);
  lv_imgbtn_set_src(buttonPLA, LV_BTN_STATE_REL, "F:/bmp_heatpla.bin");
  lv_imgbtn_set_src(buttonPLA, LV_BTN_STATE_PR, "F:/bmp_clear.bin");
  lv_imgbtn_set_style(buttonPLA, LV_BTN_STATE_PR, &tft_style_label_pre);
  lv_imgbtn_set_style(buttonPLA, LV_BTN_STATE_REL, &tft_style_label_rel);
  lv_obj_clear_protect(buttonPLA, LV_PROTECT_FOLLOW);

  lv_obj_set_event_cb_mks(buttonBAS, event_handler, ID_P_BAS, NULL, 0);
  lv_imgbtn_set_src(buttonBAS, LV_BTN_STATE_REL, "F:/bmp_heatbas.bin");
  lv_imgbtn_set_src(buttonBAS, LV_BTN_STATE_PR, "F:/bmp_clear.bin");
  lv_imgbtn_set_style(buttonBAS, LV_BTN_STATE_PR, &tft_style_label_pre);
  lv_imgbtn_set_style(buttonBAS, LV_BTN_STATE_REL, &tft_style_label_rel);

  lv_obj_set_event_cb_mks(buttonCUS, event_handler, ID_P_CUS, NULL, 0);
  lv_imgbtn_set_src(buttonCUS, LV_BTN_STATE_REL, "F:/bmp_temp.bin");
  lv_imgbtn_set_src(buttonCUS, LV_BTN_STATE_PR, "F:/bmp_clear.bin");
  lv_imgbtn_set_style(buttonCUS, LV_BTN_STATE_PR, &tft_style_label_pre);
  lv_imgbtn_set_style(buttonCUS, LV_BTN_STATE_REL, &tft_style_label_rel);

	lv_obj_set_event_cb_mks(buttonCool, event_handler, ID_P_COOL, NULL, 0);
  lv_imgbtn_set_src(buttonCool, LV_BTN_STATE_REL, "F:/bmp_pre_cool.bin");
  lv_imgbtn_set_src(buttonCool, LV_BTN_STATE_PR, "F:/bmp_clear.bin");
  lv_imgbtn_set_style(buttonCool, LV_BTN_STATE_PR, &tft_style_label_pre);
  lv_imgbtn_set_style(buttonCool, LV_BTN_STATE_REL, &tft_style_label_rel);

	lv_obj_set_event_cb_mks(buttonBack, event_handler, ID_P_RETURN, NULL, 0);
	lv_imgbtn_set_src(buttonBack, LV_BTN_STATE_REL, "F:/bmp_return.bin");
	lv_imgbtn_set_src(buttonBack, LV_BTN_STATE_PR, "F:/bmp_clear.bin");
	lv_imgbtn_set_style(buttonBack, LV_BTN_STATE_PR, &tft_style_label_pre);
	lv_imgbtn_set_style(buttonBack, LV_BTN_STATE_REL, &tft_style_label_rel);

	lv_obj_set_pos(buttonCool, BTN_X_PIXEL * 2 + INTERVAL_V * 3, BTN_Y_PIXEL + INTERVAL_H + titleHeight);
  lv_obj_set_pos(buttonPLA, INTERVAL_V, BTN_Y_PIXEL + INTERVAL_H + titleHeight);
  lv_obj_set_pos(buttonBAS, BTN_X_PIXEL + INTERVAL_V * 2, BTN_Y_PIXEL + INTERVAL_H + titleHeight);
	lv_obj_set_pos(buttonCUS, BTN_X_PIXEL * 3 + INTERVAL_V * 4, titleHeight);
  lv_obj_set_pos(buttonBack, BTN_X_PIXEL * 3 + INTERVAL_V * 4, BTN_Y_PIXEL + INTERVAL_H + titleHeight);

  // Create labels on the image buttons
  lv_btn_set_layout(buttonPLA, LV_LAYOUT_OFF);
  lv_btn_set_layout(buttonBAS, LV_LAYOUT_OFF);
  lv_btn_set_layout(buttonCUS, LV_LAYOUT_OFF);
	lv_btn_set_layout(buttonCool, LV_LAYOUT_OFF);
  lv_btn_set_layout(buttonBack, LV_LAYOUT_OFF);

	lv_obj_t *labelPLA = lv_label_create(buttonPLA, NULL);
  lv_obj_t *labelBAS = lv_label_create(buttonBAS, NULL);
  lv_obj_t *labelCUS = lv_label_create(buttonCUS, NULL);
	lv_obj_t *labelCool = lv_label_create(buttonCool, NULL);
  lv_obj_t *label_Back = lv_label_create(buttonBack, NULL);

  if (gCfgItems.multiple_language != 0) {
		lv_label_set_text(labelPLA, preheat_menu.pla);
    lv_obj_align(labelPLA, buttonPLA, LV_ALIGN_IN_BOTTOM_MID, 0, BUTTON_TEXT_Y_OFFSET);

    lv_label_set_text(labelBAS, preheat_menu.bas);
    lv_obj_align(labelBAS, buttonBAS, LV_ALIGN_IN_BOTTOM_MID, 0, BUTTON_TEXT_Y_OFFSET);

    lv_label_set_text(labelCUS, preheat_menu.custom);
    lv_obj_align(labelCUS, buttonCUS, LV_ALIGN_IN_BOTTOM_MID, 0, BUTTON_TEXT_Y_OFFSET);

		lv_label_set_text(labelCool, preheat_menu.off);
    lv_obj_align(labelCool, buttonCool, LV_ALIGN_IN_BOTTOM_MID, 0, BUTTON_TEXT_Y_OFFSET);
	
    lv_label_set_text(label_Back, common_menu.text_back);
    lv_obj_align(label_Back, buttonBack, LV_ALIGN_IN_BOTTOM_MID, 0, BUTTON_TEXT_Y_OFFSET);
  }

  #if HAS_ROTARY_ENCODER
    if (gCfgItems.encoder_enable) {
			lv_group_add_obj(g, buttonPLA);
      lv_group_add_obj(g, buttonBAS);
      lv_group_add_obj(g, buttonCUS);
			lv_group_add_obj(g, buttonCool);
      lv_group_add_obj(g, buttonBack);
    }
  #endif

	hotendTempText = lv_label_create(scr, NULL);
	lv_label_set_long_mode(hotendTempText, LV_LABEL_LONG_BREAK);
	lv_obj_set_style(hotendTempText, &tft_style_label_rel);
	lv_obj_set_pos(hotendTempText, 40, 100);
	lv_obj_set_size(hotendTempText, 320, 0);
	lv_label_set_align(hotendTempText, LV_LABEL_ALIGN_LEFT);

#if HAS_HEATED_BED
	bedTempText = lv_label_create(scr, NULL);
	lv_label_set_long_mode(bedTempText, LV_LABEL_LONG_BREAK);
	lv_obj_set_style(bedTempText, &tft_style_label_rel);
	lv_obj_set_pos(bedTempText, 40, 130);
	lv_obj_set_size(bedTempText, 320, 0);
	lv_label_set_align(bedTempText, LV_LABEL_ALIGN_LEFT);
#endif

	preHotInfo = lv_label_create(scr, NULL);
	lv_label_set_long_mode(preHotInfo, LV_LABEL_LONG_BREAK);
	lv_obj_set_pos(preHotInfo, 40, 50);
	lv_obj_set_size(preHotInfo, 320, 0);
	lv_obj_set_style(preHotInfo, &style_yellow_label);
	lv_label_set_align(preHotInfo, LV_LABEL_ALIGN_LEFT);
  draw_operation_type();
	
	flash_preHeat_status();
}

void set_preheat_temp(uint8_t type){
	if(type==0)
		thermalManager.temp_hotend[uiCfg.curSprayerChoose].target = (float)0;
	else if(type == 1)
		thermalManager.temp_hotend[uiCfg.curSprayerChoose].target = PREHEAT_1_TEMP_HOTEND;
	else if(type == 2)
		thermalManager.temp_hotend[uiCfg.curSprayerChoose].target = PREHEAT_2_TEMP_HOTEND;
	thermalManager.start_watching_hotend(uiCfg.curSprayerChoose);

	#if HAS_HEATED_BED
	if (type == 0)
		thermalManager.temp_bed.target = (float)0;
	else if(type == 1)
		thermalManager.temp_bed.target = PREHEAT_1_TEMP_BED;
	else if(type == 2)
		thermalManager.temp_bed.target = PREHEAT_2_TEMP_BED;
	thermalManager.start_watching_bed();
	#endif
	flash_preHeat_status();
}

void flash_preHeat_status() {
	draw_operation_type();
	lv_draw_sprayer_temp(hotendTempText);
	lv_draw_bed_temp(bedTempText);
}

void lv_clear_preHeat() {
  #if HAS_ROTARY_ENCODER
    if (gCfgItems.encoder_enable) lv_group_remove_all_objs(g);
  #endif
  lv_obj_del(scr);
}

#endif // HAS_TFT_LVGL_UI
