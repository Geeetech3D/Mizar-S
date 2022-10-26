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
#include "../../../../gcode/queue.h"
#include "../../../../module/endstops.h"

extern lv_group_t * g;
static lv_obj_t * scr;

static lv_obj_t *labelV, *buttonV;

#define ID_M_X_P    1
#define ID_M_X_N    2
#define ID_M_Y_P    3
#define ID_M_Y_N    4
#define ID_M_Z_P    5
#define ID_M_Z_N    6
#define ID_M_STEP   7
#define ID_M_RETURN 8

#define PIC_WIDTH 70

static void event_handler(lv_obj_t * obj, lv_event_t event) {
  switch (obj->mks_obj_id) {
    case ID_M_X_P:
      if (event == LV_EVENT_CLICKED) {
        // nothing to do
      }
      else if (event == LV_EVENT_RELEASED) {
        if (queue.length <= (BUFSIZE - 3)) {
          ZERO(public_buf_l);
          queue.enqueue_one_P(PSTR("G91"));
          sprintf_P(public_buf_l, PSTR("G1 X%3.1f F%d"), uiCfg.move_dist, uiCfg.moveSpeed);
          queue.enqueue_one_now(public_buf_l);
          queue.enqueue_one_P(PSTR("G90"));
		  		endstops.x_manual_limit = 1;
        }
      }
      break;
    case ID_M_X_N:
      if (event == LV_EVENT_CLICKED) {
        // nothing to do
      }
      else if (event == LV_EVENT_RELEASED) {
        if (queue.length <= (BUFSIZE - 3)) {
          ZERO(public_buf_l);
          queue.enqueue_now_P(PSTR("G91"));
          sprintf_P(public_buf_l, PSTR("G1 X-%3.1f F%d"), uiCfg.move_dist, uiCfg.moveSpeed);
          queue.enqueue_one_now(public_buf_l);
          queue.enqueue_now_P(PSTR("G90"));
		  		endstops.x_manual_limit = 1;
        }
      }
      break;
    case ID_M_Y_P:
      if (event == LV_EVENT_CLICKED) {
        // nothing to do
      }
      else if (event == LV_EVENT_RELEASED) {
        if (queue.length <= (BUFSIZE - 3)) {
          ZERO(public_buf_l);
          queue.enqueue_now_P(PSTR("G91"));
          sprintf_P(public_buf_l, PSTR("G1 Y%3.1f F%d"), uiCfg.move_dist, uiCfg.moveSpeed);
          queue.enqueue_one_now(public_buf_l);
          queue.enqueue_now_P(PSTR("G90"));
		  		endstops.y_manual_limit = 1;
        }
      }
      break;
    case ID_M_Y_N:
      if (event == LV_EVENT_CLICKED) {
        // nothing to do
      }
      else if (event == LV_EVENT_RELEASED) {
        if (queue.length <= (BUFSIZE - 3)) {
          ZERO(public_buf_l);
          queue.enqueue_now_P(PSTR("G91"));
          sprintf_P(public_buf_l, PSTR("G1 Y-%3.1f F%d"), uiCfg.move_dist, uiCfg.moveSpeed);
          queue.enqueue_one_now(public_buf_l);
          queue.enqueue_now_P(PSTR("G90"));
		  		endstops.y_manual_limit = 1;
        }
      }
      break;
    case ID_M_Z_P:
      if (event == LV_EVENT_CLICKED) {
        // nothing to do
      }
      else if (event == LV_EVENT_RELEASED) {
        if (queue.length <= (BUFSIZE - 3)) {
          ZERO(public_buf_l);
          queue.enqueue_now_P(PSTR("G91"));
          sprintf_P(public_buf_l, PSTR("G1 Z%3.1f F%d"), uiCfg.move_dist, uiCfg.moveSpeed);
          queue.enqueue_one_now(public_buf_l);
          queue.enqueue_now_P(PSTR("G90"));
		  		endstops.z_manual_limit = 1;
        }
      }
      break;
    case ID_M_Z_N:
      if (event == LV_EVENT_CLICKED) {
        // nothing to do
      }
      else if (event == LV_EVENT_RELEASED) {
        if (queue.length <= (BUFSIZE - 3)) {
          ZERO(public_buf_l);
          queue.enqueue_now_P(PSTR("G91"));
          sprintf_P(public_buf_l, PSTR("G1 Z-%3.1f F%d"), uiCfg.move_dist, uiCfg.moveSpeed);
          queue.enqueue_one_now(public_buf_l);
          queue.enqueue_now_P(PSTR("G90"));
		  		endstops.z_manual_limit = 1;
        }
      }
      break;
    case ID_M_STEP:
      if (event == LV_EVENT_CLICKED) {
        // nothing to do
      }
      else if (event == LV_EVENT_RELEASED) {
        if (abs(10 * (int)uiCfg.move_dist) == 100)
          uiCfg.move_dist = 0.1;
        else
          uiCfg.move_dist *= (float)10;

        disp_move_dist();
      }

      break;
    case ID_M_RETURN:
      if (event == LV_EVENT_CLICKED) {
        // nothing to do
      }
      else if (event == LV_EVENT_RELEASED) {
				endstops.x_manual_limit = 0;
				endstops.y_manual_limit = 0;
				endstops.z_manual_limit = 0;	
        clear_cur_ui();
        draw_return_ui();
      }
      break;
  }
}

void lv_draw_move_motor(void) {
  lv_obj_t *buttonXI, *buttonXD, *buttonYI, *buttonYD, *buttonZI, *buttonZD, *buttonBack;

  if (disp_state_stack._disp_state[disp_state_stack._disp_index] != MOVE_MOTOR_UI) {
    disp_state_stack._disp_index++;
    disp_state_stack._disp_state[disp_state_stack._disp_index] = MOVE_MOTOR_UI;
  }
  disp_state = MOVE_MOTOR_UI;

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
  buttonXI   = lv_imgbtn_create(scr, NULL);
  buttonXD   = lv_imgbtn_create(scr, NULL);
  buttonYI   = lv_imgbtn_create(scr, NULL);
  buttonYD   = lv_imgbtn_create(scr, NULL);
  buttonZI   = lv_imgbtn_create(scr, NULL);
  buttonZD   = lv_imgbtn_create(scr, NULL);
  buttonV    = lv_imgbtn_create(scr, NULL);
  buttonBack = lv_imgbtn_create(scr, NULL);

  lv_obj_set_event_cb_mks(buttonXI, event_handler, ID_M_X_P, NULL, 0);
  lv_imgbtn_set_src(buttonXI, LV_BTN_STATE_REL, "F:/bmp_move_xAdd.bin");
  lv_imgbtn_set_src(buttonXI, LV_BTN_STATE_PR, "F:/bmp_move_clear.bin");
  lv_imgbtn_set_style(buttonXI, LV_BTN_STATE_PR, &tft_style_label_pre);
  lv_imgbtn_set_style(buttonXI, LV_BTN_STATE_REL, &tft_style_label_rel);
  lv_obj_clear_protect(buttonXI, LV_PROTECT_FOLLOW);

  lv_obj_set_event_cb_mks(buttonXD, event_handler, ID_M_X_N, NULL, 0);
  lv_imgbtn_set_src(buttonXD, LV_BTN_STATE_REL, "F:/bmp_move_xDec.bin");
  lv_imgbtn_set_src(buttonXD, LV_BTN_STATE_PR, "F:/bmp_move_clear.bin");
  lv_imgbtn_set_style(buttonXD, LV_BTN_STATE_PR, &tft_style_label_pre);
  lv_imgbtn_set_style(buttonXD, LV_BTN_STATE_REL, &tft_style_label_rel);

  lv_obj_set_event_cb_mks(buttonYI, event_handler, ID_M_Y_P, NULL, 0);
  lv_imgbtn_set_src(buttonYI, LV_BTN_STATE_REL, "F:/bmp_move_yAdd.bin");
  lv_imgbtn_set_src(buttonYI, LV_BTN_STATE_PR, "F:/bmp_move_clear.bin");
  lv_imgbtn_set_style(buttonYI, LV_BTN_STATE_PR, &tft_style_label_pre);
  lv_imgbtn_set_style(buttonYI, LV_BTN_STATE_REL, &tft_style_label_rel);

  lv_obj_set_event_cb_mks(buttonYD, event_handler, ID_M_Y_N, NULL, 0);
  lv_imgbtn_set_src(buttonYD, LV_BTN_STATE_REL, "F:/bmp_move_yDec.bin");
  lv_imgbtn_set_src(buttonYD, LV_BTN_STATE_PR, "F:/bmp_move_clear.bin");
  lv_imgbtn_set_style(buttonYD, LV_BTN_STATE_PR, &tft_style_label_pre);
  lv_imgbtn_set_style(buttonYD, LV_BTN_STATE_REL, &tft_style_label_rel);

  lv_obj_set_event_cb_mks(buttonZI, event_handler, ID_M_Z_P, NULL, 0);
  lv_imgbtn_set_src(buttonZI, LV_BTN_STATE_REL, "F:/bmp_move_zAdd.bin");
  lv_imgbtn_set_src(buttonZI, LV_BTN_STATE_PR, "F:/bmp_move_clear.bin");
  lv_imgbtn_set_style(buttonZI, LV_BTN_STATE_PR, &tft_style_label_pre);
  lv_imgbtn_set_style(buttonZI, LV_BTN_STATE_REL, &tft_style_label_rel);

  lv_obj_set_event_cb_mks(buttonZD, event_handler, ID_M_Z_N, NULL, 0);
  lv_imgbtn_set_src(buttonZD, LV_BTN_STATE_REL, "F:/bmp_move_zDec.bin");
  lv_imgbtn_set_src(buttonZD, LV_BTN_STATE_PR, "F:/bmp_move_clear.bin");
  lv_imgbtn_set_style(buttonZD, LV_BTN_STATE_PR, &tft_style_label_pre);
  lv_imgbtn_set_style(buttonZD, LV_BTN_STATE_REL, &tft_style_label_rel);

  lv_obj_set_event_cb_mks(buttonV, event_handler, ID_M_STEP, NULL, 0);
  lv_imgbtn_set_style(buttonV, LV_BTN_STATE_PR, &tft_style_label_pre);
  lv_imgbtn_set_style(buttonV, LV_BTN_STATE_REL, &tft_style_label_rel);

  lv_obj_set_event_cb_mks(buttonBack, event_handler, ID_M_RETURN, NULL, 0);
  lv_imgbtn_set_src(buttonBack, LV_BTN_STATE_REL, "F:/bmp_return.bin");
  lv_imgbtn_set_src(buttonBack, LV_BTN_STATE_PR, "F:/bmp_clear.bin");
  lv_imgbtn_set_style(buttonBack, LV_BTN_STATE_PR, &tft_style_label_pre);
  lv_imgbtn_set_style(buttonBack, LV_BTN_STATE_REL, &tft_style_label_rel);

	lv_obj_align(buttonYD, scr, LV_ALIGN_CENTER, -110, -80);
	lv_obj_align(buttonYI, buttonYD, LV_ALIGN_OUT_BOTTOM_MID, 0, PIC_WIDTH+40);
	lv_obj_align(buttonXD, buttonYD, LV_ALIGN_OUT_LEFT_MID, -5, PIC_WIDTH+20);
	lv_obj_align(buttonXI, buttonYD, LV_ALIGN_OUT_RIGHT_MID, 5, PIC_WIDTH+20);
	lv_obj_align(buttonZI, buttonYD, LV_ALIGN_OUT_RIGHT_MID, 90, 0);
	lv_obj_align(buttonZD, buttonYI, LV_ALIGN_OUT_RIGHT_MID, 90, 0);
  lv_obj_set_pos(buttonV, BTN_X_PIXEL * 3 + INTERVAL_V * 4, titleHeight);
  lv_obj_set_pos(buttonBack, BTN_X_PIXEL * 3 + INTERVAL_V * 4, BTN_Y_PIXEL + INTERVAL_H + titleHeight);

  // Create labels on the image buttons
  lv_btn_set_layout(buttonV, LV_LAYOUT_OFF);
  lv_btn_set_layout(buttonBack, LV_LAYOUT_OFF);

  labelV = lv_label_create(buttonV, NULL);
  lv_obj_t *label_Back = lv_label_create(buttonBack, NULL);

  if (gCfgItems.multiple_language != 0) {
    lv_label_set_text(label_Back, common_menu.text_back);
    lv_obj_align(label_Back, buttonBack, LV_ALIGN_IN_BOTTOM_MID, 0, BUTTON_TEXT_Y_OFFSET);
  }

  disp_move_dist();
}

void disp_move_dist() {
  // char buf[30] = {0};

  if ((int)(10 * uiCfg.move_dist) == 1) {
    lv_imgbtn_set_src(buttonV, LV_BTN_STATE_REL, "F:/bmp_step_move0_1.bin");
    lv_imgbtn_set_src(buttonV, LV_BTN_STATE_PR, "F:/bmp_clear.bin");
  }
  else if ((int)(10 * uiCfg.move_dist) == 10) {
    lv_imgbtn_set_src(buttonV, LV_BTN_STATE_REL, "F:/bmp_step_move1.bin");
    lv_imgbtn_set_src(buttonV, LV_BTN_STATE_PR, "F:/bmp_clear.bin");
  }
  else if ((int)(10 * uiCfg.move_dist) == 100) {
    lv_imgbtn_set_src(buttonV, LV_BTN_STATE_REL, "F:/bmp_step_move10.bin");
    lv_imgbtn_set_src(buttonV, LV_BTN_STATE_PR, "F:/bmp_clear.bin");
  }
  if (gCfgItems.multiple_language != 0) {
    if ((int)(10 * uiCfg.move_dist) == 1) {
      lv_label_set_text(labelV, move_menu.step_01mm);
      lv_obj_align(labelV, buttonV, LV_ALIGN_IN_BOTTOM_MID, 0, BUTTON_TEXT_Y_OFFSET);
    }
    else if ((int)(10 * uiCfg.move_dist) == 10) {
      lv_label_set_text(labelV, move_menu.step_1mm);
      lv_obj_align(labelV, buttonV, LV_ALIGN_IN_BOTTOM_MID, 0, BUTTON_TEXT_Y_OFFSET);
    }
    else if ((int)(10 * uiCfg.move_dist) == 100) {
      lv_label_set_text(labelV, move_menu.step_10mm);
      lv_obj_align(labelV, buttonV, LV_ALIGN_IN_BOTTOM_MID, 0, BUTTON_TEXT_Y_OFFSET);
    }
  }
}

void lv_clear_move_motor() {
  lv_obj_del(scr);
}

#endif // HAS_TFT_LVGL_UI
