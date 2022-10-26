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
#include "../../../../gcode/queue.h"
#include "../../../../module/temperature.h"
#include "../../../../module/planner.h"

#if ENABLED(BLTOUCH)
#include "../../../../feature/bltouch.h"
#endif

extern lv_group_t *g;
static lv_obj_t *scr;

#define ID_T_PRE_HEAT 1
#define ID_T_EXTRUCT 2
#define ID_T_MOV 3
#define ID_T_HOME 4
#define ID_T_FILAMENT 5
#define ID_T_RETURN 6
#define ID_T_MOTOR_OFF 7
#define ID_T_LEVEL_MANUAL 8
#define ID_T_LEVEL_AUTO 9

#if ENABLED(MKS_TEST)
extern uint8_t curent_disp_ui;
#endif

static int32_t repeat_time;
static uint16_t repeat_event_id;

void tool_repeat_ops()
{
  if (ABS(systick_uptime_millis - repeat_time) < 500)
    return;

  switch (repeat_event_id)
  {
  case ID_T_MOTOR_OFF:
    queue.inject_P(PSTR("M84"));
    break;
  }
  repeat_event_id = 0;
}

static void event_handler(lv_obj_t *obj, lv_event_t event)
{
  switch (obj->mks_obj_id)
  {
  case ID_T_PRE_HEAT:
    if (event == LV_EVENT_CLICKED)
    {
      // nothing to do
    }
    else if (event == LV_EVENT_RELEASED)
    {
      lv_clear_tool();
      lv_draw_preHeat();
    }
    break;
  case ID_T_MOTOR_OFF:
    if (event == LV_EVENT_CLICKED)
    {
      // nothing to do
    }
    else if (event == LV_EVENT_RELEASED)
    {
#if HAS_SUICIDE
      suicide();
#else
      repeat_time = systick_uptime_millis;
      repeat_event_id = ID_T_MOTOR_OFF;
#endif
    }
    break;
 	case ID_T_LEVEL_MANUAL:
		if (event == LV_EVENT_CLICKED)
    {
      // nothing to do
    }
    else if (event == LV_EVENT_RELEASED)
    {
			uiCfg.leveling_type_save = auto_manu_level_sel;
			auto_manu_level_sel = 0;
			clear_cur_ui();
			lv_draw_manual_level();
    }
    break;
  case ID_T_LEVEL_AUTO:
		if (event == LV_EVENT_CLICKED)
    {
      // nothing to do
    }
    else if (event == LV_EVENT_RELEASED)
    {
			clear_cur_ui();
			lv_draw_autolevel_menu();
    }
    break;
  case ID_T_MOV:
    if (event == LV_EVENT_CLICKED)
    {
      // nothing to do
    }
    else if (event == LV_EVENT_RELEASED)
    {
      lv_clear_tool();
      lv_draw_move_motor();
    }
    break;
  case ID_T_HOME:
    if (event == LV_EVENT_CLICKED)
    {
      // nothing to do
    }
    else if (event == LV_EVENT_RELEASED)
    {
      //lv_clear_tool();
      //lv_draw_home();
	    queue.inject_P(PSTR("G28"));
    }
    break;
  case ID_T_FILAMENT:
    if (event == LV_EVENT_CLICKED)
    {
      // nothing to do
    }
    else if (event == LV_EVENT_RELEASED)
    {
      uiCfg.desireSprayerTempBak = thermalManager.temp_hotend[uiCfg.curSprayerChoose].target;
      lv_clear_tool();
      lv_draw_filament_change();
    }
    break;
  case ID_T_RETURN:
    if (event == LV_EVENT_CLICKED)
    {
      // nothing to do
    }
    else if (event == LV_EVENT_RELEASED)
    {
      TERN_(MKS_TEST, curent_disp_ui = 1);
      lv_clear_tool();
      lv_draw_ready_print();
    }
    break;
  }
}

void lv_draw_tool(void)
{
  lv_obj_t *buttonPreHeat, *buttonMove, *buttonHome, *buttonFilament;
  lv_obj_t *buttonBack, *buMotorOff, *buttonAuto, *buttonManual;

  if (disp_state_stack._disp_state[disp_state_stack._disp_index] != TOOL_UI)
  {
    disp_state_stack._disp_index++;
    disp_state_stack._disp_state[disp_state_stack._disp_index] = TOOL_UI;
  }
  disp_state = TOOL_UI;

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

  // Create image buttons
  buttonPreHeat = lv_imgbtn_create(scr, NULL);
  buttonMove = lv_imgbtn_create(scr, NULL);
  buttonHome = lv_imgbtn_create(scr, NULL);
  buttonFilament = lv_imgbtn_create(scr, NULL);
  buMotorOff = lv_imgbtn_create(scr, NULL);
  buttonBack = lv_imgbtn_create(scr, NULL);
	buttonAuto = lv_imgbtn_create(scr, NULL);
	buttonManual = lv_imgbtn_create(scr, NULL);

  lv_obj_set_event_cb_mks(buttonPreHeat, event_handler, ID_T_PRE_HEAT, NULL, 0);
  lv_imgbtn_set_src(buttonPreHeat, LV_BTN_STATE_REL, "F:/bmp_preHeat.bin");
  lv_imgbtn_set_src(buttonPreHeat, LV_BTN_STATE_PR, "F:/bmp_clear.bin");
  lv_imgbtn_set_style(buttonPreHeat, LV_BTN_STATE_PR, &tft_style_label_pre);
  lv_imgbtn_set_style(buttonPreHeat, LV_BTN_STATE_REL, &tft_style_label_rel);

  lv_obj_set_event_cb_mks(buttonMove, event_handler, ID_T_MOV, NULL, 0);
  lv_imgbtn_set_src(buttonMove, LV_BTN_STATE_REL, "F:/bmp_mov.bin");
  lv_imgbtn_set_src(buttonMove, LV_BTN_STATE_PR, "F:/bmp_clear.bin");
  lv_imgbtn_set_style(buttonMove, LV_BTN_STATE_PR, &tft_style_label_pre);
  lv_imgbtn_set_style(buttonMove, LV_BTN_STATE_REL, &tft_style_label_rel);

  lv_obj_set_event_cb_mks(buttonHome, event_handler, ID_T_HOME, NULL, 0);
  lv_imgbtn_set_src(buttonHome, LV_BTN_STATE_REL, "F:/bmp_zero.bin");
  lv_imgbtn_set_src(buttonHome, LV_BTN_STATE_PR, "F:/bmp_clear.bin");
  lv_imgbtn_set_style(buttonHome, LV_BTN_STATE_PR, &tft_style_label_pre);
  lv_imgbtn_set_style(buttonHome, LV_BTN_STATE_REL, &tft_style_label_rel);

  lv_obj_set_event_cb_mks(buttonFilament, event_handler, ID_T_FILAMENT, NULL, 0);
  lv_imgbtn_set_src(buttonFilament, LV_BTN_STATE_REL, "F:/bmp_filamentchange.bin");
  lv_imgbtn_set_src(buttonFilament, LV_BTN_STATE_PR, "F:/bmp_clear.bin");
  lv_imgbtn_set_style(buttonFilament, LV_BTN_STATE_PR, &tft_style_label_pre);
  lv_imgbtn_set_style(buttonFilament, LV_BTN_STATE_REL, &tft_style_label_rel);

  lv_obj_set_event_cb_mks(buttonBack, event_handler, ID_T_RETURN, NULL, 0);
  lv_imgbtn_set_src(buttonBack, LV_BTN_STATE_REL, "F:/bmp_return.bin");
  lv_imgbtn_set_src(buttonBack, LV_BTN_STATE_PR, "F:/bmp_clear.bin");
  lv_imgbtn_set_style(buttonBack, LV_BTN_STATE_PR, &tft_style_label_pre);
  lv_imgbtn_set_style(buttonBack, LV_BTN_STATE_REL, &tft_style_label_rel);

  lv_obj_set_event_cb_mks(buMotorOff, event_handler, ID_T_MOTOR_OFF, NULL, 0);
#if HAS_SUICIDE
  lv_imgbtn_set_src(buMotorOff, LV_BTN_STATE_REL, "F:/bmp_manual_off.bin");
  lv_imgbtn_set_src(buMotorOff, LV_BTN_STATE_PR, "F:/bmp_clear.bin");
#else
	if(gCfgItems.language == LANG_SIMPLE_CHINESE)
		lv_imgbtn_set_src(buMotorOff, LV_BTN_STATE_REL, "F:/bmp_motor_off_cn.bin");
	else
		lv_imgbtn_set_src(buMotorOff, LV_BTN_STATE_REL, "F:/bmp_motor_off.bin");
	lv_imgbtn_set_src(buMotorOff, LV_BTN_STATE_PR, "F:/bmp_clear.bin");
#endif
  lv_imgbtn_set_style(buMotorOff, LV_BTN_STATE_PR, &tft_style_label_pre);
  lv_imgbtn_set_style(buMotorOff, LV_BTN_STATE_REL, &tft_style_label_rel);

	lv_obj_set_event_cb_mks(buttonAuto, event_handler, ID_T_LEVEL_AUTO, NULL, 0);
  lv_imgbtn_set_src(buttonAuto, LV_BTN_STATE_REL, "F:/bmp_leveling_auto.bin");
  lv_imgbtn_set_src(buttonAuto, LV_BTN_STATE_PR, "F:/bmp_clear.bin");
  lv_imgbtn_set_style(buttonAuto, LV_BTN_STATE_PR, &tft_style_label_pre);
  lv_imgbtn_set_style(buttonAuto, LV_BTN_STATE_REL, &tft_style_label_rel);
  lv_obj_clear_protect(buttonAuto, LV_PROTECT_FOLLOW);

  lv_obj_set_event_cb_mks(buttonManual, event_handler, ID_T_LEVEL_MANUAL, NULL, 0);
  lv_imgbtn_set_src(buttonManual, LV_BTN_STATE_REL, "F:/bmp_leveling_manual.bin");
  lv_imgbtn_set_src(buttonManual, LV_BTN_STATE_PR, "F:/bmp_clear.bin");
  lv_imgbtn_set_style(buttonManual, LV_BTN_STATE_PR, &tft_style_label_pre);
  lv_imgbtn_set_style(buttonManual, LV_BTN_STATE_REL, &tft_style_label_rel);

  lv_obj_set_pos(buttonPreHeat, BTN_ROW1_COL1_POS);
	lv_obj_set_pos(buttonFilament, BTN_ROW1_COL2_POS);
  lv_obj_set_pos(buttonMove, BTN_ROW1_COL3_POS);
  lv_obj_set_pos(buttonHome, BTN_ROW1_COL4_POS);
  lv_obj_set_pos(buMotorOff, BTN_ROW2_COL3_POS);
  lv_obj_set_pos(buttonBack, BTN_ROW2_COL4_POS);
	lv_obj_set_pos(buttonAuto, BTN_ROW2_COL1_POS);
	lv_obj_set_pos(buttonManual, BTN_ROW2_COL2_POS);

  // Create labels on the image buttons
  lv_btn_set_layout(buttonPreHeat, LV_LAYOUT_OFF);
  lv_btn_set_layout(buttonMove, LV_LAYOUT_OFF);
  lv_btn_set_layout(buttonHome, LV_LAYOUT_OFF);
  lv_btn_set_layout(buttonFilament, LV_LAYOUT_OFF);
  lv_btn_set_layout(buMotorOff, LV_LAYOUT_OFF);
  lv_btn_set_layout(buttonBack, LV_LAYOUT_OFF);
	lv_btn_set_layout(buttonAuto, LV_LAYOUT_OFF);
  lv_btn_set_layout(buttonManual, LV_LAYOUT_OFF);

  lv_obj_t *labelPreHeat = lv_label_create(buttonPreHeat, NULL);
  lv_obj_t *label_Move = lv_label_create(buttonMove, NULL);
  lv_obj_t *label_Home = lv_label_create(buttonHome, NULL);
  lv_obj_t *label_Filament = lv_label_create(buttonFilament, NULL);
  lv_obj_t *label_MotorOff = lv_label_create(buMotorOff, NULL);
  lv_obj_t *label_Back = lv_label_create(buttonBack, NULL);
	lv_obj_t *labelAuto  = lv_label_create(buttonAuto, NULL);
	lv_obj_t *labelManual = lv_label_create(buttonManual, NULL);

  if (gCfgItems.multiple_language != 0)
  {
    lv_label_set_text(labelPreHeat, tool_menu.preheat);
    lv_obj_align(labelPreHeat, buttonPreHeat, LV_ALIGN_IN_BOTTOM_MID, 0, BUTTON_TEXT_Y_OFFSET);

    lv_label_set_text(label_Move, tool_menu.move);
    lv_obj_align(label_Move, buttonMove, LV_ALIGN_IN_BOTTOM_MID, 0, BUTTON_TEXT_Y_OFFSET);

    lv_label_set_text(label_Home, tool_menu.home);
    lv_obj_align(label_Home, buttonHome, LV_ALIGN_IN_BOTTOM_MID, 0, BUTTON_TEXT_Y_OFFSET);

    lv_label_set_text(label_Filament, tool_menu.filament);
    lv_obj_align(label_Filament, buttonFilament, LV_ALIGN_IN_BOTTOM_MID, 0, BUTTON_TEXT_Y_OFFSET);

    lv_label_set_text(label_MotorOff, set_menu.motoroff);
    lv_obj_align(label_MotorOff, buMotorOff, LV_ALIGN_IN_BOTTOM_MID, 0, BUTTON_TEXT_Y_OFFSET);

    lv_label_set_text(label_Back, common_menu.text_back);
    lv_obj_align(label_Back, buttonBack, LV_ALIGN_IN_BOTTOM_MID, 0, BUTTON_TEXT_Y_OFFSET);

    lv_label_set_text(labelAuto, tool_menu.autoleveling);
    lv_obj_align(labelAuto, buttonAuto, LV_ALIGN_IN_BOTTOM_MID, 0, BUTTON_TEXT_Y_OFFSET);

    lv_label_set_text(labelManual, tool_menu.manuleveling);
    lv_obj_align(labelManual, buttonManual, LV_ALIGN_IN_BOTTOM_MID, 0, BUTTON_TEXT_Y_OFFSET);
  }
#if HAS_ROTARY_ENCODER
  if (gCfgItems.encoder_enable)
  {
    lv_group_add_obj(g, buttonPreHeat);
    lv_group_add_obj(g, buttonMove);
    lv_group_add_obj(g, buttonHome);
    lv_group_add_obj(g, buttonFilament);
    lv_group_add_obj(g, buMotorOff);
    lv_group_add_obj(g, buttonBack);
		lv_group_add_obj(g, buttonAuto);
    lv_group_add_obj(g, buttonManual);
  }
#endif
}

void lv_clear_tool()
{
#if HAS_ROTARY_ENCODER
  if (gCfgItems.encoder_enable)
    lv_group_remove_all_objs(g);
#endif
  lv_obj_del(scr);
}

#endif // HAS_TFT_LVGL_UI
