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

#include "SPI_TFT.h"

#include "lv_conf.h"
#include "draw_ui.h"
#include "tft_lvgl_configuration.h"
#include "mks_hardware_test.h"
//#include "../lvgl/src/lv_objx/lv_imgbtn.h"
//#include "../lvgl/src/lv_objx/lv_img.h"
//#include "../lvgl/src/lv_core/lv_disp.h"
//#include "../lvgl/src/lv_core/lv_refr.h"

#include "../../../../MarlinCore.h"

static lv_obj_t * scr;

#define ID_ERRMSG_RESET   1

static void event_handler(lv_obj_t * obj, lv_event_t event) {
  switch (obj->mks_obj_id) {
    case ID_ERRMSG_RESET:
      if (event == LV_EVENT_CLICKED) {
        // do nothing
      }
      else if (event == LV_EVENT_RELEASED) {
				flashFirmware(0);	
      }
      break;
  }
}

void lv_draw_error_message(PGM_P const msg) {
  #if 1
    static lv_obj_t * message = NULL, *kill_message = NULL, *reset_tips = NULL, *buttonReset = NULL, *labelReset = NULL;
    if (disp_state_stack._disp_state[disp_state_stack._disp_index] != ERROR_MESSAGE_UI) {
      disp_state_stack._disp_index++;
      disp_state_stack._disp_state[disp_state_stack._disp_index] = ERROR_MESSAGE_UI;
    }
    disp_state = ERROR_MESSAGE_UI;

    scr = lv_obj_create(NULL, NULL);

    lv_obj_set_style(scr, &tft_style_scr);
    lv_scr_load(scr);
    lv_obj_clean(scr);

    lv_refr_now(lv_refr_get_disp_refreshing());

    if (msg) {
      message = lv_label_create(scr, NULL);
      lv_obj_set_style(message, &tft_style_label_rel);
      lv_label_set_text(message, msg);
      lv_obj_align(message, NULL, LV_ALIGN_CENTER, 0, -50);
    }

    kill_message = lv_label_create(scr, NULL);
    lv_obj_set_style(kill_message, &tft_style_label_rel);
    lv_label_set_text(kill_message, common_menu.killprintstop);
    lv_obj_align(kill_message, NULL, LV_ALIGN_CENTER, 0, -10);

    reset_tips = lv_label_create(scr, NULL);
    lv_obj_set_style(reset_tips, &tft_style_label_rel);
    lv_label_set_text(reset_tips, common_menu.killpleasereset);
    lv_obj_align(reset_tips, NULL, LV_ALIGN_CENTER, 0, 30);

		buttonReset = lv_btn_create(scr, NULL);
		lv_obj_set_event_cb_mks(buttonReset, event_handler, ID_ERRMSG_RESET, NULL, 0);
		lv_btn_set_style(buttonReset, LV_BTN_STYLE_REL, &style_para_back);
		lv_btn_set_style(buttonReset, LV_BTN_STYLE_PR, &style_para_back);
		lv_obj_set_pos(buttonReset, PARA_UI_BACL_POS_X, PARA_UI_BACL_POS_Y);
		lv_obj_set_size(buttonReset, PARA_UI_BACK_BTN_X_SIZE, PARA_UI_BACK_BTN_Y_SIZE);
		labelReset = lv_label_create(buttonReset, NULL);

		lv_label_set_text(labelReset, common_menu.killreset);
		lv_obj_align(labelReset, buttonReset, LV_ALIGN_CENTER, 0, 0);

	#else
	
  	SPI_TFT.LCD_clear(0x0000);
  	if (msg) disp_string((TFT_WIDTH - strlen(msg) * 16) / 2, 100, msg, 0xFFFF, 0x0000);
  	disp_string((TFT_WIDTH - strlen("PRINTER HALTED") * 16) / 2, 140, "PRINTER HALTED", 0xFFFF, 0x0000);
  	disp_string((TFT_WIDTH - strlen("Please Reset") * 16) / 2, 180, "Please Reset", 0xFFFF, 0x0000);
 
  #endif
}

void lv_clear_error_message() { lv_obj_del(scr); }

#endif // HAS_TFT_LVGL_UI
