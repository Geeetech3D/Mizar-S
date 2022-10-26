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
#include "../../../../module/motion.h"
#include "../../../../sd/cardreader.h"
#include "../../../../gcode/queue.h"
#include "../../../../gcode/gcode.h"
#include "../../../../feature/bltouch.h"

extern lv_group_t *g;
static lv_obj_t *scr;

#define ID_O_PRE_HEAT 1
#define ID_O_EXTRUCT 2
#define ID_O_MOV 3
#define ID_O_FILAMENT 4
#define ID_O_SPEED 5
#define ID_O_RETURN 6
#define ID_O_FAN 7
#define ID_O_LED 8
#define ID_O_BABY_STEP 9
#define ID_O_RUNOUT 10

extern feedRate_t feedrate_mm_s;
extern void draw_led_status(lv_obj_t *button, lv_obj_t *label);

static int32_t repeat_time;
static uint16_t repeat_event_id;
static lv_obj_t *buttonLed = NULL, *labelLed = NULL;
static lv_obj_t *buttonRunout = NULL, *labelRunout = NULL;

void options_repeat_ops()
{
    if(ABS(systick_uptime_millis - repeat_time) < 500) return;
    switch(repeat_event_id) {
    }
    repeat_event_id = 0;
}

static void draw_runout_status()
{
    if(filament_runout_sel) {
        lv_imgbtn_set_src(buttonRunout, LV_BTN_STATE_REL, "F:/bmp_filament_on.bin");
        lv_imgbtn_set_src(buttonRunout, LV_BTN_STATE_PR, "F:/bmp_filament_on.bin");
    } else {
        lv_imgbtn_set_src(buttonRunout, LV_BTN_STATE_REL, "F:/bmp_filament_off.bin");
        lv_imgbtn_set_src(buttonRunout, LV_BTN_STATE_PR, "F:/bmp_filament_off.bin");
    }

    lv_label_set_text(labelRunout, set_menu.Runout);
    lv_obj_align(labelRunout, buttonRunout, LV_ALIGN_IN_BOTTOM_MID, 0, BUTTON_TEXT_Y_OFFSET);
}

static void event_handler(lv_obj_t *obj, lv_event_t event)
{
    switch(obj->mks_obj_id) {
    case ID_O_RUNOUT:
        if(event == LV_EVENT_CLICKED) {
            // nothing to do
        } else if(event == LV_EVENT_RELEASED) {
            if(filament_runout_sel)
                filament_runout_sel = false;
            else
                filament_runout_sel = true;
            draw_runout_status();
            uiCfg.storage_flag = true;
        }
        break;
    case ID_O_PRE_HEAT:
        if(event == LV_EVENT_CLICKED) {
            // nothing to do
        } else if(event == LV_EVENT_RELEASED) {
            lv_clear_operation();
            lv_draw_handHeat();
        }
        break;
    case ID_O_EXTRUCT:
        if(event == LV_EVENT_CLICKED) {
            // nothing to do
        } else if(event == LV_EVENT_RELEASED) {
//      lv_clear_operation();
//      lv_draw_extrusion();
        }
        break;
    case ID_O_MOV:
        if(event == LV_EVENT_CLICKED) {
            // nothing to do
        } else if(event == LV_EVENT_RELEASED) {
            lv_clear_operation();
            lv_draw_move_motor();
        }
        break;
    case ID_O_FILAMENT:
        if(event == LV_EVENT_CLICKED) {
            // nothing to do
        } else if(event == LV_EVENT_RELEASED) {
            if(uiCfg.print_state == WORKING) {
                clear_cur_ui();
                lv_draw_dialog(DIALOG_TYPE_FILAMENT_CHANGE);
            } else {
                uiCfg.desireSprayerTempBak = thermalManager.temp_hotend[active_extruder].target;
                lv_clear_operation();
                lv_draw_filament_change();
            }
        }
        break;
    case ID_O_FAN:
        if(event == LV_EVENT_CLICKED) {
            // nothing to do
        } else if(event == LV_EVENT_RELEASED) {
            lv_clear_operation();
            lv_draw_fan();
        }
        break;
    case ID_O_SPEED:
        if(event == LV_EVENT_CLICKED) {
            // nothing to do
        } else if(event == LV_EVENT_RELEASED) {
            lv_clear_operation();
            lv_draw_change_speed();
        }
        break;
    case ID_O_RETURN:
        if(event == LV_EVENT_CLICKED) {
            // nothing to do
        } else if(event == LV_EVENT_RELEASED) {
            clear_cur_ui();
            draw_return_ui();
        }
        break;
    case ID_O_LED:
        if(event == LV_EVENT_CLICKED) {
            // nothing to do
        } else if(event == LV_EVENT_RELEASED) {
            uiCfg.ledSwitch++;
            uiCfg.ledSwitch %= 3;
            draw_led_status(buttonLed, labelLed);
            if(uiCfg.ledSwitch == 1)
                bltouch._whiteLedOn();
            else if(uiCfg.ledSwitch == 2)
                bltouch._colorLedOn();
            else
                bltouch._ledOff();
        }
        break;
    case ID_O_BABY_STEP:
        if(event == LV_EVENT_CLICKED) {
            // nothing to do
        } else if(event == LV_EVENT_RELEASED) {
            lv_clear_operation();
            lv_draw_baby_stepping();
        }
        break;
    }
}

void lv_draw_operation(void)
{
    lv_obj_t *buttonPreHeat = NULL, *buttonSpeed = NULL;
    lv_obj_t *buttonBack = NULL, *buttonFan = NULL;
    lv_obj_t *labelPreHeat = NULL;
    lv_obj_t *label_Back = NULL, *label_Speed = NULL, *label_Fan = NULL;
    lv_obj_t *buttonBabyStep = NULL, *label_BabyStep = NULL;
    lv_obj_t *buttonFilament = NULL, *label_Filament = NULL;

    if(disp_state_stack._disp_state[disp_state_stack._disp_index] != OPERATE_UI) {
        disp_state_stack._disp_index++;
        disp_state_stack._disp_state[disp_state_stack._disp_index] = OPERATE_UI;
    }
    disp_state = OPERATE_UI;

    scr = lv_obj_create(NULL, NULL);

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
    buttonFilament    = lv_imgbtn_create(scr, NULL);
    buttonFan = lv_imgbtn_create(scr, NULL);
    buttonLed = lv_imgbtn_create(scr, NULL);
    buttonBack = lv_imgbtn_create(scr, NULL);
    buttonRunout  = lv_imgbtn_create(scr, NULL);

    if(uiCfg.print_state == WORKING) {
        buttonSpeed = lv_imgbtn_create(scr, NULL);
        buttonBabyStep = lv_imgbtn_create(scr, NULL);
    }

    lv_obj_set_event_cb_mks(buttonPreHeat, event_handler, ID_O_PRE_HEAT, NULL, 0);
    lv_imgbtn_set_src(buttonPreHeat, LV_BTN_STATE_REL, "F:/bmp_temp.bin");
    lv_imgbtn_set_src(buttonPreHeat, LV_BTN_STATE_PR, "F:/bmp_clear.bin");
    lv_imgbtn_set_style(buttonPreHeat, LV_BTN_STATE_PR, &tft_style_label_pre);
    lv_imgbtn_set_style(buttonPreHeat, LV_BTN_STATE_REL, &tft_style_label_rel);

    lv_obj_set_event_cb_mks(buttonFilament, event_handler, ID_O_FILAMENT, NULL, 0);
    lv_imgbtn_set_src(buttonFilament, LV_BTN_STATE_REL, "F:/bmp_filamentchange.bin");
    lv_imgbtn_set_src(buttonFilament, LV_BTN_STATE_PR, "F:/bmp_clear.bin");
    lv_imgbtn_set_style(buttonFilament, LV_BTN_STATE_PR, &tft_style_label_pre);
    lv_imgbtn_set_style(buttonFilament, LV_BTN_STATE_REL, &tft_style_label_rel);

    lv_obj_set_event_cb_mks(buttonFan, event_handler, ID_O_FAN, NULL, 0);
    lv_imgbtn_set_src(buttonFan, LV_BTN_STATE_REL, "F:/bmp_fan.bin");
    lv_imgbtn_set_src(buttonFan, LV_BTN_STATE_PR, "F:/bmp_clear.bin");
    lv_imgbtn_set_style(buttonFan, LV_BTN_STATE_PR, &tft_style_label_pre);
    lv_imgbtn_set_style(buttonFan, LV_BTN_STATE_REL, &tft_style_label_rel);

    lv_obj_set_event_cb_mks(buttonLed, event_handler, ID_O_LED, NULL, 0);
    lv_imgbtn_set_style(buttonLed, LV_BTN_STATE_PR, &tft_style_label_pre);
    lv_imgbtn_set_style(buttonLed, LV_BTN_STATE_REL, &tft_style_label_rel);

    lv_obj_set_event_cb_mks(buttonRunout, event_handler, ID_O_RUNOUT, NULL, 0);
    lv_imgbtn_set_style(buttonRunout, LV_BTN_STATE_PR, &tft_style_label_pre);
    lv_imgbtn_set_style(buttonRunout, LV_BTN_STATE_REL, &tft_style_label_rel);

    if(uiCfg.print_state == WORKING) {
        lv_obj_set_event_cb_mks(buttonSpeed, event_handler, ID_O_SPEED, NULL, 0);
        lv_imgbtn_set_src(buttonSpeed, LV_BTN_STATE_REL, "F:/bmp_speed.bin");
        lv_imgbtn_set_src(buttonSpeed, LV_BTN_STATE_PR, "F:/bmp_clear.bin");
        lv_imgbtn_set_style(buttonSpeed, LV_BTN_STATE_PR, &tft_style_label_pre);
        lv_imgbtn_set_style(buttonSpeed, LV_BTN_STATE_REL, &tft_style_label_rel);

        lv_obj_set_event_cb_mks(buttonBabyStep, event_handler, ID_O_BABY_STEP, NULL, 0);
        lv_imgbtn_set_src(buttonBabyStep, LV_BTN_STATE_REL, "F:/bmp_babystep.bin");
        lv_imgbtn_set_src(buttonBabyStep, LV_BTN_STATE_PR, "F:/bmp_clear.bin");
        lv_imgbtn_set_style(buttonBabyStep, LV_BTN_STATE_PR, &tft_style_label_pre);
        lv_imgbtn_set_style(buttonBabyStep, LV_BTN_STATE_REL, &tft_style_label_rel);
    }

    lv_obj_set_event_cb_mks(buttonBack, event_handler, ID_O_RETURN, NULL, 0);
    lv_imgbtn_set_src(buttonBack, LV_BTN_STATE_REL, "F:/bmp_return.bin");
    lv_imgbtn_set_src(buttonBack, LV_BTN_STATE_PR, "F:/bmp_clear.bin");
    lv_imgbtn_set_style(buttonBack, LV_BTN_STATE_PR, &tft_style_label_pre);
    lv_imgbtn_set_style(buttonBack, LV_BTN_STATE_REL, &tft_style_label_rel);

    lv_obj_set_pos(buttonPreHeat, BTN_ROW1_COL1_POS);
    lv_obj_set_pos(buttonFilament, BTN_ROW1_COL2_POS);
    lv_obj_set_pos(buttonFan, BTN_ROW1_COL3_POS);
    lv_obj_set_pos(buttonLed, BTN_ROW1_COL4_POS);
    lv_obj_set_pos(buttonBack, BTN_ROW2_COL4_POS);
    lv_obj_set_pos(buttonRunout, BTN_ROW2_COL1_POS);

    if(uiCfg.print_state == WORKING) {
        lv_obj_set_pos(buttonSpeed, BTN_ROW2_COL2_POS);
        lv_obj_set_pos(buttonBabyStep, BTN_ROW2_COL3_POS);
    }

    // Create labels on the image buttons
    lv_btn_set_layout(buttonPreHeat, LV_LAYOUT_OFF);
    lv_btn_set_layout(buttonFilament, LV_LAYOUT_OFF);
    lv_btn_set_layout(buttonFan, LV_LAYOUT_OFF);
    lv_btn_set_layout(buttonLed, LV_LAYOUT_OFF);
    lv_btn_set_layout(buttonBack, LV_LAYOUT_OFF);
    lv_btn_set_layout(buttonRunout, LV_LAYOUT_OFF);

    if(uiCfg.print_state == WORKING) {
        lv_btn_set_layout(buttonSpeed, LV_LAYOUT_OFF);
        lv_btn_set_layout(buttonBabyStep, LV_LAYOUT_OFF);
    }

    labelPreHeat = lv_label_create(buttonPreHeat, NULL);
    label_Filament = lv_label_create(buttonFilament, NULL);
    label_Fan = lv_label_create(buttonFan, NULL);
    labelLed = lv_label_create(buttonLed, NULL);
    label_Back = lv_label_create(buttonBack, NULL);
    labelRunout = lv_label_create(buttonRunout, NULL);
    if(uiCfg.print_state == WORKING) {
        label_Speed = lv_label_create(buttonSpeed, NULL);
        label_BabyStep = lv_label_create(buttonBabyStep, NULL);
    }

    if(gCfgItems.multiple_language != 0) {
        lv_label_set_text(labelPreHeat, operation_menu.temp);
        lv_obj_align(labelPreHeat, buttonPreHeat, LV_ALIGN_IN_BOTTOM_MID, 0, BUTTON_TEXT_Y_OFFSET);

        lv_label_set_text(label_Filament, operation_menu.filament);
        lv_obj_align(label_Filament, buttonFilament, LV_ALIGN_IN_BOTTOM_MID, 0, BUTTON_TEXT_Y_OFFSET);

        lv_label_set_text(label_Fan, operation_menu.fan);
        lv_obj_align(label_Fan, buttonFan, LV_ALIGN_IN_BOTTOM_MID, 0, BUTTON_TEXT_Y_OFFSET);

        lv_label_set_text(labelLed, set_menu.led);
        lv_obj_align(labelLed, buttonLed, LV_ALIGN_IN_BOTTOM_MID, 0, BUTTON_TEXT_Y_OFFSET);

        lv_label_set_text(label_Back, common_menu.text_back);
        lv_obj_align(label_Back, buttonBack, LV_ALIGN_IN_BOTTOM_MID, 0, BUTTON_TEXT_Y_OFFSET);

        if(uiCfg.print_state == WORKING) {
            lv_label_set_text(label_Speed, operation_menu.speed);
            lv_obj_align(label_Speed, buttonSpeed, LV_ALIGN_IN_BOTTOM_MID, 0, BUTTON_TEXT_Y_OFFSET);

            lv_label_set_text(label_BabyStep, operation_menu.babystep);
            lv_obj_align(label_BabyStep, buttonBabyStep, LV_ALIGN_IN_BOTTOM_MID, 0, BUTTON_TEXT_Y_OFFSET);
        }
    }

    draw_runout_status();
    draw_led_status(buttonLed, labelLed);
}

void lv_clear_operation()
{
    lv_obj_del(scr);
}

#endif // HAS_TFT_LVGL_UI
