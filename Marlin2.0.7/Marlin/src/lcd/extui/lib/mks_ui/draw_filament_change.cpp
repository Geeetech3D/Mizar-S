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
#include "../../../../gcode/queue.h"
#include "../../../../gcode/gcode.h"
#include "../../../../module/motion.h"
#include "../../../../module/planner.h"

extern lv_group_t * g;
static lv_obj_t * scr;
static lv_obj_t * tempText1;
static lv_obj_t * filamentWarn;

#define ID_FILAMNT_IN     1
#define ID_FILAMNT_OUT    2
#define ID_FILAMNT_TYPE   3
#define ID_FILAMNT_RETURN 4

extern feedRate_t feedrate_mm_s;

static void event_handler(lv_obj_t * obj, lv_event_t event) {
  switch (obj->mks_obj_id) {
    case ID_FILAMNT_IN:
			if (printJobOngoing()) break;
      if (event == LV_EVENT_CLICKED) {
        // nothing to do
      }
      else if (event == LV_EVENT_RELEASED) {
        uiCfg.filament_load_heat_flg = 1;
        if ((abs(gCfgItems.filament_limit_temper - thermalManager.temp_hotend[uiCfg.curSprayerChoose].celsius) <= 1)
            || (gCfgItems.filament_limit_temper <= thermalManager.temp_hotend[uiCfg.curSprayerChoose].celsius)) {
          lv_clear_filament_change();
          lv_draw_dialog(DIALOG_TYPE_FILAMENT_HEAT_LOAD_COMPLETED);
        }
        else {
          lv_clear_filament_change();
          lv_draw_dialog(DIALOG_TYPE_FILAMENT_LOAD_HEAT);
          if (thermalManager.temp_hotend[uiCfg.curSprayerChoose].target < gCfgItems.filament_limit_temper) {
            thermalManager.temp_hotend[uiCfg.curSprayerChoose].target = gCfgItems.filament_limit_temper;
            thermalManager.start_watching_hotend(uiCfg.curSprayerChoose);
          }
        }
      }
      break;
    case ID_FILAMNT_OUT:
			if (printJobOngoing()) break;
      if (event == LV_EVENT_CLICKED) {
        // nothing to do
      }
      else if (event == LV_EVENT_RELEASED) {
        uiCfg.filament_unload_heat_flg=1;
        if ((thermalManager.temp_hotend[uiCfg.curSprayerChoose].target > 0)
          && ((abs((int)((int)gCfgItems.filament_limit_temper - thermalManager.temp_hotend[uiCfg.curSprayerChoose].celsius)) <= 1)
          || ((int)thermalManager.temp_hotend[uiCfg.curSprayerChoose].celsius >= gCfgItems.filament_limit_temper))
        ) {
          lv_clear_filament_change();
          lv_draw_dialog(DIALOG_TYPE_FILAMENT_HEAT_UNLOAD_COMPLETED);
        }
        else {
          lv_clear_filament_change();
          lv_draw_dialog(DIALOG_TYPE_FILAMENT_UNLOAD_HEAT);
          if (thermalManager.temp_hotend[uiCfg.curSprayerChoose].target < gCfgItems.filament_limit_temper) {
            thermalManager.temp_hotend[uiCfg.curSprayerChoose].target = gCfgItems.filament_limit_temper;
            thermalManager.start_watching_hotend(uiCfg.curSprayerChoose);
          }
          filament_sprayer_temp();
        }
      }
      break;
    case ID_FILAMNT_TYPE:
      if (event == LV_EVENT_CLICKED) {
        // nothing to do
      }
      else if (event == LV_EVENT_RELEASED) {
        #if HAS_MULTI_EXTRUDER
          if (uiCfg.curSprayerChoose == 0)
            uiCfg.curSprayerChoose = 1;
          else if (uiCfg.curSprayerChoose == 1)
            uiCfg.curSprayerChoose = 0;
        #endif
      }
      break;
    case ID_FILAMNT_RETURN:
      if (event == LV_EVENT_CLICKED) {
        // nothing to do
      }
      else if (event == LV_EVENT_RELEASED) {
        #if HAS_MULTI_EXTRUDER
          if (uiCfg.print_state != IDLE && uiCfg.print_state != REPRINTED)
            gcode.process_subcommands_now_P(uiCfg.curSprayerChoose_bak == 1 ? PSTR("T1") : PSTR("T0"));
        #endif
        if (uiCfg.print_state == PAUSED)
          planner.set_e_position_mm((destination.e = current_position.e = uiCfg.current_e_position_bak));
          //current_position.e = destination.e = uiCfg.current_e_position_bak;
        //thermalManager.temp_hotend[uiCfg.curSprayerChoose].target = uiCfg.desireSprayerTempBak;
        //thermalManager.start_watching_hotend(uiCfg.curSprayerChoose);

        clear_cur_ui();
        draw_return_ui();
      }
      break;
  }
}

void lv_draw_filament_change(void) {
  lv_obj_t *buttonIn, *buttonOut;
  lv_obj_t *buttonBack;

  if (disp_state_stack._disp_state[disp_state_stack._disp_index] != FILAMENTCHANGE_UI) {
    disp_state_stack._disp_index++;
    disp_state_stack._disp_state[disp_state_stack._disp_index] = FILAMENTCHANGE_UI;
  }
  disp_state = FILAMENTCHANGE_UI;

  scr = lv_obj_create(NULL, NULL);

  lv_obj_set_style(scr, &tft_style_scr);
  lv_scr_load(scr);
  lv_obj_clean(scr);

  lv_obj_t * title = lv_label_create(scr, NULL);
  lv_obj_set_style(title, &tft_style_label_rel);
  lv_obj_set_pos(title, TITLE_XPOS, TITLE_YPOS);
  lv_label_set_text(title, creat_title_text());

  lv_refr_now(lv_refr_get_disp_refreshing());

  // Create an Image button
  buttonIn   = lv_imgbtn_create(scr, NULL);
  buttonOut  = lv_imgbtn_create(scr, NULL);
  buttonBack = lv_imgbtn_create(scr, NULL);

  lv_obj_set_event_cb_mks(buttonIn, event_handler, ID_FILAMNT_IN, NULL, 0);
  lv_imgbtn_set_src(buttonIn, LV_BTN_STATE_REL, "F:/bmp_in.bin");
  lv_imgbtn_set_src(buttonIn, LV_BTN_STATE_PR, "F:/bmp_clear.bin");
  lv_imgbtn_set_style(buttonIn, LV_BTN_STATE_PR, &tft_style_label_pre);
  lv_imgbtn_set_style(buttonIn, LV_BTN_STATE_REL, &tft_style_label_rel);
  lv_obj_clear_protect(buttonIn, LV_PROTECT_FOLLOW);

  lv_obj_set_event_cb_mks(buttonOut, event_handler, ID_FILAMNT_OUT, NULL, 0);
  lv_imgbtn_set_src(buttonOut, LV_BTN_STATE_REL, "F:/bmp_out.bin");
  lv_imgbtn_set_src(buttonOut, LV_BTN_STATE_PR, "F:/bmp_clear.bin");
  lv_imgbtn_set_style(buttonOut, LV_BTN_STATE_PR, &tft_style_label_pre);
  lv_imgbtn_set_style(buttonOut, LV_BTN_STATE_REL, &tft_style_label_rel);
	
  lv_obj_set_event_cb_mks(buttonBack, event_handler, ID_FILAMNT_RETURN, NULL, 0);
  lv_imgbtn_set_src(buttonBack, LV_BTN_STATE_REL, "F:/bmp_return.bin");
  lv_imgbtn_set_src(buttonBack, LV_BTN_STATE_PR, "F:/bmp_clear.bin");
  lv_imgbtn_set_style(buttonBack, LV_BTN_STATE_PR, &tft_style_label_pre);
  lv_imgbtn_set_style(buttonBack, LV_BTN_STATE_REL, &tft_style_label_rel);

  lv_obj_set_pos(buttonIn, BTN_ROW1_COL1_POS);
  lv_obj_set_pos(buttonOut, BTN_ROW1_COL4_POS);
	lv_obj_set_pos(buttonBack, BTN_ROW2_COL4_POS);

  // Create labels on the image buttons
  lv_btn_set_layout(buttonIn, LV_LAYOUT_OFF);
  lv_btn_set_layout(buttonOut, LV_LAYOUT_OFF);
  lv_btn_set_layout(buttonBack, LV_LAYOUT_OFF);

  lv_obj_t *labelIn  = lv_label_create(buttonIn, NULL);
  lv_obj_t *labelOut = lv_label_create(buttonOut, NULL);
  lv_obj_t *label_Back = lv_label_create(buttonBack, NULL);

  if (gCfgItems.multiple_language != 0) {
    lv_label_set_text(labelIn, filament_menu.in);
    lv_obj_align(labelIn, buttonIn, LV_ALIGN_IN_BOTTOM_MID, 0, BUTTON_TEXT_Y_OFFSET);

    lv_label_set_text(labelOut, filament_menu.out);
    lv_obj_align(labelOut, buttonOut, LV_ALIGN_IN_BOTTOM_MID, 0, BUTTON_TEXT_Y_OFFSET);

    lv_label_set_text(label_Back, common_menu.text_back);
    lv_obj_align(label_Back, buttonBack, LV_ALIGN_IN_BOTTOM_MID, 0, BUTTON_TEXT_Y_OFFSET);
  }

  #if HAS_ROTARY_ENCODER
    if (gCfgItems.encoder_enable) {
      lv_group_add_obj(g, buttonIn);
      lv_group_add_obj(g, buttonOut);
      lv_group_add_obj(g, buttonBack);
    }
  #endif
	
  tempText1 = lv_label_create(scr, NULL);
  lv_obj_set_style(tempText1, &tft_style_label_rel);
  disp_filament_temp();

	filamentWarn = lv_label_create(scr, NULL);
	lv_obj_set_style(filamentWarn, &style_yellow_label);
	lv_obj_set_pos(filamentWarn, BTN_ROW2_COL2_POS);
	disp_filament_warn();
}

void disp_filament_warn() {	
	if(uiCfg.print_state ==PAUSING)
		lv_label_set_text(filamentWarn, filament_menu.change_filament_ready);
	else if(uiCfg.print_state ==PAUSED)
		lv_label_set_text(filamentWarn, filament_menu.change_filament_start);
	else
		lv_label_set_text(filamentWarn, "\0");
}

void disp_filament_temp() {
  char buf[20] = {0};

  public_buf_l[0] = '\0';

  if (uiCfg.curSprayerChoose < 1)
    strcat(public_buf_l, preheat_menu.ext1);
  else
    strcat(public_buf_l, preheat_menu.ext2);
  sprintf(buf, preheat_menu.value_state, (int)thermalManager.temp_hotend[uiCfg.curSprayerChoose].celsius,  (int)thermalManager.temp_hotend[uiCfg.curSprayerChoose].target);

  strcat_P(public_buf_l, PSTR(": "));
  strcat(public_buf_l, buf);
  lv_label_set_text(tempText1, public_buf_l);
  lv_obj_align(tempText1, NULL, LV_ALIGN_CENTER, 0, -50);
}

void lv_clear_filament_change() {
  #if HAS_ROTARY_ENCODER
    if (gCfgItems.encoder_enable) lv_group_remove_all_objs(g);
  #endif
	if(disp_state_stack._disp_state[disp_state_stack._disp_index-1]==DIALOG_UI)
	{
		if (disp_state_stack._disp_index > 0)
			disp_state_stack._disp_index--;
	}
  lv_obj_del(scr);
}

#endif // HAS_TFT_LVGL_UI
