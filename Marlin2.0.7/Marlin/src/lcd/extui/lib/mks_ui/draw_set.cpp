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
#include "../../../../gcode/queue.h"
#include "../../../../feature/bltouch.h"
#include "../../../../libs/buzzer.h"

#include "draw_ready_print.h"
#include "draw_set.h"
#include "lv_conf.h"
#include "draw_ui.h"
#include "pic_manager.h"

#define ID_S_WIFI 1
#define ID_S_FAN 2
#define ID_S_ABOUT 3
#define ID_S_CONTINUE 4
#define ID_S_LANGUAGE 5
#define ID_S_MACHINE_PARA 6
#define ID_S_EEPROM_SET 7
#define ID_S_3DTOUCH_SWT 8
#define ID_S_RETURN 9
#define ID_S_BUZZER 10
#define ID_S_LED 11
#define ID_S_FILAMENT_SET 12

static lv_obj_t *scr;
extern lv_group_t *g;
static lv_obj_t *buttonBuzz, *labelBuzz;

static lv_obj_t *buttonLanguage, *labelLanguage;
static lv_obj_t *buttonLed, *labelLed;
static lv_obj_t *buttonFilament, *labelFilament;

static int32_t repeat_time;
static uint16_t repeat_event_id;

void set_repeat_ops()
{
  if (ABS(systick_uptime_millis - repeat_time) < 500)
    return;

  switch (repeat_event_id)
  {
  case ID_S_BUZZER:
	case ID_S_FILAMENT_SET:
	case ID_S_3DTOUCH_SWT:
	case ID_S_LED:
    queue.inject_P(PSTR("M500"));
    break;
  }
  repeat_event_id = 0;
}

void draw_led_status(lv_obj_t *button, lv_obj_t *label)
{
	if(uiCfg.ledSwitch == 1)
	{
		lv_imgbtn_set_src(button, LV_BTN_STATE_REL, "F:/bmp_led_white.bin");
		lv_imgbtn_set_src(button, LV_BTN_STATE_PR, "F:/bmp_led_white.bin");
		lv_label_set_text(label, set_menu.led_white);
	}
	else if(uiCfg.ledSwitch == 2)
	{
		lv_imgbtn_set_src(button, LV_BTN_STATE_REL, "F:/bmp_led_color.bin");
		lv_imgbtn_set_src(button, LV_BTN_STATE_PR, "F:/bmp_led_color.bin");
		lv_label_set_text(label, set_menu.led_color);
	}
	else
	{
		lv_imgbtn_set_src(button, LV_BTN_STATE_REL, "F:/bmp_led_off.bin");
		lv_imgbtn_set_src(button, LV_BTN_STATE_PR, "F:/bmp_led_off.bin");
		lv_label_set_text(label, set_menu.led_off);
	}
	
	if (gCfgItems.multiple_language != 0)
  {
		if(uiCfg.ledSwitch == 1)
			lv_label_set_text(label, set_menu.led_white);
		else if(uiCfg.ledSwitch == 2)
			lv_label_set_text(label, set_menu.led_color);
		else
			lv_label_set_text(label, set_menu.led_off);
    lv_obj_align(label, button, LV_ALIGN_IN_BOTTOM_MID, 0, BUTTON_TEXT_Y_OFFSET);
	}
}

static void draw_language_status()
{
	if(gCfgItems.language == LANG_SIMPLE_CHINESE)
	{
		lv_imgbtn_set_src(buttonLanguage, LV_BTN_STATE_REL, "F:/bmp_simplified_cn_sel.bin");
		lv_imgbtn_set_src(buttonLanguage, LV_BTN_STATE_PR, "F:/bmp_simplified_cn_sel.bin");		
	}
	else
	{
		lv_imgbtn_set_src(buttonLanguage, LV_BTN_STATE_REL, "F:/bmp_english_sel.bin");
		lv_imgbtn_set_src(buttonLanguage, LV_BTN_STATE_PR, "F:/bmp_english_sel.bin");
	}
	
	if (gCfgItems.multiple_language != 0)
  {
		lv_label_set_text(labelLanguage, set_menu.language); 		
    lv_obj_align(labelLanguage, buttonLanguage, LV_ALIGN_IN_BOTTOM_MID, 0, BUTTON_TEXT_Y_OFFSET);	
	}
}

static void draw_filament_status()
{
	if(filament_runout_sel)
	{
		lv_imgbtn_set_src(buttonFilament, LV_BTN_STATE_REL, "F:/bmp_filament_on.bin");
		lv_imgbtn_set_src(buttonFilament, LV_BTN_STATE_PR, "F:/bmp_filament_on.bin");
	}
	else
	{
		lv_imgbtn_set_src(buttonFilament, LV_BTN_STATE_REL, "F:/bmp_filament_off.bin");
		lv_imgbtn_set_src(buttonFilament, LV_BTN_STATE_PR, "F:/bmp_filament_off.bin");
	}
	
	if (gCfgItems.multiple_language != 0)
	{
		lv_label_set_text(labelFilament, set_menu.Runout);
		lv_obj_align(labelFilament, buttonFilament, LV_ALIGN_IN_BOTTOM_MID, 0, BUTTON_TEXT_Y_OFFSET);
	}
}

static void draw_buzz_status()
{
  if (buzzer.onoff == 1)
  {
    lv_imgbtn_set_src(buttonBuzz, LV_BTN_STATE_REL, "F:/bmp_buzz_on.bin");
    lv_imgbtn_set_src(buttonBuzz, LV_BTN_STATE_PR, "F:/bmp_buzz_on.bin");
  }
  else
  {
    lv_imgbtn_set_src(buttonBuzz, LV_BTN_STATE_REL, "F:/bmp_buzz_off.bin");
    lv_imgbtn_set_src(buttonBuzz, LV_BTN_STATE_PR, "F:/bmp_buzz_off.bin");
  }
	
  if (gCfgItems.multiple_language != 0)
  {
    lv_label_set_text(labelBuzz, set_menu.buzzer);
    lv_obj_align(labelBuzz, buttonBuzz, LV_ALIGN_IN_BOTTOM_MID, 0, BUTTON_TEXT_Y_OFFSET);
  }
}

static void event_handler(lv_obj_t *obj, lv_event_t event)
{
  switch (obj->mks_obj_id)
  {
  case ID_S_LED:
    if (event == LV_EVENT_CLICKED)
    {
      // nothing to do
    }
    else if (event == LV_EVENT_RELEASED)
    {
    	uiCfg.ledSwitch++;
			uiCfg.ledSwitch %= 3;
			draw_led_status(buttonLed, labelLed);
			if(uiCfg.ledSwitch == 1){
				bltouch._whiteLedOn();
				led_onoff_sel = 1;
			}else if(uiCfg.ledSwitch == 2){
				bltouch._colorLedOn();
				led_onoff_sel = 1;
			}else{
				bltouch._ledOff();
				led_onoff_sel = 0;
			}
			repeat_time = systick_uptime_millis;
      repeat_event_id = ID_S_LED;
    }
    break;
  case ID_S_BUZZER:
    if (event == LV_EVENT_CLICKED)
    {
      // nothing to do
    }
    else if (event == LV_EVENT_RELEASED)
    {
      if (buzzer.onoff == 0)
        buzzer.onoff = 1;
      else
        buzzer.onoff = 0;
      draw_buzz_status();
      repeat_time = systick_uptime_millis;
      repeat_event_id = ID_S_BUZZER;
    }
    break;
  case ID_S_ABOUT:
    if (event == LV_EVENT_CLICKED)
    {
      // nothing to do
    }
    else if (event == LV_EVENT_RELEASED)
    {
      lv_clear_set();
      lv_draw_about();
    }
    break;
  case ID_S_LANGUAGE:
    if (event == LV_EVENT_CLICKED)
    {
      // nothing to do
    }
    else if (event == LV_EVENT_RELEASED)
    {
			if(gCfgItems.language == LANG_SIMPLE_CHINESE)
				gCfgItems.language = LANG_ENGLISH;
			else
				gCfgItems.language = LANG_SIMPLE_CHINESE;

			disp_language_init();
			draw_language_status();
			update_spi_flash();
			lv_clear_set();
			lv_draw_set();
    }
    break;
  case ID_S_RETURN:
    if (event == LV_EVENT_CLICKED)
    {
      // nothing to do
    }
    else if (event == LV_EVENT_RELEASED)
    {
      lv_clear_set();
      lv_draw_ready_print();
    }
    break;
	case ID_S_FILAMENT_SET:
		if (event == LV_EVENT_CLICKED)
    {
      // nothing to do
    }
    else if (event == LV_EVENT_RELEASED)
    {
    	if(filament_runout_sel)
				filament_runout_sel = false;
			else
				filament_runout_sel = true;
			draw_filament_status();
			repeat_time = systick_uptime_millis;
      repeat_event_id = ID_S_FILAMENT_SET;
    }
		break;
  }
}

void lv_draw_set(void)
{
  lv_obj_t *buttonAbout, *buttonBack;
	lv_obj_t *labelAbout, *labelBack;

  if (disp_state_stack._disp_state[disp_state_stack._disp_index] != SET_UI)
  {
    disp_state_stack._disp_index++;
    disp_state_stack._disp_state[disp_state_stack._disp_index] = SET_UI;
  }
  disp_state = SET_UI;

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
  buttonAbout = lv_imgbtn_create(scr, NULL);
  buttonBack = lv_imgbtn_create(scr, NULL);
  buttonLanguage = lv_imgbtn_create(scr, NULL);
  buttonBuzz = lv_imgbtn_create(scr, NULL);
  buttonFilament = lv_imgbtn_create(scr, NULL);
	buttonLed = lv_imgbtn_create(scr, NULL);

  lv_obj_set_event_cb_mks(buttonAbout, event_handler, ID_S_ABOUT, NULL, 0);
  lv_imgbtn_set_src(buttonAbout, LV_BTN_STATE_REL, "F:/bmp_about.bin");
  lv_imgbtn_set_src(buttonAbout, LV_BTN_STATE_PR, "F:/bmp_clear.bin");
  lv_imgbtn_set_style(buttonAbout, LV_BTN_STATE_PR, &tft_style_label_pre);
  lv_imgbtn_set_style(buttonAbout, LV_BTN_STATE_REL, &tft_style_label_rel);

	lv_obj_set_event_cb_mks(buttonBack, event_handler, ID_S_RETURN, NULL, 0);
  lv_imgbtn_set_src(buttonBack, LV_BTN_STATE_REL, "F:/bmp_return.bin");
  lv_imgbtn_set_src(buttonBack, LV_BTN_STATE_PR, "F:/bmp_clear.bin");
  lv_imgbtn_set_style(buttonBack, LV_BTN_STATE_PR, &tft_style_label_pre);
  lv_imgbtn_set_style(buttonBack, LV_BTN_STATE_REL, &tft_style_label_rel);

  lv_obj_set_event_cb_mks(buttonLanguage, event_handler, ID_S_LANGUAGE, NULL, 0);
  lv_imgbtn_set_style(buttonLanguage, LV_BTN_STATE_PR, &tft_style_label_pre);
  lv_imgbtn_set_style(buttonLanguage, LV_BTN_STATE_REL, &tft_style_label_rel);

  lv_obj_set_event_cb_mks(buttonBuzz, event_handler, ID_S_BUZZER, NULL, 0);
  lv_imgbtn_set_style(buttonBuzz, LV_BTN_STATE_PR, &tft_style_label_pre);
  lv_imgbtn_set_style(buttonBuzz, LV_BTN_STATE_REL, &tft_style_label_rel);

  lv_obj_set_event_cb_mks(buttonFilament, event_handler, ID_S_FILAMENT_SET, NULL, 0);
  lv_imgbtn_set_style(buttonFilament, LV_BTN_STATE_PR, &tft_style_label_pre);
  lv_imgbtn_set_style(buttonFilament, LV_BTN_STATE_REL, &tft_style_label_rel);

  lv_obj_set_event_cb_mks(buttonLed, event_handler, ID_S_LED, NULL, 0);
  lv_imgbtn_set_style(buttonLed, LV_BTN_STATE_PR, &tft_style_label_pre);
  lv_imgbtn_set_style(buttonLed, LV_BTN_STATE_REL, &tft_style_label_rel);

	lv_obj_set_pos(buttonAbout, BTN_ROW2_COL1_POS);
	lv_obj_set_pos(buttonBack, BTN_ROW2_COL4_POS);
  lv_obj_set_pos(buttonLanguage, BTN_ROW1_COL2_POS);
  lv_obj_set_pos(buttonBuzz, BTN_ROW1_COL3_POS);
	lv_obj_set_pos(buttonLed, BTN_ROW1_COL4_POS);
  lv_obj_set_pos(buttonFilament, BTN_ROW1_COL1_POS);

  /// Create labels on the buttons
  lv_btn_set_layout(buttonAbout, LV_LAYOUT_OFF);
  lv_btn_set_layout(buttonBack, LV_LAYOUT_OFF);
	lv_btn_set_layout(buttonLanguage, LV_LAYOUT_OFF);
  lv_btn_set_layout(buttonBuzz, LV_LAYOUT_OFF);
  lv_btn_set_layout(buttonLed, LV_LAYOUT_OFF);
  lv_btn_set_layout(buttonFilament, LV_LAYOUT_OFF);

	labelAbout = lv_label_create(buttonAbout, NULL);
	labelBack = lv_label_create(buttonBack, NULL);
	labelFilament = lv_label_create(buttonFilament, NULL);
	labelLed = lv_label_create(buttonLed, NULL);
	labelBuzz = lv_label_create(buttonBuzz, NULL);
	labelLanguage = lv_label_create(buttonLanguage, NULL);

  if (gCfgItems.multiple_language != 0)
  {
    lv_label_set_text(labelAbout, set_menu.about);
    lv_obj_align(labelAbout, buttonAbout, LV_ALIGN_IN_BOTTOM_MID, 0, BUTTON_TEXT_Y_OFFSET);

    lv_label_set_text(labelBack, common_menu.text_back);
    lv_obj_align(labelBack, buttonBack, LV_ALIGN_IN_BOTTOM_MID, 0, BUTTON_TEXT_Y_OFFSET);
  }

  draw_buzz_status();
	draw_led_status(buttonLed, labelLed);
	draw_language_status();
	draw_filament_status();
}

void lv_clear_set()
{
  lv_obj_del(scr);
}

#endif // HAS_TFT_LVGL_UI
