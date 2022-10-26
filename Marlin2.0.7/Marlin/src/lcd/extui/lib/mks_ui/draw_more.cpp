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

#if 0//HAS_TFT_LVGL_UI

#include "../../../../MarlinCore.h"
#include "draw_ready_print.h"
#include "draw_set.h"
#include "lv_conf.h"
#include "draw_ui.h"
#include "../../../../gcode/queue.h"
#include "../../../../module/planner.h"
#include "../../../../feature/bedlevel/bedlevel.h"
#include "../../../../module/settings.h"
#include "../../../../libs/buzzer.h"
#include "../../../../feature/bltouch.h"

extern lv_group_t *g;
static lv_obj_t *scr;
static lv_obj_t *buttonRange;
static lv_obj_t * labelInfo1, *labelInfo2, *labelState; 
static bool test_start_flag;

#define ID_MORE_RETURN 1
#define ID_MORE_RANGE 2
#define ID_MORE_CALIB1	 3
#define ID_MORE_CALIB2	 4

const static uint8_t test_number = 5;

void flash_test_info()
{
	if(test_start_flag == false) return;
	
	if(uiCfg.test_finish_flag)
	{
		if(uiCfg.test_range > 0.05)
			lv_label_set_text(labelState, "状态: 测试失败");
		else
			lv_label_set_text(labelState, "状态: 测试成功");
		lv_obj_set_click(buttonRange, 1);
		leveling_data_restore(1);
		test_start_flag = false;
	}
	else
	{
		if(uiCfg.test_counter)
		{
			sprintf(public_buf_l, "状态:  %d / %d ", uiCfg.test_counter, test_number);
			lv_label_set_text(labelState, public_buf_l);
		}	
	}
	
	sprintf(public_buf_l, "Z值:%4.3f 平均值:%4.3f 偏差:%4.3f", uiCfg.test_z,uiCfg.test_mean, uiCfg.test_sigma);
	lv_label_set_text(labelInfo1, public_buf_l);
	sprintf(public_buf_l, "最小值:%4.3f 最大值:%4.3f 范围:%4.3f", uiCfg.test_min, uiCfg.test_max, uiCfg.test_range);
	lv_label_set_text(labelInfo2, public_buf_l);
}

static void event_handler(lv_obj_t *obj, lv_event_t event)
{
  switch (obj->mks_obj_id)
  {
  case ID_MORE_RETURN:
    if (event == LV_EVENT_CLICKED)
    {
    }
    else if (event == LV_EVENT_RELEASED)
    {
    	uiCfg.ledSwitch = 2;
			bltouch._colorLedOn();
    	lv_clear_more();
			lv_draw_ready_print();
    }
    break;
	 case ID_MORE_CALIB1:
	 	if (event == LV_EVENT_CLICKED)
    {
    }
    else if (event == LV_EVENT_RELEASED)
    {
			uiCfg.ledSwitch = 0;
			bltouch._ledOff();
			lv_label_set_text(labelState, "状态: 第二步");
			lv_label_set_text(labelInfo1, "请先挂上砝码");
			lv_label_set_text(labelInfo2, "点击压力校准");
    	bltouch._touch_reset();			
    }
	 	break;
	 case ID_MORE_CALIB2:
	 	if (event == LV_EVENT_CLICKED)
    {
    }
    else if (event == LV_EVENT_RELEASED)
    {
    	bltouch._touch_calibrate();
			lv_label_set_text(labelState, "状态: 第三步");
			lv_label_set_text(labelInfo1, "等待结果");
			lv_label_set_text(labelInfo2, "彩灯:标定成功 - 白灯:标定失败");
    }
	 	break;	
	 case ID_MORE_RANGE:
    if (event == LV_EVENT_CLICKED)
    {
    }
    else if (event == LV_EVENT_RELEASED)
    {
    	bltouch._reset();
			test_set_abl_parameter();
			
			if (queue.length <= (BUFSIZE - 3)) {
				queue.enqueue_now_P(PSTR("G28"));
				ZERO(public_buf_l);
				sprintf_P(public_buf_l, PSTR("G1 F3000 X%4.1f Y%4.1f"), (float)X_BED_SIZE/2, (float)Y_BED_SIZE/2);
				queue.enqueue_one_now(public_buf_l);
				ZERO(public_buf_l);
				sprintf_P(public_buf_l, PSTR("M48 V4 P%d"), test_number);
				queue.enqueue_one_now(public_buf_l);
				uiCfg.test_finish_flag = 0;
			}
			
			test_start_flag = true;
			lv_obj_set_click(buttonRange, 0);
			lv_label_set_text(labelState, "状态: 开始测试...");	
    }
    break;	
  }
}

void lv_draw_more(void)
{
  if (disp_state_stack._disp_state[disp_state_stack._disp_index] != MORE_UI)
  {
    disp_state_stack._disp_index++;
    disp_state_stack._disp_state[disp_state_stack._disp_index] = MORE_UI;
  }
  disp_state = MORE_UI;

  scr = lv_obj_create(NULL, NULL);

  //static lv_style_t tool_style;

  lv_obj_set_style(scr, &tft_style_scr);
  lv_scr_load(scr);
  lv_obj_clean(scr);

  lv_obj_t *title = lv_label_create(scr, NULL);
  lv_obj_set_style(title, &tft_style_label_rel);
  lv_obj_set_pos(title, TITLE_XPOS, TITLE_YPOS);
  lv_label_set_text(title, creat_title_text());

  lv_refr_now(lv_refr_get_disp_refreshing());

	lv_obj_t *buttonCalib1= lv_imgbtn_create(scr, NULL);
  lv_obj_set_event_cb_mks(buttonCalib1, event_handler, ID_MORE_CALIB1, NULL, 0);
  lv_imgbtn_set_src(buttonCalib1, LV_BTN_STATE_REL, "F:/bmp_step_move0_1.bin");
  lv_imgbtn_set_src(buttonCalib1, LV_BTN_STATE_PR, "F:/bmp_clear.bin");
  lv_imgbtn_set_style(buttonCalib1, LV_BTN_STATE_PR, &tft_style_label_pre);
  lv_imgbtn_set_style(buttonCalib1, LV_BTN_STATE_REL, &tft_style_label_rel);
  lv_obj_set_pos(buttonCalib1, BTN_ROW2_COL1_POS);
  lv_btn_set_layout(buttonCalib1, LV_LAYOUT_OFF);
  lv_obj_t *labelCalib1 = lv_label_create(buttonCalib1, NULL);

  lv_label_set_text(labelCalib1, "零点校准");
  lv_obj_align(labelCalib1, buttonCalib1, LV_ALIGN_IN_BOTTOM_MID, 0, BUTTON_TEXT_Y_OFFSET);

	lv_obj_t *buttonCalib2 = lv_imgbtn_create(scr, NULL);
  lv_obj_set_event_cb_mks(buttonCalib2, event_handler, ID_MORE_CALIB2, NULL, 0);
  lv_imgbtn_set_src(buttonCalib2, LV_BTN_STATE_REL, "F:/bmp_step_move1.bin");
  lv_imgbtn_set_src(buttonCalib2, LV_BTN_STATE_PR, "F:/bmp_clear.bin");
  lv_imgbtn_set_style(buttonCalib2, LV_BTN_STATE_PR, &tft_style_label_pre);
  lv_imgbtn_set_style(buttonCalib2, LV_BTN_STATE_REL, &tft_style_label_rel);
  lv_obj_set_pos(buttonCalib2, BTN_ROW2_COL2_POS);
  lv_btn_set_layout(buttonCalib2, LV_LAYOUT_OFF);
  lv_obj_t *labelCalib2 = lv_label_create(buttonCalib2, NULL);

  lv_label_set_text(labelCalib2, "压力校准");
  lv_obj_align(labelCalib2, buttonCalib2, LV_ALIGN_IN_BOTTOM_MID, 0, BUTTON_TEXT_Y_OFFSET);

	buttonRange = lv_imgbtn_create(scr, NULL);
	lv_obj_set_event_cb_mks(buttonRange, event_handler, ID_MORE_RANGE, NULL, 0);
	lv_imgbtn_set_src(buttonRange, LV_BTN_STATE_REL, "F:/bmp_start.bin");
	lv_imgbtn_set_src(buttonRange, LV_BTN_STATE_PR, "F:/bmp_clear.bin");
	lv_imgbtn_set_style(buttonRange, LV_BTN_STATE_PR, &tft_style_label_pre);
	lv_imgbtn_set_style(buttonRange, LV_BTN_STATE_REL, &tft_style_label_rel);
	lv_obj_set_pos(buttonRange, BTN_ROW2_COL3_POS);
	lv_btn_set_layout(buttonRange, LV_LAYOUT_OFF);
	lv_obj_t *labelRange = lv_label_create(buttonRange, NULL);

	lv_label_set_text(labelRange, "精度测试");
	lv_obj_align(labelRange, buttonRange, LV_ALIGN_IN_BOTTOM_MID, 0, BUTTON_TEXT_Y_OFFSET);
	
  lv_obj_t *buttonBack = lv_imgbtn_create(scr, NULL);
  lv_obj_set_event_cb_mks(buttonBack, event_handler, ID_MORE_RETURN, NULL, 0);
  lv_imgbtn_set_src(buttonBack, LV_BTN_STATE_REL, "F:/bmp_return.bin");
  lv_imgbtn_set_src(buttonBack, LV_BTN_STATE_PR, "F:/bmp_clear.bin");
  lv_imgbtn_set_style(buttonBack, LV_BTN_STATE_PR, &tft_style_label_pre);
  lv_imgbtn_set_style(buttonBack, LV_BTN_STATE_REL, &tft_style_label_rel);
  lv_obj_set_pos(buttonBack, BTN_ROW2_COL4_POS);
  lv_btn_set_layout(buttonBack, LV_LAYOUT_OFF);
  lv_obj_t *labelBack = lv_label_create(buttonBack, NULL);

  lv_label_set_text(labelBack, "返回");
  lv_obj_align(labelBack, buttonBack, LV_ALIGN_IN_BOTTOM_MID, 0, BUTTON_TEXT_Y_OFFSET);

	labelInfo1 = lv_label_create(scr, NULL);
	lv_label_set_long_mode(labelInfo1, LV_LABEL_LONG_BREAK);
	lv_obj_set_style(labelInfo1, &tft_style_label_rel);
	lv_obj_set_pos(labelInfo1, 10, 100);
	lv_obj_set_size(labelInfo1, 460, 0);
	lv_label_set_align(labelInfo1, LV_LABEL_ALIGN_LEFT);

	labelInfo2 = lv_label_create(scr, NULL);
	lv_label_set_long_mode(labelInfo2, LV_LABEL_LONG_BREAK);
	lv_obj_set_style(labelInfo2, &tft_style_label_rel);
	lv_obj_set_pos(labelInfo2, 10, 130);
	lv_obj_set_size(labelInfo2, 460, 0);
	lv_label_set_align(labelInfo2, LV_LABEL_ALIGN_LEFT);

	labelState = lv_label_create(scr, NULL);
	lv_label_set_long_mode(labelState, LV_LABEL_LONG_BREAK);
	lv_obj_set_pos(labelState, 10, 50);
	lv_obj_set_size(labelState, 460, 0);
	lv_obj_set_style(labelState, &style_yellow_label);
	lv_label_set_align(labelState, LV_LABEL_ALIGN_LEFT);

	lv_label_set_text(labelState, "状态: 第一步");
	lv_label_set_text(labelInfo1, "请先拿掉砝码");
	lv_label_set_text(labelInfo2, "点击零点校准");
	test_start_flag = false;

	uiCfg.ledSwitch = 0;
	bltouch._ledOff();
	queue.inject_P(PSTR("M84"));

}

void lv_clear_more()
{
  lv_obj_del(scr);
}

#endif // HAS_TFT_LVGL_UI
