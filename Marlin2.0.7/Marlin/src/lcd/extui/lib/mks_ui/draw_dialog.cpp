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

/**
 * draw_dialog.cpp
 */

#include "../../../../inc/MarlinConfigPre.h"

#if HAS_TFT_LVGL_UI

#include "lv_conf.h"
#include "draw_ui.h"

//#include "../lvgl/src/lv_objx/lv_imgbtn.h"
//#include "../lvgl/src/lv_objx/lv_img.h"
//#include "../lvgl/src/lv_core/lv_disp.h"
//#include "../lvgl/src/lv_core/lv_refr.h"

#include "../../../../MarlinCore.h"
#include "../../../../sd/cardreader.h"
#include "../../../../gcode/queue.h"
#include "../../../../module/temperature.h"
#include "../../../../module/planner.h"
#include "../../../../gcode/gcode.h"
#include "../../../../module/motion.h"
#include "../../../../feature/bedlevel/bedlevel.h"
#include "../../../../module/settings.h"
#include "../../../../libs/buzzer.h"

#if ENABLED(POWER_LOSS_RECOVERY)
#include "../../../../feature/powerloss.h"
#endif

#if ENABLED(BLTOUCH)
#include "../../../../feature/bltouch.h"
#endif

#if ENABLED(PARK_HEAD_ON_PAUSE)
#include "../../../../feature/pause.h"
#endif
#include "../../../../gcode/gcode.h"

extern lv_group_t * g;
static lv_obj_t * scr;
static lv_obj_t * labExtraTemp;
static lv_obj_t * filament_bar;
static lv_obj_t * labSensorCheck;

#define CHECK_OVERTIME_NUM 300  //second
#define SENSOR_TARGET_CNT			10  //times

static bool pre_sensor_status = false;
static uint8_t sensor_target_cnt = 0;
static uint16_t sensor_check_overtime = 0;
static bool f_init_sensor = 0;
static lv_obj_t *print_hotend_temp;
static lv_obj_t *print_bed_temp;

extern uint8_t sel_id;
extern uint8_t once_flag;
extern uint8_t gcode_preview_over;
extern int upload_result ;
extern uint32_t upload_time;
extern uint32_t upload_size;
extern uint8_t temperature_change_frequency;
extern int16_t saved_feedrate_percentage;

static void btn_ok_event_cb(lv_obj_t * btn, lv_event_t event)
{
    if(event == LV_EVENT_CLICKED) {
        // nothing to do
    } else if(event == LV_EVENT_RELEASED) {
        if(uiCfg.dialogType == DIALOG_TYPE_PRINT_FILE) {
#if HAS_GCODE_PREVIEW
            preview_gcode_prehandle(list_file.file_name[sel_id]);
#endif
            reset_print_time();
            start_print_time();
            uiCfg.print_open_led = 1;
            uiCfg.print_state = WORKING;
						uiCfg.stop_reprint_step	= 0;
            lv_clear_dialog();
            lv_draw_printing();

#if ENABLED(SDSUPPORT)
            if(gcode_preview_over != 1) {
                char *cur_name;
                cur_name = strrchr(list_file.file_name[sel_id], '/');

                SdFile file, *curDir;
                card.endFilePrint();
                const char * const fname = card.diveToFile(true, curDir, cur_name);
                if(!fname) return;
                if(file.open(curDir, fname, O_READ)) {
                    gCfgItems.curFilesize = file.fileSize();
                    file.close();
                    update_spi_flash();
                }
                card.openFileRead(cur_name);
                if(card.isFileOpen()) {
                    feedrate_percentage = 100;
                    saved_feedrate_percentage = feedrate_percentage;
                    planner.flow_percentage[0] = 100;
                    planner.e_factor[0]        = planner.flow_percentage[0] * 0.01f;
#if HAS_MULTI_EXTRUDER
                    planner.flow_percentage[1] = 100;
                    planner.e_factor[1]        = planner.flow_percentage[1] * 0.01f;
#endif
                    card.startFileprint();
#if ENABLED(POWER_LOSS_RECOVERY)
                    recovery.prepare();
#endif
                    once_flag = 0;
                }
            }
#endif
        } else if(uiCfg.dialogType == DIALOG_TYPE_FILAMENT_CHANGE) {
            uiCfg.moveSpeed_bak = (uint16_t)feedrate_mm_s;
            uiCfg.desireSprayerTempBak = thermalManager.temp_hotend[active_extruder].target;
            stop_print_time();
            clear_cur_ui();
            lv_draw_filament_change();

#if ENABLED(SDSUPPORT)
            card.pauseSDPrint();
            uiCfg.print_state = PAUSING;
#endif
        } else if(uiCfg.dialogType == DIALOG_TYPE_PAUSE) {
            uiCfg.moveSpeed_bak = (uint16_t)feedrate_mm_s;
            stop_print_time();
            clear_cur_ui();
            draw_return_ui();
#if ENABLED(SDSUPPORT)
            card.pauseSDPrint();
            uiCfg.print_state = PAUSING;
#endif
        } else if(uiCfg.dialogType == DIALOG_TYPE_STOP) {
            wait_for_heatup = false;
            recovery.purge();
            stop_print_time();
            lv_clear_dialog();
            lv_draw_ready_print();

#if ENABLED(SDSUPPORT)
            //card.endFilePrint();
            //wait_for_heatup = false;
            uiCfg.print_state           = IDLE;
            card.flag.abort_sd_printing = true;
            uiCfg.stop_reprint_step = 1;
						uiCfg.e_relative = 0;
            //queue.clear();
            //quickstop_stepper();
            //print_job_timer.stop();
            //thermalManager.disable_all_heaters();

            //#if ENABLED(POWER_LOSS_RECOVERY)
            //  recovery.purge();
            //#endif
            //queue.enqueue_now_P(PSTR("G91\nG1 Z10\nG90\nG28 X0 Y0"));
            //queue.inject_P(PSTR("G91\nG1 Z10\nG90\nG28 X0 Y0\nM84\nM107"));
#endif
        } else if(uiCfg.dialogType == DIALOG_TYPE_FINISH_PRINT) {
						uiCfg.dialog_temp_display = 0;
						clear_cur_ui();
            lv_draw_ready_print();
            preHotCurOpt = ID_P_COOL;
            recovery.purge();
						uiCfg.e_relative = 0;
        }else if(uiCfg.dialogType == DIALOG_TYPE_PRINT_CONTINUE){
						wait_for_user = false;
						clear_cur_ui();
            draw_return_ui();
        }
#if ENABLED(ADVANCED_PAUSE_FEATURE)
        else if(uiCfg.dialogType == DIALOG_PAUSE_MESSAGE_WAITING
                || uiCfg.dialogType == DIALOG_PAUSE_MESSAGE_INSERT
                || uiCfg.dialogType == DIALOG_PAUSE_MESSAGE_HEAT
               ) {
            wait_for_user = false;
        } else if(uiCfg.dialogType == DIALOG_PAUSE_MESSAGE_OPTION) {
            pause_menu_response = PAUSE_RESPONSE_EXTRUDE_MORE;
        } else if(uiCfg.dialogType == DIALOG_PAUSE_MESSAGE_RESUME) {
            clear_cur_ui();
            draw_return_ui();
        }
#endif
        else if(uiCfg.dialogType == DIALOG_STORE_EEPROM_TIPS) {
            gcode.process_subcommands_now_P(PSTR("M500"));
            clear_cur_ui();
            draw_return_ui();
        } else if(uiCfg.dialogType == DIALOG_READ_EEPROM_TIPS) {
            gcode.process_subcommands_now_P(PSTR("M501"));
            clear_cur_ui();
            draw_return_ui();
        } else if(uiCfg.dialogType == DIALOG_REVERT_EEPROM_TIPS) {
            gcode.process_subcommands_now_P(PSTR("M502"));
            clear_cur_ui();
            draw_return_ui();
        } else if(uiCfg.dialogType == DIALOG_RESTORE_DATA) {
#ifdef RESTORE_LEVELING_DATA
            reset_bed_level();
#else
            factory_reset_bed_level();
#endif
            gCfgItems.spi_flash_flag = FLASH_INF_VALID_FLAG + 1;
            gCfgItems_init();
            buzzer.lvgl_tone();
            gcode.process_subcommands_now_P(PSTR("M500\nM997"));
            clear_cur_ui();
            draw_return_ui();
        } else if(uiCfg.dialogType == DIALOG_WIFI_CONFIG_TIPS) {
            uiCfg.configWifi = 1;
            clear_cur_ui();
            draw_return_ui();
        } else if(uiCfg.dialogType == DIALOG_TYPE_FILAMENT_HEAT_LOAD_COMPLETED) {
            uiCfg.filament_heat_completed_load = 1;
        } else if(uiCfg.dialogType == DIALOG_TYPE_FILAMENT_HEAT_UNLOAD_COMPLETED) {
            uiCfg.filament_heat_completed_unload = 1;
        } else if(uiCfg.dialogType == DIALOG_TYPE_FILAMENT_LOAD_COMPLETED
                  || uiCfg.dialogType == DIALOG_TYPE_FILAMENT_UNLOAD_COMPLETED
                 ) {
            clear_cur_ui();
            draw_return_ui();
        }
    }
}

static void btn_cancel_event_cb(lv_obj_t * btn, lv_event_t event)
{
    if(event == LV_EVENT_CLICKED) {
        // nothing to do
    } else if(event == LV_EVENT_RELEASED) {
        if(uiCfg.dialogType == DIALOG_PAUSE_MESSAGE_OPTION) {
#if ENABLED(ADVANCED_PAUSE_FEATURE)
            pause_menu_response = PAUSE_RESPONSE_RESUME_PRINT;
#endif
        } else if((uiCfg.dialogType      == DIALOG_TYPE_FILAMENT_LOAD_HEAT)
                  || (uiCfg.dialogType == DIALOG_TYPE_FILAMENT_UNLOAD_HEAT)
                  || (uiCfg.dialogType == DIALOG_TYPE_FILAMENT_HEAT_LOAD_COMPLETED)
                  || (uiCfg.dialogType == DIALOG_TYPE_FILAMENT_HEAT_UNLOAD_COMPLETED)
                 ) {
            thermalManager.temp_hotend[uiCfg.curSprayerChoose].target = uiCfg.desireSprayerTempBak;
            thermalManager.start_watching_hotend(uiCfg.curSprayerChoose);
            clear_cur_ui();
            draw_return_ui();
						uiCfg.filament_loading_time_flg = 0;
        } else if((uiCfg.dialogType   == DIALOG_TYPE_FILAMENT_LOADING)
                  || (uiCfg.dialogType == DIALOG_TYPE_FILAMENT_UNLOADING)
                 ) {
            queue.enqueue_one_P(PSTR("M410"));
            uiCfg.filament_rate                = 0;
            uiCfg.filament_loading_completed   = 0;
            uiCfg.filament_unloading_completed = 0;
            uiCfg.filament_loading_time_flg    = 0;
            uiCfg.filament_loading_time_cnt    = 0;
            uiCfg.filament_unloading_time_flg  = 0;
            uiCfg.filament_unloading_time_cnt  = 0;
            thermalManager.temp_hotend[uiCfg.curSprayerChoose].target = uiCfg.desireSprayerTempBak;
            thermalManager.start_watching_hotend(uiCfg.curSprayerChoose);
            clear_cur_ui();
            draw_return_ui();
        } else if(uiCfg.dialogType == DIALOG_CHECK_SENSOR) {
            pre_sensor_status = false;
            sensor_target_cnt = 0;
            sensor_check_overtime = 0;
            f_init_sensor = 0;
            clear_cur_ui();
            draw_return_ui();
        } else {
						uiCfg.filament_loading_time_flg = 0;
            clear_cur_ui();
            draw_return_ui();
        }
    }
}

void lv_draw_dialog(uint8_t type)
{

    lv_obj_t * btnOk = NULL;
    lv_obj_t * btnCancel = NULL;

    if(disp_state_stack._disp_state[disp_state_stack._disp_index] != DIALOG_UI) {
        disp_state_stack._disp_index++;
        disp_state_stack._disp_state[disp_state_stack._disp_index] = DIALOG_UI;
    }
    disp_state = DIALOG_UI;

    uiCfg.dialogType = type;
    scr = lv_obj_create(NULL, NULL);
    lv_obj_set_style(scr, &tft_style_scr);
    lv_scr_load(scr);
    lv_obj_clean(scr);

    lv_obj_t * title = lv_label_create(scr, NULL);
    lv_obj_set_style(title, &tft_style_label_rel);
    lv_obj_set_pos(title, TITLE_XPOS, TITLE_YPOS);
    lv_label_set_text(title, creat_title_text());
    lv_refr_now(lv_refr_get_disp_refreshing());

		print_hotend_temp = lv_label_create(scr, NULL);
		print_bed_temp = lv_label_create(scr, NULL);
		lv_obj_set_style(print_hotend_temp, &tft_style_label_rel);
		lv_obj_set_style(print_bed_temp, &tft_style_label_rel);
		lv_obj_set_hidden(print_hotend_temp, 1);
		lv_obj_set_hidden(print_bed_temp, 1);

    static lv_style_t style_btn_rel;                                 // A variable to store the released style
    lv_style_copy(&style_btn_rel, &lv_style_plain);                  // Initialize from a built-in style
    style_btn_rel.body.border.color = lv_color_hex3(0x269);
    style_btn_rel.body.border.width = 1;
    style_btn_rel.body.main_color   = lv_color_hex3(0xADF);
    style_btn_rel.body.grad_color   = lv_color_hex3(0x46B);
    style_btn_rel.body.shadow.width = 4;
    style_btn_rel.body.shadow.type  = LV_SHADOW_BOTTOM;
    style_btn_rel.body.radius       = LV_RADIUS_CIRCLE;
    style_btn_rel.text.color        = LV_COLOR_TEXT;
    style_btn_rel.text.font         = &TERN(HAS_SPI_FLASH_FONT, gb2312_puhui32, lv_font_roboto_22);

    static lv_style_t style_btn_pr;                                  // A variable to store the pressed style
    lv_style_copy(&style_btn_pr, &style_btn_rel);                    // Initialize from the released style
    style_btn_pr.body.border.color = lv_color_hex3(0x46B);
    style_btn_pr.body.main_color   = lv_color_hex3(0x8BD);
    style_btn_pr.body.grad_color   = lv_color_hex3(0x24A);
    style_btn_pr.body.shadow.width = 2;
    style_btn_pr.text.color        = lv_color_hex3(0xBCD);
    style_btn_pr.text.font         = &TERN(HAS_SPI_FLASH_FONT, gb2312_puhui32, lv_font_roboto_22);

    lv_obj_t *labelDialog = lv_label_create(scr, NULL);
    lv_obj_set_style(labelDialog, &tft_style_label_rel);

    if(uiCfg.dialogType == DIALOG_TYPE_FINISH_PRINT || uiCfg.dialogType == DIALOG_PAUSE_MESSAGE_RESUME) {
        btnOk = lv_btn_create(scr, NULL);                   // Add a button the current screen
        lv_obj_set_pos(btnOk, BTN_OK_X + 90, BTN_OK_Y);                // Set its position
        lv_obj_set_size(btnOk, 100, 50);                               // Set its size
        lv_obj_set_event_cb(btnOk, btn_ok_event_cb);
        lv_btn_set_style(btnOk, LV_BTN_STYLE_REL, &style_btn_rel);     // Set the button's released style
        lv_btn_set_style(btnOk, LV_BTN_STYLE_PR, &style_btn_pr);       // Set the button's pressed style
        lv_obj_t *labelOk = lv_label_create(btnOk, NULL);             // Add a label to the button
        lv_label_set_text(labelOk, print_file_dialog_menu.confirm);    // Set the labels text
    } else if(uiCfg.dialogType == DIALOG_PAUSE_MESSAGE_WAITING
              || uiCfg.dialogType == DIALOG_PAUSE_MESSAGE_INSERT
              || uiCfg.dialogType == DIALOG_PAUSE_MESSAGE_HEAT
             ) {
        btnOk = lv_btn_create(scr, NULL);                   // Add a button the current screen
        lv_obj_set_pos(btnOk, BTN_OK_X + 90, BTN_OK_Y);                // Set its position
        lv_obj_set_size(btnOk, 100, 50);                               // Set its size
        lv_obj_set_event_cb(btnOk, btn_ok_event_cb);
        lv_btn_set_style(btnOk, LV_BTN_STYLE_REL, &style_btn_rel);     // Set the button's released style
        lv_btn_set_style(btnOk, LV_BTN_STYLE_PR, &style_btn_pr);       // Set the button's pressed style
        lv_obj_t *labelOk = lv_label_create(btnOk, NULL);             // Add a label to the button
        lv_label_set_text(labelOk, print_file_dialog_menu.confirm);    // Set the labels text
    } else if(uiCfg.dialogType == DIALOG_PAUSE_MESSAGE_PAUSING
              || uiCfg.dialogType == DIALOG_PAUSE_MESSAGE_UNLOAD
              || uiCfg.dialogType == DIALOG_PAUSE_MESSAGE_LOAD
              || uiCfg.dialogType == DIALOG_PAUSE_MESSAGE_PURGE
              || uiCfg.dialogType == DIALOG_PAUSE_MESSAGE_RESUME
              || uiCfg.dialogType == DIALOG_PAUSE_MESSAGE_HEATING
             ) {
        // nothing to do
    }else if(uiCfg.dialogType == DIALOG_TYPE_PRINT_CONTINUE){
			 	btnOk = lv_btn_create(scr, NULL);
        lv_obj_set_pos(btnOk, BTN_OK_X + 90, BTN_OK_Y);
        lv_obj_set_size(btnOk, 100, 50);
        lv_obj_set_event_cb(btnOk, btn_ok_event_cb);
        lv_btn_set_style(btnOk, LV_BTN_STYLE_REL, &style_btn_rel);
        lv_btn_set_style(btnOk, LV_BTN_STYLE_PR, &style_btn_pr);
        lv_obj_t *labelOk = lv_label_create(btnOk, NULL);
        lv_label_set_text(labelOk, print_file_dialog_menu.confirm);
    } else if(uiCfg.dialogType == DIALOG_PAUSE_MESSAGE_CHANGING) {
        btnOk = lv_btn_create(scr, NULL);
        lv_obj_set_pos(btnOk, BTN_OK_X + 90, BTN_OK_Y);
        lv_obj_set_size(btnOk, 100, 50);
        lv_obj_set_event_cb(btnOk, btn_cancel_event_cb);
        lv_btn_set_style(btnOk, LV_BTN_STYLE_REL, &style_btn_rel);
        lv_btn_set_style(btnOk, LV_BTN_STYLE_PR, &style_btn_pr);
        lv_obj_t *labelOk = lv_label_create(btnOk, NULL);
        lv_label_set_text(labelOk, print_file_dialog_menu.confirm);
    } else if(uiCfg.dialogType == WIFI_ENABLE_TIPS) {
        btnCancel = lv_btn_create(scr, NULL);
        lv_obj_set_pos(btnCancel, BTN_OK_X + 90, BTN_OK_Y);
        lv_obj_set_size(btnCancel, 100, 50);
        lv_obj_set_event_cb(btnCancel, btn_cancel_event_cb);
        lv_btn_set_style(btnCancel, LV_BTN_STYLE_REL, &style_btn_rel);
        lv_btn_set_style(btnCancel, LV_BTN_STYLE_PR, &style_btn_pr);
        lv_obj_t *labelCancel = lv_label_create(btnCancel, NULL);
        lv_label_set_text(labelCancel, print_file_dialog_menu.cancle);
    } else if(uiCfg.dialogType == DIALOG_TRANSFER_NO_DEVICE) {
        btnCancel = lv_btn_create(scr, NULL);
        lv_obj_set_pos(btnCancel, BTN_OK_X + 90, BTN_OK_Y);
        lv_obj_set_size(btnCancel, 100, 50);
        lv_obj_set_event_cb(btnCancel, btn_cancel_event_cb);
        lv_btn_set_style(btnCancel, LV_BTN_STYLE_REL, &style_btn_rel);
        lv_btn_set_style(btnCancel, LV_BTN_STYLE_PR, &style_btn_pr);
        lv_obj_t *labelCancel = lv_label_create(btnCancel, NULL);
        lv_label_set_text(labelCancel, print_file_dialog_menu.cancle);
    }
#if ENABLED(USE_WIFI_FUNCTION)
    else if(uiCfg.dialogType == DIALOG_TYPE_UPLOAD_FILE) {
        if(upload_result == 2) {
            btnCancel = lv_btn_create(scr, NULL);
            lv_obj_set_pos(btnCancel, BTN_OK_X + 90, BTN_OK_Y);
            lv_obj_set_size(btnCancel, 100, 50);
            lv_obj_set_event_cb(btnCancel, btn_cancel_event_cb);
            lv_btn_set_style(btnCancel, LV_BTN_STYLE_REL, &style_btn_rel);
            lv_btn_set_style(btnCancel, LV_BTN_STYLE_PR, &style_btn_pr);
            lv_obj_t *labelCancel = lv_label_create(btnCancel, NULL);
            lv_label_set_text(labelCancel, print_file_dialog_menu.cancle);
        } else if(upload_result == 3) {
            btnOk = lv_btn_create(scr, NULL);
            lv_obj_set_pos(btnOk, BTN_OK_X + 90, BTN_OK_Y);
            lv_obj_set_size(btnOk, 100, 50);
            lv_obj_set_event_cb(btnOk, btn_ok_event_cb);
            lv_btn_set_style(btnOk, LV_BTN_STYLE_REL, &style_btn_rel);
            lv_btn_set_style(btnOk, LV_BTN_STYLE_PR, &style_btn_pr);
            lv_obj_t *labelOk = lv_label_create(btnOk, NULL);
            lv_label_set_text(labelOk, print_file_dialog_menu.confirm);
        }
    }
#endif //USE_WIFI_FUNCTION
    else if(uiCfg.dialogType == DIALOG_TYPE_FILAMENT_LOAD_HEAT
            || uiCfg.dialogType == DIALOG_TYPE_FILAMENT_UNLOAD_HEAT
           ) {
        btnCancel = lv_btn_create(scr, NULL);
        lv_obj_set_pos(btnCancel, BTN_OK_X + 90, BTN_OK_Y);
        lv_obj_set_size(btnCancel, 100, 50);
        lv_obj_set_event_cb(btnCancel, btn_cancel_event_cb);
        lv_btn_set_style(btnCancel, LV_BTN_STYLE_REL, &style_btn_rel);
        lv_btn_set_style(btnCancel, LV_BTN_STYLE_PR, &style_btn_pr);
        lv_obj_t *labelCancel = lv_label_create(btnCancel, NULL);
        lv_label_set_text(labelCancel, print_file_dialog_menu.cancle);

        labExtraTemp = lv_label_create(scr, NULL);
        lv_obj_set_style(labExtraTemp, &tft_style_label_rel);
        filament_sprayer_temp();
    } else if(uiCfg.dialogType == DIALOG_CHECK_SENSOR) {
        lv_obj_t *imgSensorChk = lv_img_create(scr, NULL);
        lv_obj_align(imgSensorChk, NULL, LV_ALIGN_IN_TOP_MID, 0, 45);
        lv_img_set_src(imgSensorChk, "F:/bmp_sensor_check.bin");

        btnCancel = lv_btn_create(scr, NULL);
        lv_obj_set_size(btnCancel, 100, 50);
        lv_obj_align(btnCancel, imgSensorChk, LV_ALIGN_OUT_BOTTOM_MID, 0, 130);
        lv_obj_set_event_cb(btnCancel, btn_cancel_event_cb);
        lv_btn_set_style(btnCancel, LV_BTN_STYLE_REL, &style_btn_rel);
        lv_btn_set_style(btnCancel, LV_BTN_STYLE_PR, &style_btn_pr);
        lv_obj_t *labelCancel = lv_label_create(btnCancel, NULL);
        lv_label_set_text(labelCancel, print_file_dialog_menu.cancle);

        lv_obj_set_hidden(labelDialog, 1);

        labSensorCheck = lv_label_create(scr, NULL);
        lv_obj_set_style(labSensorCheck, &style_yellow_label);
        lv_label_set_text(labSensorCheck, leveling_menu.sensorchkbef);
        lv_obj_align(labSensorCheck, imgSensorChk, LV_ALIGN_OUT_BOTTOM_MID, 10, 20);
    } else if(uiCfg.dialogType == DIALOG_TYPE_FILAMENT_LOAD_COMPLETED
              || uiCfg.dialogType == DIALOG_TYPE_FILAMENT_UNLOAD_COMPLETED
             ) {
        btnOk = lv_btn_create(scr, NULL);
        lv_obj_set_pos(btnOk, BTN_OK_X + 90, BTN_OK_Y);
        lv_obj_set_size(btnOk, 100, 50);
        lv_obj_set_event_cb(btnOk, btn_ok_event_cb);
        lv_btn_set_style(btnOk, LV_BTN_STYLE_REL, &style_btn_rel);
        lv_btn_set_style(btnOk, LV_BTN_STYLE_PR, &style_btn_pr);
        lv_obj_t *labelOk = lv_label_create(btnOk, NULL);
        lv_label_set_text(labelOk, print_file_dialog_menu.confirm);
    } else if(uiCfg.dialogType == DIALOG_TYPE_FILAMENT_LOADING
              || uiCfg.dialogType == DIALOG_TYPE_FILAMENT_UNLOADING
             ) {
        btnCancel = lv_btn_create(scr, NULL);
        lv_obj_set_pos(btnCancel, BTN_OK_X + 90, BTN_OK_Y);
        lv_obj_set_size(btnCancel, 100, 50);
        lv_obj_set_event_cb(btnCancel, btn_cancel_event_cb);
        lv_btn_set_style(btnCancel, LV_BTN_STYLE_REL, &style_btn_rel);
        lv_btn_set_style(btnCancel, LV_BTN_STYLE_PR, &style_btn_pr);
        lv_obj_t *labelCancel = lv_label_create(btnCancel, NULL);
        lv_label_set_text(labelCancel, print_file_dialog_menu.cancle);

        filament_bar = lv_bar_create(scr, NULL);
        lv_obj_set_pos(filament_bar, (TFT_WIDTH - 400) / 2, ((TFT_HEIGHT - titleHeight) - 40) / 2);
        lv_obj_set_size(filament_bar, 400, 25);
        lv_bar_set_style(filament_bar, LV_BAR_STYLE_INDIC, &lv_bar_style_indic);
        lv_bar_set_anim_time(filament_bar, 1000);
        lv_bar_set_value(filament_bar, 0, LV_ANIM_ON);
    } else {
        btnOk = lv_btn_create(scr, NULL);                   // Add a button the current screen
        lv_obj_set_pos(btnOk, BTN_OK_X, BTN_OK_Y);                     // Set its position
        lv_obj_set_size(btnOk, 100, 50);                               // Set its size
        lv_obj_set_event_cb(btnOk, btn_ok_event_cb);
        lv_btn_set_style(btnOk, LV_BTN_STYLE_REL, &style_btn_rel);     // Set the button's released style
        lv_btn_set_style(btnOk, LV_BTN_STYLE_PR, &style_btn_pr);       // Set the button's pressed style
        lv_obj_t *labelOk = lv_label_create(btnOk, NULL);             // Add a label to the button

        btnCancel = lv_btn_create(scr, NULL);               // Add a button the current screen
        lv_obj_set_pos(btnCancel, BTN_CANCEL_X, BTN_CANCEL_Y);         // Set its position
        lv_obj_set_size(btnCancel, 100, 50);                           // Set its size
        lv_obj_set_event_cb(btnCancel, btn_cancel_event_cb);
        lv_btn_set_style(btnCancel, LV_BTN_STYLE_REL, &style_btn_rel); // Set the button's released style
        lv_btn_set_style(btnCancel, LV_BTN_STYLE_PR, &style_btn_pr);   // Set the button's pressed style
        lv_obj_t *labelCancel = lv_label_create(btnCancel, NULL);     // Add a label to the button

        if(uiCfg.dialogType == DIALOG_PAUSE_MESSAGE_OPTION) {
            lv_label_set_text(labelOk, pause_msg_menu.purgeMore);        // Set the labels text
            lv_label_set_text(labelCancel, pause_msg_menu.continuePrint);
        } else {
            lv_label_set_text(labelOk, print_file_dialog_menu.confirm);  // Set the labels text
            lv_label_set_text(labelCancel, print_file_dialog_menu.cancle);
        }
    }
    if(uiCfg.dialogType == DIALOG_TYPE_PRINT_FILE) {
        if(!leveling_is_valid()) {
            lv_obj_set_hidden(btnOk, 1);
            lv_obj_set_pos(btnCancel, BTN_OK_X + 90, BTN_OK_Y);
            lv_label_set_text(labelDialog, print_file_dialog_menu.level_invalid);
        } else
            lv_label_set_text(labelDialog, print_file_dialog_menu.print_file);

        lv_obj_align(labelDialog, NULL, LV_ALIGN_CENTER, 0, -30);

        lv_obj_t *labelFile = lv_label_create(scr, NULL);
        lv_obj_set_style(labelFile, &tft_style_label_rel);

        lv_label_set_text(labelFile, list_file.long_name[sel_id]);
        lv_obj_align(labelFile, NULL, LV_ALIGN_CENTER, 0, -60);
    } else if(uiCfg.dialogType == DIALOG_TYPE_FILAMENT_CHANGE) {
        lv_label_set_text(labelDialog, print_file_dialog_menu.change_filament);
        lv_obj_align(labelDialog, NULL, LV_ALIGN_CENTER, 0, -20);
    } else if(uiCfg.dialogType == DIALOG_TYPE_PAUSE) {
        lv_label_set_text(labelDialog, print_file_dialog_menu.pause_print);
        lv_obj_align(labelDialog, NULL, LV_ALIGN_CENTER, 0, -20);
    } else if(uiCfg.dialogType == DIALOG_TYPE_STOP) {
        lv_label_set_text(labelDialog, print_file_dialog_menu.cancle_print);
        lv_obj_align(labelDialog, NULL, LV_ALIGN_CENTER, 0, -20);
    } else if(uiCfg.dialogType == DIALOG_TYPE_FINISH_PRINT) {
        char buf[20] = {0};
        lv_obj_t *print_total_time = lv_label_create(scr, NULL);
        uiCfg.dialog_temp_display = 1;

				lv_obj_set_hidden(print_hotend_temp, 0);
				lv_obj_set_hidden(print_bed_temp, 0);

        lv_obj_set_style(print_total_time, &tft_style_label_rel);
        lv_label_set_text(labelDialog, print_file_dialog_menu.print_finish);
        lv_obj_align(labelDialog, NULL, LV_ALIGN_CENTER, 0, -80);

        public_buf_l[0] = '\0';
        strcat(public_buf_l, print_file_dialog_menu.print_time);
        sprintf_P(buf, PSTR("%d%d:%d%d:%d%d"), print_time.hours / 10, print_time.hours % 10, print_time.minutes / 10, print_time.minutes % 10, print_time.seconds / 10, print_time.seconds % 10);
        strcat(public_buf_l, buf);
        lv_label_set_text(print_total_time, public_buf_l);
        lv_obj_align(print_total_time, NULL, LV_ALIGN_CENTER, 0, -110);

    } else if(uiCfg.dialogType == DIALOG_PAUSE_MESSAGE_PAUSING) {
        lv_label_set_text(labelDialog, pause_msg_menu.pausing);
        lv_obj_align(labelDialog, NULL, LV_ALIGN_CENTER, 0, -20);
    } else if(uiCfg.dialogType == DIALOG_PAUSE_MESSAGE_CHANGING) {
        lv_label_set_text(labelDialog, pause_msg_menu.filaChange);
        lv_obj_align(labelDialog, NULL, LV_ALIGN_CENTER, 0, -20);
    } else if(uiCfg.dialogType == DIALOG_PAUSE_MESSAGE_UNLOAD) {
        lv_label_set_text(labelDialog, pause_msg_menu.unload);
        lv_obj_align(labelDialog, NULL, LV_ALIGN_CENTER, 0, -20);
    } else if(uiCfg.dialogType == DIALOG_PAUSE_MESSAGE_WAITING) {
        lv_label_set_text(labelDialog, pause_msg_menu.waiting);
        lv_obj_align(labelDialog, NULL, LV_ALIGN_CENTER, 0, -20);
    } else if(uiCfg.dialogType == DIALOG_PAUSE_MESSAGE_INSERT) {
        lv_label_set_text(labelDialog, pause_msg_menu.insert);
        lv_obj_align(labelDialog, NULL, LV_ALIGN_CENTER, 0, -20);
    } else if(uiCfg.dialogType == DIALOG_PAUSE_MESSAGE_LOAD) {
        lv_label_set_text(labelDialog, pause_msg_menu.load);
        lv_obj_align(labelDialog, NULL, LV_ALIGN_CENTER, 0, -20);
    } else if(uiCfg.dialogType == DIALOG_PAUSE_MESSAGE_PURGE) {
        lv_label_set_text(labelDialog, pause_msg_menu.purge);
        lv_obj_align(labelDialog, NULL, LV_ALIGN_CENTER, 0, -20);
    } else if(uiCfg.dialogType == DIALOG_PAUSE_MESSAGE_RESUME) {
        lv_label_set_text(labelDialog, pause_msg_menu.resume);
        lv_obj_align(labelDialog, NULL, LV_ALIGN_CENTER, 0, -20);
    } else if(uiCfg.dialogType == DIALOG_PAUSE_MESSAGE_HEAT) {
        lv_label_set_text(labelDialog, pause_msg_menu.heat);
        lv_obj_align(labelDialog, NULL, LV_ALIGN_CENTER, 0, -20);
    } else if(uiCfg.dialogType == DIALOG_PAUSE_MESSAGE_HEATING) {
        lv_label_set_text(labelDialog, pause_msg_menu.heating);
        lv_obj_align(labelDialog, NULL, LV_ALIGN_CENTER, 0, -20);
    } else if(uiCfg.dialogType == DIALOG_PAUSE_MESSAGE_OPTION) {
        lv_label_set_text(labelDialog, pause_msg_menu.option);
        lv_obj_align(labelDialog, NULL, LV_ALIGN_CENTER, 0, -20);
    } else if(uiCfg.dialogType == DIALOG_STORE_EEPROM_TIPS) {
        lv_label_set_text(labelDialog, eeprom_menu.storeTips);
        lv_obj_align(labelDialog, NULL, LV_ALIGN_CENTER, 0, -20);
    } else if(uiCfg.dialogType == DIALOG_READ_EEPROM_TIPS) {
        lv_label_set_text(labelDialog, eeprom_menu.readTips);
        lv_obj_align(labelDialog, NULL, LV_ALIGN_CENTER, 0, -20);
    } else if(uiCfg.dialogType == DIALOG_REVERT_EEPROM_TIPS) {
        lv_label_set_text(labelDialog, eeprom_menu.revertTips);
        lv_obj_align(labelDialog, NULL, LV_ALIGN_CENTER, 0, -20);
    } else if(uiCfg.dialogType == DIALOG_WIFI_CONFIG_TIPS) {
        lv_label_set_text(labelDialog, machine_menu.wifiConfigTips);
        lv_obj_align(labelDialog, NULL, LV_ALIGN_CENTER, 0, -20);
    } else if(uiCfg.dialogType == WIFI_ENABLE_TIPS) {
        lv_label_set_text(labelDialog, print_file_dialog_menu.wifi_enable_tips);
        lv_obj_align(labelDialog, NULL, LV_ALIGN_CENTER, 0, -20);
    } else if(uiCfg.dialogType == DIALOG_TRANSFER_NO_DEVICE) {
        lv_label_set_text(labelDialog, DIALOG_UPDATE_NO_DEVICE_EN);
        lv_obj_align(labelDialog, NULL, LV_ALIGN_CENTER, 0, -20);
    } else if(uiCfg.dialogType == DIALOG_RESTORE_DATA) {
        lv_label_set_text(labelDialog, common_menu.restore_warn);
        lv_obj_align(labelDialog, NULL, LV_ALIGN_CENTER, 0, -20);
    }else if(uiCfg.dialogType == DIALOG_TYPE_PRINT_CONTINUE){
		  	lv_label_set_text(labelDialog, print_file_dialog_menu.print_continue);
        lv_obj_align(labelDialog, NULL, LV_ALIGN_CENTER, 0, -20);
		}
#if ENABLED(USE_WIFI_FUNCTION)
    else if(uiCfg.dialogType == DIALOG_TYPE_UPLOAD_FILE) {
        if(upload_result == 1) {
            lv_label_set_text(labelDialog, DIALOG_UPLOAD_ING_EN);
            lv_obj_align(labelDialog, NULL, LV_ALIGN_CENTER, 0, -20);
        } else if(upload_result == 2) {
            lv_label_set_text(labelDialog, DIALOG_UPLOAD_ERROR_EN);
            lv_obj_align(labelDialog, NULL, LV_ALIGN_CENTER, 0, -20);
        } else if(upload_result == 3) {
            char buf[200];
            int _index = 0;

            ZERO(buf);

            strcpy(buf, DIALOG_UPLOAD_FINISH_EN);
            _index = strlen(buf);
            buf[_index] = '\n';
            _index++;
            strcat(buf, DIALOG_UPLOAD_SIZE_EN);

            _index = strlen(buf);
            buf[_index] = ':';
            _index++;
            sprintf(&buf[_index], " %d KBytes\n", (int)(upload_size / 1024));

            strcat(buf, DIALOG_UPLOAD_TIME_EN);
            _index = strlen(buf);
            buf[_index] = ':';
            _index++;
            sprintf(&buf[_index], " %d s\n", (int)upload_time);

            strcat(buf, DIALOG_UPLOAD_SPEED_EN);
            _index = strlen(buf);
            buf[_index] = ':';
            _index++;
            sprintf(&buf[_index], " %d KBytes/s\n", (int)(upload_size / upload_time / 1024));

            lv_label_set_text(labelDialog, buf);
            lv_obj_align(labelDialog, NULL, LV_ALIGN_CENTER, 0, -20);

        }
    }
#endif //USE_WIFI_FUNCTION
    else if(uiCfg.dialogType == DIALOG_TYPE_FILAMENT_LOAD_HEAT) {
        lv_label_set_text(labelDialog, filament_menu.filament_dialog_load_heat);
        lv_obj_align(labelDialog, NULL, LV_ALIGN_CENTER, 0, -20);
    } else if(uiCfg.dialogType == DIALOG_TYPE_FILAMENT_HEAT_LOAD_COMPLETED) {
        lv_label_set_text(labelDialog, filament_menu.filament_dialog_load_heat_confirm);
        lv_obj_align(labelDialog, NULL, LV_ALIGN_CENTER, 0, -20);
    } else if(uiCfg.dialogType == DIALOG_TYPE_FILAMENT_UNLOAD_HEAT) {
        lv_label_set_text(labelDialog, filament_menu.filament_dialog_unload_heat);
        lv_obj_align(labelDialog, NULL, LV_ALIGN_CENTER, 0, -20);
    } else if(uiCfg.dialogType == DIALOG_TYPE_FILAMENT_HEAT_UNLOAD_COMPLETED) {
        lv_label_set_text(labelDialog, filament_menu.filament_dialog_unload_heat_confirm);
        lv_obj_align(labelDialog, NULL, LV_ALIGN_CENTER, 0, -20);
    } else if(uiCfg.dialogType == DIALOG_TYPE_FILAMENT_LOAD_COMPLETED) {
        lv_label_set_text(labelDialog, filament_menu.filament_dialog_load_completed);
        lv_obj_align(labelDialog, NULL, LV_ALIGN_CENTER, 0, -20);
    } else if(uiCfg.dialogType == DIALOG_TYPE_FILAMENT_UNLOAD_COMPLETED) {
        lv_label_set_text(labelDialog, filament_menu.filament_dialog_unload_completed);
        lv_obj_align(labelDialog, NULL, LV_ALIGN_CENTER, 0, -20);
    } else if(uiCfg.dialogType == DIALOG_TYPE_FILAMENT_LOADING) {
        lv_label_set_text(labelDialog, filament_menu.filament_dialog_loading);
        lv_obj_align(labelDialog, NULL, LV_ALIGN_CENTER, 0, -70);
    } else if(uiCfg.dialogType == DIALOG_TYPE_FILAMENT_UNLOADING) {
        lv_label_set_text(labelDialog, filament_menu.filament_dialog_unloading);
        lv_obj_align(labelDialog, NULL, LV_ALIGN_CENTER, 0, -70);
    }
#if HAS_ROTARY_ENCODER
    if(gCfgItems.encoder_enable) {
        if(btnOk) lv_group_add_obj(g, btnOk);
        if(btnCancel) lv_group_add_obj(g, btnCancel);
    }
#endif
}

void dialog_temperature_display()
{
    if(uiCfg.dialog_temp_display) {		
        char buf[20] = {0};
        public_buf_l[0] = '\0';
        strcat(public_buf_l, preheat_menu.ext1);
        sprintf(buf, preheat_menu.value_state, (int)thermalManager.temp_hotend[uiCfg.curSprayerChoose].celsius, (int)thermalManager.temp_hotend[uiCfg.curSprayerChoose].target);
        strcat_P(public_buf_l, PSTR(": "));
        strcat(public_buf_l, buf);
        lv_label_set_text(print_hotend_temp, public_buf_l);
        lv_obj_align(print_hotend_temp, NULL, LV_ALIGN_CENTER, 0, -50);

        public_buf_l[0] = '\0';
        strcat(public_buf_l, preheat_menu.hotbed);
        sprintf(buf, preheat_menu.value_state, (int)thermalManager.temp_bed.celsius, (int)thermalManager.temp_bed.target);
        strcat_P(public_buf_l, PSTR(": "));
        strcat(public_buf_l, buf);
        lv_label_set_text(print_bed_temp, public_buf_l);
        lv_obj_align(print_bed_temp, NULL, LV_ALIGN_CENTER, 0, -20);
    }
}

void filament_sprayer_temp()
{
    char buf[20] = {0};

    public_buf_l[0] = '\0';

    if(uiCfg.curSprayerChoose < 1)
        strcat(public_buf_l, preheat_menu.ext1);
    else
        strcat(public_buf_l, preheat_menu.ext2);
    sprintf(buf, preheat_menu.value_state, (int)thermalManager.temp_hotend[uiCfg.curSprayerChoose].celsius, (int)thermalManager.temp_hotend[uiCfg.curSprayerChoose].target);

    strcat_P(public_buf_l, PSTR(": "));
    strcat(public_buf_l, buf);
    lv_label_set_text(labExtraTemp, public_buf_l);
    lv_obj_align(labExtraTemp, NULL, LV_ALIGN_CENTER, 0, -60);
}

void sensor_dialog_handle()
{
    static bool enter_autoleveling_flag = 0;
    static uint8_t enter_autoleveling_cnt = 0;

    if(uiCfg.dialogType != DIALOG_CHECK_SENSOR)
        return;

    if(f_init_sensor == 0) {
        bltouch._startSensorChk();
        f_init_sensor = 1;
    }

    bool cur_sensor_status = READ(Z_MIN_PROBE_PIN);

    if(pre_sensor_status != cur_sensor_status) {
        pre_sensor_status = cur_sensor_status;
        if(sensor_target_cnt < SENSOR_TARGET_CNT && sensor_check_overtime < (CHECK_OVERTIME_NUM - 1)) {
            sensor_target_cnt++;
            SERIAL_ECHOLNPAIR("sensor_target_cnt:", sensor_target_cnt);
        }
    }

    if(temperature_change_frequency) {
        temperature_change_frequency = 0;
        sensor_check_overtime++;
        //SERIAL_ECHOLNPAIR("sensor_check_overtime:", sensor_check_overtime);

        if(sensor_target_cnt >= SENSOR_TARGET_CNT) {
            lv_label_set_text(labSensorCheck, leveling_menu.sensorchkok);
            if(enter_autoleveling_flag == 0) {
                enter_autoleveling_flag = 1;
                enter_autoleveling_cnt = sensor_check_overtime;
            }

            if((sensor_check_overtime - enter_autoleveling_cnt) >= 1) {
                enter_autoleveling_cnt = 0;
                enter_autoleveling_flag = 0;
                pre_sensor_status = false;
                f_init_sensor = 0;
                sensor_check_overtime = 0;
                sensor_target_cnt = 0;
                uiCfg.leveling_type_save = auto_manu_level_sel;
                auto_manu_level_sel = 1;
                bltouch._endSensorChk();
                clear_cur_ui();
                lv_draw_auto_level();
            }
        } else {
            if(sensor_check_overtime >= CHECK_OVERTIME_NUM) {
                sensor_check_overtime--;
                lv_label_set_text(labSensorCheck, leveling_menu.sensorchkng);
            }
        }
    }
}

void filament_dialog_handle()
{
    if((temperature_change_frequency == 1)
       && ((uiCfg.dialogType == DIALOG_TYPE_FILAMENT_LOAD_HEAT)
           || (uiCfg.dialogType  == DIALOG_TYPE_FILAMENT_UNLOAD_HEAT))
      ) {
        filament_sprayer_temp();
        temperature_change_frequency = 0;
    }
    if(uiCfg.filament_heat_completed_load == 1) {
        uiCfg.filament_heat_completed_load = 0;
        lv_clear_dialog();
        lv_draw_dialog(DIALOG_TYPE_FILAMENT_LOADING);
        planner.synchronize();
        uiCfg.filament_loading_time_flg = 1;
        uiCfg.filament_loading_time_cnt = 0;
        ZERO(public_buf_m);
        sprintf_P(public_buf_m, PSTR("T%d\nG91\nG1 E%d F%d\nG90"), uiCfg.curSprayerChoose, gCfgItems.filamentchange_load_length, gCfgItems.filamentchange_load_speed);
        queue.inject_P(PSTR(public_buf_m));
        //gcode.process_subcommands_now_P(PSTR(public_buf_m));
    }
    if(uiCfg.filament_heat_completed_unload == 1) {
        uiCfg.filament_heat_completed_unload = 0;
        lv_clear_dialog();
        lv_draw_dialog(DIALOG_TYPE_FILAMENT_UNLOADING);
        planner.synchronize();
        uiCfg.filament_unloading_time_flg = 1;
        uiCfg.filament_unloading_time_cnt = 0;
        ZERO(public_buf_m);
        sprintf_P(public_buf_m, PSTR("T%d\nG91\nG1 E-%d F%d\nG90"), uiCfg.curSprayerChoose, gCfgItems.filamentchange_unload_length, gCfgItems.filamentchange_unload_speed);
        queue.inject_P(PSTR(public_buf_m));
    }

    if(((abs((int)((int)thermalManager.temp_hotend[uiCfg.curSprayerChoose].celsius - gCfgItems.filament_limit_temper)) <= 1)
        || ((int)thermalManager.temp_hotend[uiCfg.curSprayerChoose].celsius > gCfgItems.filament_limit_temper))
       && (uiCfg.filament_load_heat_flg == 1)
      ) {
        uiCfg.filament_load_heat_flg = 0;
        lv_clear_dialog();
        lv_draw_dialog(DIALOG_TYPE_FILAMENT_HEAT_LOAD_COMPLETED);
    }

    if(uiCfg.filament_loading_completed == 1) {
        uiCfg.filament_rate = 0;
        uiCfg.filament_loading_completed = 0;
        lv_clear_dialog();
        lv_draw_dialog(DIALOG_TYPE_FILAMENT_LOAD_COMPLETED);
    }
    if(((abs((int)((int)thermalManager.temp_hotend[uiCfg.curSprayerChoose].celsius - gCfgItems.filament_limit_temper)) <= 1)
        || ((int)thermalManager.temp_hotend[uiCfg.curSprayerChoose].celsius > gCfgItems.filament_limit_temper))
       && (uiCfg.filament_unload_heat_flg == 1)
      ) {
        uiCfg.filament_unload_heat_flg = 0;
        lv_clear_dialog();
        lv_draw_dialog(DIALOG_TYPE_FILAMENT_HEAT_UNLOAD_COMPLETED);
    }

    if(uiCfg.filament_unloading_completed == 1) {
        uiCfg.filament_rate = 0;
        uiCfg.filament_unloading_completed = 0;
        lv_clear_dialog();
        lv_draw_dialog(DIALOG_TYPE_FILAMENT_UNLOAD_COMPLETED);
    }

    if(uiCfg.dialogType == DIALOG_TYPE_FILAMENT_LOADING
       || uiCfg.dialogType == DIALOG_TYPE_FILAMENT_UNLOADING
      ) lv_filament_setbar();
}

void lv_filament_setbar()
{
    lv_bar_set_value(filament_bar, uiCfg.filament_rate, LV_ANIM_ON);
}

void lv_clear_dialog()
{
#if HAS_ROTARY_ENCODER
    if(gCfgItems.encoder_enable) lv_group_remove_all_objs(g);
#endif
    uiCfg.filament_load_heat_flg = 0;
    uiCfg.filament_unload_heat_flg = 0;
    uiCfg.filament_heat_completed_load = 0;
    uiCfg.filament_heat_completed_unload = 0;
    lv_obj_del(scr);
}

#endif // HAS_TFT_LVGL_UI
