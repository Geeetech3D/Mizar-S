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

#if BOTH(HAS_TFT_LVGL_UI, HAS_BED_PROBE)
#include "../../../../MarlinCore.h"
#include "lv_conf.h"
#include "draw_ui.h"
#include "../../../../module/motion.h"
#include "../../../../gcode/queue.h"
#include "../../../../module/planner.h"
#include "../../../../libs/numtostr.h"
#include "../../../../module/temperature.h"
#include "../../../../feature/bedlevel/mbl/mesh_bed_leveling.h"
#include "../../../../module/probe.h"
#include "../../../../module/settings.h"
#include "../../../../feature/bltouch.h"

extern lv_group_t * g;
static lv_obj_t * scr;
static lv_obj_t *z_offset_info, *z_offset_value_disp;
static lv_obj_t *buttonStep, *labelStep;
static lv_obj_t *buttonStart, *labelStart;
static uint8_t move_delay_count = 0;

#ifdef EN_ZOFFSET_PREHEAT
static lv_obj_t *labExtraTemp;
#if HAS_HEATED_BED
static lv_obj_t *labBedTemp;
#endif
#endif

static float z_offset_dist = MESH_EDIT_Z_STEP;
static uint8_t z_offset_step;
static int8_t z_offset_value;

#define ID_ZOFFSET_RETURN  	 	1
#define ID_ZOFFSET_ADD        2
#define ID_ZOFFSET_DEC      	3
#define ID_ZOFFSET_STEP       4
#define ID_ZOFFSET_CONFIRM	 5
#define ID_ZOFFSET_START			6

static bool dataSaveFlg, dataSavedFlg;
static float z_offset_back;
static uint8_t z_offset_status;

enum {
    Z_OFFSET_STATUS_START,
    Z_OFFSET_STATUS_HOT,
    Z_OFFSET_STATUS_MOVE,
    Z_OFFSET_STATUS_ADJUST,
    Z_OFFSET_STATUS_SAVE
};

static void draw_zoffset_dist()
{
    if(z_offset_step == 0) {
        z_offset_dist = 0.01;
        lv_imgbtn_set_src(buttonStep, LV_BTN_STATE_REL, "F:/bmp_step_move0_1.bin");
        lv_imgbtn_set_src(buttonStep, LV_BTN_STATE_PR, "F:/bmp_clear.bin");
        if(gCfgItems.multiple_language != 0) {
            lv_label_set_text(labelStep, "0.01mm");
            lv_obj_align(labelStep, buttonStep, LV_ALIGN_IN_BOTTOM_MID, 0, BUTTON_TEXT_Y_OFFSET);
        }
    } else if(z_offset_step == 1) {
        z_offset_dist = 0.1;
        lv_imgbtn_set_src(buttonStep, LV_BTN_STATE_REL, "F:/bmp_step_move1.bin");
        lv_imgbtn_set_src(buttonStep, LV_BTN_STATE_PR, "F:/bmp_clear.bin");
        if(gCfgItems.multiple_language != 0) {
            lv_label_set_text(labelStep, "0.1mm");
            lv_obj_align(labelStep, buttonStep, LV_ALIGN_IN_BOTTOM_MID, 0, BUTTON_TEXT_Y_OFFSET);
        }
    } else if(z_offset_step == 2) {
        z_offset_dist = 0.5;
        lv_imgbtn_set_src(buttonStep, LV_BTN_STATE_REL, "F:/bmp_step_move10.bin");
        lv_imgbtn_set_src(buttonStep, LV_BTN_STATE_PR, "F:/bmp_clear.bin");
        if(gCfgItems.multiple_language != 0) {
            lv_label_set_text(labelStep, "0.5mm");
            lv_obj_align(labelStep, buttonStep, LV_ALIGN_IN_BOTTOM_MID, 0, BUTTON_TEXT_Y_OFFSET);
        }
    }
}

static void draw_zoffset_value()
{
    constexpr float dpo[] = NOZZLE_TO_PROBE_OFFSET;
    const float v = probe.offset.z - dpo[Z_AXIS];
    ZERO(public_buf_l);
    sprintf_P(public_buf_l, PSTR("%s %s"), leveling_menu.zoffsetvalue, ftostr43sign(v + (v < 0 ? -0.0001f : 0.0001f), '+'));
    lv_label_set_text(z_offset_value_disp, public_buf_l);
    lv_label_set_text(z_offset_info, leveling_menu.zoffsetadjust);
}

static void set_zoffset_value()
{
    float z = current_position.z + float(z_offset_value * z_offset_dist);
    if(z > LCD_PROBE_Z_RANGE / 2) {
        z = LCD_PROBE_Z_RANGE / 2;
        probe.offset.z = z;
    } else if(z < -LCD_PROBE_Z_RANGE / 2) {
        z = -LCD_PROBE_Z_RANGE / 2;
        probe.offset.z = z;
    } else
        probe.offset.z += z_offset_value * z_offset_dist;
    current_position.z = constrain(z, -(LCD_PROBE_Z_RANGE) * 0.5f, (LCD_PROBE_Z_RANGE) * 0.5f);
    line_to_current_position(manual_feedrate_mm_s.z);
}

void flash_zoffset_status()
{
    if(z_offset_status == Z_OFFSET_STATUS_HOT) {
#ifdef EN_ZOFFSET_PREHEAT
        if(((abs((int)((int)thermalManager.temp_hotend[uiCfg.curSprayerChoose].celsius - BLTOUCH_HEAT_EXTRUDE_TEMP)) <= 3) /*|| ((int)thermalManager.temp_hotend[uiCfg.curSprayerChoose].celsius > BLTOUCH_HEAT_EXTRUDE_TEMP)*/)
#if HAS_HEATED_BED
           && ((abs((int)((int)thermalManager.temp_bed.celsius - BLTOUCH_HEAT_BED_TEMP)) <= 1) || ((int)thermalManager.temp_bed.celsius > BLTOUCH_HEAT_BED_TEMP))
#endif
          )
#endif
        {
            if(queue.length <= (BUFSIZE - 3)) {
                z_offset_status = Z_OFFSET_STATUS_MOVE;
                lv_label_set_text(z_offset_info, leveling_menu.zoffsetcenter);

                if(!all_axes_homed()) {
                    set_all_unhomed();
                    queue.enqueue_now_P(PSTR("G28\n M420 S1"));
                } else
                    queue.enqueue_now_P(PSTR("M420 S1"));

                ZERO(public_buf_l);
                sprintf_P(public_buf_l, PSTR("G1 F5000 X%4.1f Y%4.1f"), (float)X_BED_SIZE / 2, (float)Y_BED_SIZE / 2);
                queue.enqueue_one_now(public_buf_l);
                ZERO(public_buf_l);
                queue.enqueue_now_P(PSTR("G1 Z0"));
                move_delay_count = 0;
            }
        }
    }

    if(all_axes_homed() && current_position.z == 0 && dataSavedFlg == false
       && z_offset_status == Z_OFFSET_STATUS_MOVE) {
        if(move_delay_count < 3)
            move_delay_count++;
        else
            z_offset_status = Z_OFFSET_STATUS_ADJUST;
    }

    if(z_offset_status == Z_OFFSET_STATUS_ADJUST)
        draw_zoffset_value();

#ifdef EN_ZOFFSET_PREHEAT
    lv_draw_sprayer_temp(labExtraTemp);
#if HAS_HEATED_BED
    lv_draw_bed_temp(labBedTemp);
#endif
#endif

}

static void event_handler(lv_obj_t * obj, lv_event_t event)
{
    if(event == LV_EVENT_RELEASED) {
        switch(obj->mks_obj_id) {
        case ID_ZOFFSET_ADD:
        case ID_ZOFFSET_DEC:
            if(z_offset_status == Z_OFFSET_STATUS_SAVE)
                z_offset_status = Z_OFFSET_STATUS_ADJUST;

            if(z_offset_status == Z_OFFSET_STATUS_ADJUST) {
                if(obj->mks_obj_id == ID_ZOFFSET_ADD) {
                    if(probe.offset.z < LCD_PROBE_Z_RANGE / 2)
                        z_offset_value = 1;
                } else {
                    if(probe.offset.z > -LCD_PROBE_Z_RANGE / 2)
                        z_offset_value = -1;
                }

                dataSaveFlg = true;
                dataSavedFlg = false;
                set_zoffset_value();
                draw_zoffset_value();
            }
            break;
        case ID_ZOFFSET_RETURN:
#ifdef EN_ZOFFSET_PREHEAT
            thermalManager.temp_hotend[uiCfg.curSprayerChoose].target = uiCfg.desireSprayerTempBak;
            thermalManager.start_watching_hotend(uiCfg.curSprayerChoose);
#if HAS_HEATED_BED
            thermalManager.temp_bed.target = uiCfg.desireBedTempBak;
            thermalManager.start_watching_bed();
#endif
#endif

            if(dataSavedFlg == false)
                probe.offset.z = z_offset_back;

            if(z_offset_status > Z_OFFSET_STATUS_START)
                queue.inject_P(PSTR("G28"));

            clear_cur_ui();
            draw_return_ui();
            break;
        case ID_ZOFFSET_STEP:
            z_offset_step++;
            z_offset_step %= 3;
            draw_zoffset_dist();
            break;
        case ID_ZOFFSET_CONFIRM:
            if(dataSaveFlg) {
                dataSaveFlg = 0;
                dataSavedFlg = true;
                z_offset_status = Z_OFFSET_STATUS_SAVE;
                settings.save();
                lv_label_set_text(z_offset_info, leveling_menu.saveok);
            }
            break;
        case ID_ZOFFSET_START:
#ifdef EN_ZOFFSET_PREHEAT
            thermalManager.temp_hotend[uiCfg.curSprayerChoose].target = BLTOUCH_HEAT_EXTRUDE_TEMP;
            thermalManager.start_watching_hotend(uiCfg.curSprayerChoose);
#if HAS_HEATED_BED
            thermalManager.temp_bed.target = BLTOUCH_HEAT_BED_TEMP;
            thermalManager.start_watching_bed();
#endif
            lv_label_set_text(z_offset_info, leveling_menu.level_heat);
#endif
            lv_imgbtn_set_src(buttonStart, LV_BTN_STATE_REL, "F:/bmp_start_dis.bin");
            lv_obj_set_click(buttonStart, 0);
            z_offset_status = Z_OFFSET_STATUS_HOT;
            break;
        }
    }
}

void lv_draw_auto_level_offset_settings(void)
{
    lv_obj_t *buttonAdd, *buttonDec, *buttonConfirm;
    lv_obj_t *buttonBack, *labelBack;

    if(disp_state_stack._disp_state[disp_state_stack._disp_index] != NOZZLE_PROBE_OFFSET_UI) {
        disp_state_stack._disp_index++;
        disp_state_stack._disp_state[disp_state_stack._disp_index] = NOZZLE_PROBE_OFFSET_UI;
    }
    disp_state = NOZZLE_PROBE_OFFSET_UI;

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
    buttonConfirm = lv_imgbtn_create(scr, NULL);
    buttonBack = lv_imgbtn_create(scr, NULL);
    buttonStart = lv_imgbtn_create(scr, NULL);

    // Create image buttons
    lv_obj_set_event_cb_mks(buttonAdd, event_handler, ID_ZOFFSET_ADD, NULL, 0);
    lv_imgbtn_set_src(buttonAdd, LV_BTN_STATE_REL, "F:/bmp_zAdd.bin");
    lv_imgbtn_set_src(buttonAdd, LV_BTN_STATE_PR, "F:/bmp_clear.bin");
    lv_imgbtn_set_style(buttonAdd, LV_BTN_STATE_PR, &tft_style_label_pre);
    lv_imgbtn_set_style(buttonAdd, LV_BTN_STATE_REL, &tft_style_label_rel);
    lv_obj_clear_protect(buttonAdd, LV_PROTECT_FOLLOW);

    lv_obj_set_event_cb_mks(buttonDec, event_handler, ID_ZOFFSET_DEC, NULL, 0);
    lv_imgbtn_set_src(buttonDec, LV_BTN_STATE_REL, "F:/bmp_zDec.bin");
    lv_imgbtn_set_src(buttonDec, LV_BTN_STATE_PR, "F:/bmp_clear.bin");
    lv_imgbtn_set_style(buttonDec, LV_BTN_STATE_PR, &tft_style_label_pre);
    lv_imgbtn_set_style(buttonDec, LV_BTN_STATE_REL, &tft_style_label_rel);

    lv_obj_set_event_cb_mks(buttonStart, event_handler, ID_ZOFFSET_START, NULL, 0);
    lv_imgbtn_set_src(buttonStart, LV_BTN_STATE_REL, "F:/bmp_start.bin");
    lv_imgbtn_set_src(buttonStart, LV_BTN_STATE_PR, "F:/bmp_clear.bin");
    lv_imgbtn_set_style(buttonStart, LV_BTN_STATE_PR, &tft_style_label_pre);
    lv_imgbtn_set_style(buttonStart, LV_BTN_STATE_REL, &tft_style_label_rel);

    lv_obj_set_event_cb_mks(buttonBack, event_handler, ID_ZOFFSET_RETURN, NULL, 0);
    lv_imgbtn_set_src(buttonBack, LV_BTN_STATE_REL, "F:/bmp_return.bin");
    lv_imgbtn_set_src(buttonBack, LV_BTN_STATE_PR, "F:/bmp_clear.bin");
    lv_imgbtn_set_style(buttonBack, LV_BTN_STATE_PR, &tft_style_label_pre);
    lv_imgbtn_set_style(buttonBack, LV_BTN_STATE_REL, &tft_style_label_rel);

    lv_obj_set_event_cb_mks(buttonConfirm, event_handler, ID_ZOFFSET_CONFIRM, NULL, 0);
    lv_imgbtn_set_src(buttonConfirm, LV_BTN_STATE_REL, "F:/bmp_confirm.bin");
    lv_imgbtn_set_src(buttonConfirm, LV_BTN_STATE_PR, "F:/bmp_clear.bin");
    lv_imgbtn_set_style(buttonConfirm, LV_BTN_STATE_PR, &tft_style_label_pre);
    lv_imgbtn_set_style(buttonConfirm, LV_BTN_STATE_REL, &tft_style_label_rel);

    lv_obj_set_event_cb_mks(buttonStep, event_handler, ID_ZOFFSET_STEP, NULL, 0);
    lv_imgbtn_set_style(buttonStep, LV_BTN_STATE_PR, &tft_style_label_pre);
    lv_imgbtn_set_style(buttonStep, LV_BTN_STATE_REL, &tft_style_label_rel);

    lv_obj_set_pos(buttonAdd, BTN_ROW2_COL2_POS);
    lv_obj_set_pos(buttonDec, BTN_ROW2_COL1_POS);
    lv_obj_set_pos(buttonStep, BTN_ROW2_COL3_POS);
    lv_obj_set_pos(buttonBack, BTN_ROW2_COL4_POS);
    lv_obj_set_pos(buttonConfirm, BTN_ROW1_COL4_POS);
    lv_obj_set_pos(buttonStart, BTN_ROW1_COL1_POS);

    // Create labels on the image buttons
    lv_btn_set_layout(buttonAdd, LV_LAYOUT_OFF);
    lv_btn_set_layout(buttonDec, LV_LAYOUT_OFF);
    lv_btn_set_layout(buttonBack, LV_LAYOUT_OFF);
    lv_btn_set_layout(buttonStep, LV_LAYOUT_OFF);
    lv_btn_set_layout(buttonConfirm, LV_LAYOUT_OFF);
    lv_btn_set_layout(buttonStart, LV_LAYOUT_OFF);

    lv_obj_t *labelAdd = lv_label_create(buttonAdd, NULL);
    lv_obj_t *labelDec = lv_label_create(buttonDec, NULL);
    lv_obj_t *labelConfirm = lv_label_create(buttonConfirm, NULL);
    labelStart = lv_label_create(buttonStart, NULL);
    labelBack = lv_label_create(buttonBack, NULL);
    labelStep = lv_label_create(buttonStep, NULL);

    if(gCfgItems.multiple_language != 0) {
        lv_label_set_text(labelAdd, move_menu.z_add);
        lv_obj_align(labelAdd, buttonAdd, LV_ALIGN_IN_BOTTOM_MID, 0, BUTTON_TEXT_Y_OFFSET);

        lv_label_set_text(labelDec, move_menu.z_dec);
        lv_obj_align(labelDec, buttonDec, LV_ALIGN_IN_BOTTOM_MID, 0, BUTTON_TEXT_Y_OFFSET);

        lv_label_set_text(labelBack, common_menu.text_back);
        lv_obj_align(labelBack, buttonBack, LV_ALIGN_IN_BOTTOM_MID, 0, BUTTON_TEXT_Y_OFFSET);

        lv_label_set_text(labelConfirm, common_menu.save);
        lv_obj_align(labelConfirm, buttonConfirm, LV_ALIGN_IN_BOTTOM_MID, 0, BUTTON_TEXT_Y_OFFSET);

        lv_label_set_text(labelStart, print_file_dialog_menu.start);
        lv_obj_align(labelStart, buttonStart, LV_ALIGN_IN_BOTTOM_MID, 0, BUTTON_TEXT_Y_OFFSET);
    }

    z_offset_step = 0;
    draw_zoffset_dist();

#ifdef EN_ZOFFSET_PREHEAT
    z_offset_info = lv_label_create(scr, NULL);
    lv_label_set_long_mode(z_offset_info, LV_LABEL_LONG_BREAK);
    lv_obj_set_pos(z_offset_info, 140, 40);
    lv_obj_set_size(z_offset_info, 220, 0);
    lv_obj_set_style(z_offset_info, &style_yellow_label);
    lv_label_set_align(z_offset_info, LV_LABEL_ALIGN_LEFT);

    z_offset_value_disp = lv_label_create(scr, NULL);
    lv_label_set_long_mode(z_offset_value_disp, LV_LABEL_LONG_BREAK);
    lv_obj_set_pos(z_offset_value_disp, 140, 70);
    lv_obj_set_size(z_offset_value_disp, 220, 0);
    lv_obj_set_style(z_offset_value_disp, &style_yellow_label);
    lv_label_set_align(z_offset_value_disp, LV_LABEL_ALIGN_LEFT);
    draw_zoffset_value();

    labExtraTemp = lv_label_create(scr, NULL);
    lv_label_set_long_mode(labExtraTemp, LV_LABEL_LONG_BREAK);
    lv_obj_set_style(labExtraTemp, &tft_style_label_rel);
    lv_obj_set_pos(labExtraTemp, 140, 100);
    lv_obj_set_size(labExtraTemp, 220, 0);
    lv_label_set_align(labExtraTemp, LV_LABEL_ALIGN_LEFT);
    lv_draw_sprayer_temp(labExtraTemp);

#if HAS_HEATED_BED
    labBedTemp = lv_label_create(scr, NULL);
    lv_label_set_long_mode(labBedTemp, LV_LABEL_LONG_BREAK);
    lv_obj_set_style(labBedTemp, &tft_style_label_rel);
    lv_obj_set_pos(labBedTemp, 140, 130);
    lv_obj_set_size(labBedTemp, 220, 0);
    lv_label_set_align(labBedTemp, LV_LABEL_ALIGN_LEFT);
    lv_draw_bed_temp(labBedTemp);
#endif

    uiCfg.desireSprayerTempBak = thermalManager.temp_hotend[uiCfg.curSprayerChoose].target;
    uiCfg.desireBedTempBak = thermalManager.temp_bed.target;
#else
    z_offset_info = lv_label_create(scr, NULL);
    lv_label_set_long_mode(z_offset_info, LV_LABEL_LONG_BREAK);
    lv_obj_set_pos(z_offset_info, 130, 60);
    lv_obj_set_size(z_offset_info, 240, 0);
    lv_obj_set_style(z_offset_info, &style_yellow_label);
    lv_label_set_align(z_offset_info, LV_LABEL_ALIGN_LEFT);

    z_offset_value_disp = lv_label_create(scr, NULL);
    lv_label_set_long_mode(z_offset_value_disp, LV_LABEL_LONG_BREAK);
    lv_obj_set_pos(z_offset_value_disp, 130, 110);
    lv_obj_set_size(z_offset_value_disp, 240, 0);
    lv_obj_set_style(z_offset_value_disp, &style_yellow_label);
    lv_label_set_align(z_offset_value_disp, LV_LABEL_ALIGN_LEFT);
    draw_zoffset_value();
#endif

    z_offset_back = probe.offset.z;
    dataSaveFlg = 0;
    dataSavedFlg = false;
    z_offset_status = Z_OFFSET_STATUS_START;
    lv_label_set_text(z_offset_info, leveling_menu.zoffsetstart);
}

void lv_clear_auto_level_offset_settings()
{
    lv_obj_del(scr);
}

#endif // HAS_TFT_LVGL_UI && HAS_BED_PROBE
