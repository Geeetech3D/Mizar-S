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
#if HAS_TFT_LVGL_UI
#pragma once

#include <lvgl.h>
#include <stdint.h>
#include <string.h>

// the colors of the last MKS Ui
#undef LV_COLOR_BACKGROUND
#define LV_COLOR_BACKGROUND LV_COLOR_MAKE(0x1A, 0x1A, 0x1A) // LV_COLOR_MAKE(0x00, 0x00, 0x00)

#define TFT_LV_PARA_BACK_BODY_COLOR		  LV_COLOR_MAKE(0x4A, 0x52, 0xFF)

#include "tft_lvgl_configuration.h"
#include "tft_multi_language.h"
#include "pic_manager.h"
#include "draw_ready_print.h"
#include "draw_language.h"
#include "draw_set.h"
#include "draw_tool.h"
#include "draw_print_file.h"
#include "draw_dialog.h"
#include "draw_printing.h"
#include "draw_operation.h"
#include "draw_preHeat.h"
#include "draw_handHeat.h"
#include "draw_extrusion.h"
#include "draw_home.h"
#include "draw_more.h"
#include "draw_move_motor.h"
#include "draw_fan.h"
#include "draw_about.h"
#include "draw_change_speed.h"
#include "draw_manuaLevel.h"
#include "draw_autoLevel.h"
#include "draw_error_message.h"
#include "printer_operation.h"
#include "draw_machine_para.h"
#include "draw_machine_settings.h"
#include "draw_motor_settings.h"
#include "draw_advance_settings.h"
#include "draw_acceleration_settings.h"
#include "draw_number_key.h"
#include "draw_jerk_settings.h"
#include "draw_pause_position.h"
#include "draw_step_settings.h"
#include "draw_tmc_current_settings.h"
#include "draw_eeprom_settings.h"
#include "draw_max_feedrate_settings.h"
#include "draw_tmc_step_mode_settings.h"
#include "draw_level_settings.h"
#include "draw_manual_level_pos_settings.h"
#include "draw_autolevel_offset_settings.h"
#include "draw_filament_change.h"
#include "draw_filament_settings.h"
#include "draw_homing_sensitivity_settings.h"
#include "draw_baby_stepping.h"
#include "draw_keyboard.h"
#include "draw_encoder_settings.h"
#include "draw_autolevel_menu.h"

#if ENABLED(USE_WIFI_FUNCTION)
#include "wifiSerial.h"
#include "wifi_module.h"
#include "wifi_upload.h"
#include "draw_wifi_settings.h"
#include "draw_wifi.h"
#include "draw_wifi_list.h"
#include "draw_wifi_tips.h"
#endif

#include "../../inc/MarlinConfigPre.h"
#define FILE_SYS_USB  0
#define FILE_SYS_SD 1

#define TICK_CYCLE 1

#define PARA_SEL_ICON_TEXT_COLOR  LV_COLOR_MAKE(0x4A, 0x52, 0xFF);

#define TFT35

#ifdef TFT35

#define TFT_WIDTH         480
#define TFT_HEIGHT        320

#define titleHeight        36   // TFT_screen.title_high
#define INTERVAL_H          2   // TFT_screen.gap_h // 2
#define INTERVAL_V          5   // TFT_screen.gap_v // 2
#define BTN_X_PIXEL       117   // TFT_screen.btn_x_pixel
#define BTN_Y_PIXEL       140   // TFT_screen.btn_y_pixel

#define SIMPLE_FIRST_PAGE_GRAP   30

#define BUTTON_TEXT_Y_OFFSET    -9

#define TITLE_XPOS          3    // TFT_screen.title_xpos
#define TITLE_YPOS          5    // TFT_screen.title_ypos

#define FILE_BTN_CNT        6

#define OTHER_BTN_XPIEL   117
#define OTHER_BTN_YPIEL    92

#define FILE_PRE_PIC_X_OFFSET 8
#define FILE_PRE_PIC_Y_OFFSET 0

#define PREVIEW_LITTLE_PIC_SIZE  40910  // 400*100+9*101+1
#define PREVIEW_SIZE      202720        // (PREVIEW_LITTLE_PIC_SIZE+800*200+201*9+1)

// machine parameter ui
#define PARA_UI_POS_X             10
#define PARA_UI_POS_Y             50

#define PARA_UI_SIZE_X            450
#define PARA_UI_SIZE_Y            40

#define PARA_UI_ARROW_V          12

#define PARA_UI_BACL_POS_X        400
#define PARA_UI_BACL_POS_Y        270

#define PARA_UI_TURN_PAGE_POS_X   320
#define PARA_UI_TURN_PAGE_POS_Y   270

#define PARA_UI_VALUE_SIZE_X      370
#define PARA_UI_VALUE_POS_X       400
#define PARA_UI_VALUE_V           5

#define PARA_UI_STATE_POS_X       380
#define PARA_UI_STATE_V           2

#define PARA_UI_VALUE_SIZE_X_2    200
#define PARA_UI_VALUE_POS_X_2     320
#define PARA_UI_VALUE_V_2         5

#define PARA_UI_VALUE_BTN_X_SIZE  80
#define PARA_UI_VALUE_BTN_Y_SIZE  28

#define PARA_UI_BACK_BTN_X_SIZE      80
#define PARA_UI_BACK_BTN_Y_SIZE	   50

#else // ifdef TFT35

#define TFT_WIDTH     320
#define TFT_HEIGHT    240

#endif // ifdef TFT35

#define BTN_ROW1_COL1_POS	INTERVAL_V, titleHeight
#define BTN_ROW1_COL2_POS	BTN_X_PIXEL + INTERVAL_V * 2, titleHeight
#define BTN_ROW1_COL3_POS	BTN_X_PIXEL * 2 + INTERVAL_V * 3, titleHeight
#define BTN_ROW1_COL4_POS	BTN_X_PIXEL * 3 + INTERVAL_V * 4, titleHeight

#define BTN_ROW2_COL1_POS	 INTERVAL_V, BTN_Y_PIXEL + INTERVAL_H + titleHeight
#define BTN_ROW2_COL2_POS 	BTN_X_PIXEL + INTERVAL_V * 2, BTN_Y_PIXEL + INTERVAL_H + titleHeight
#define BTN_ROW2_COL3_POS 	BTN_X_PIXEL * 2 + INTERVAL_V * 3, BTN_Y_PIXEL + INTERVAL_H + titleHeight
#define BTN_ROW2_COL4_POS 	BTN_X_PIXEL * 3 + INTERVAL_V * 4, BTN_Y_PIXEL + INTERVAL_H + titleHeight


#ifdef __cplusplus
extern "C" { /* C-declarations for C++ */
#endif

extern char public_buf_m[100];
extern char public_buf_l[30];

typedef struct {
    uint32_t spi_flash_flag;
    uint8_t disp_rotation_180;
    uint8_t multiple_language;
    uint8_t language;
    uint8_t leveling_mode;
    uint8_t from_flash_pic;
    uint8_t finish_power_off;

    bool  cloud_enable;
    bool  encoder_enable;
    int   filamentchange_load_length;
    int   filamentchange_load_speed;
    int   filamentchange_unload_length;
    int   filamentchange_unload_speed;
    int   filament_limit_temper;
    float pausePosX;
    float pausePosY;
    float pausePosZ;
    uint32_t curFilesize;

		float bk_abl_start;
		float bk_abl_grid;
		float bk_abl_z_values[GRID_MAX_POINTS_X][GRID_MAX_POINTS_Y];
		float bk_mbl_z_value[GRID_MAX_POINTS_X][GRID_MAX_POINTS_Y];
} CFG_ITMES;

typedef struct {
    uint8_t curTempType : 1,
            curSprayerChoose : 3,
            stepHeat : 4;
    uint8_t para_ui_page: 1,
            configWifi: 1,
            command_send: 1,
            filament_load_heat_flg: 1,
            filament_heat_completed_load: 1,
            filament_unload_heat_flg: 1,
            filament_heat_completed_unload: 1;
    uint8_t filament_loading_completed: 1,
            filament_unloading_completed: 1,
            filament_loading_time_flg: 1,
            filament_unloading_time_flg: 1,
            curSprayerChoose_bak: 4;
    uint8_t speedType;
    uint8_t wifi_name[32];
    uint8_t wifi_key[64];
    uint8_t cloud_hostUrl[96];
    uint8_t extruStep;
    uint8_t extruSpeed;
    uint8_t print_state;
    uint8_t stepPrintSpeed;
    uint8_t waitEndMoves;
    uint8_t dialogType;
    uint8_t F[4];
    uint8_t filament_rate;
    uint8_t ledSwitch;
    uint8_t auto_leveling_point_status;
    uint8_t auto_leveling_point_num: 5,
            auto_leveling_heat_flg: 1,
            auto_leveling_force_stop: 1,
            auto_leveling_done_flag: 1;
    uint8_t manu_leveling_heat_flg: 1,
            manu_leveling_begin_flg: 1,
            manu_leveling_done_flag: 1;
    uint8_t storage_flag: 1,
            leveling_type_save: 1,
            test_finish_flag: 1,
            print_open_led: 1,
            dialog_temp_display:1;
    uint8_t mintemp_except_flag: 1,
            maxtemp_except_flag: 1,
            e_relative:1;
    uint8_t temp_except_id;
		uint8_t stop_reprint_step;

    uint16_t moveSpeed;
    uint16_t cloud_port;
    uint16_t serial_stable_cnt;

    uint32_t totalSend;
    uint32_t filament_loading_time;
    uint32_t filament_unloading_time;
    uint32_t filament_loading_time_cnt;
    uint32_t filament_unloading_time_cnt;
    float move_dist;
    float desireSprayerTempBak;
    float desireBedTempBak;
    float current_x_position_bak;
    float current_y_position_bak;
    float current_e_position_bak;
    float moveSpeed_bak;

    float test_mean;  // The average of all points so far, used to calculate deviation
    float test_sigma;  // Standard deviation of all points so far
    float test_min;  	// Smallest value sampled so far
    float test_max; 	// Largest value sampled so far
    float test_range;	// max - min
    float test_z;			// z height value
    uint8_t test_counter;	// test counter
    uint8_t test_state;		// test state
} UI_CFG;

typedef enum {
    MAIN_UI,
    PRINT_READY_UI,
    PRINT_FILE_UI,
    PRINTING_UI,
    MOVE_MOTOR_UI,
    OPERATE_UI,
    PAUSE_UI,
    EXTRUSION_UI,
    FAN_UI,
    PRE_HEAT_UI,
    HAND_HEAT_UI,
    CHANGE_SPEED_UI,
    TEMP_UI,
    SET_UI,
    ZERO_UI,
    SPRAYER_UI,
    MACHINE_UI,
    LANGUAGE_UI,
    ABOUT_UI,
    LOG_UI,
    DISK_UI,
    CALIBRATE_UI,
    DIALOG_UI,
    WIFI_UI,
    MORE_UI,
    FILETRANSFER_UI,
    FILETRANSFERSTATE_UI,
    PRINT_MORE_UI,
    FILAMENTCHANGE_UI,
    LEVELING_UI,
    AUTOLEVELING_MENU_UI,
    AUTOLEVELING_UI,
    MESHLEVELING_UI,
    BIND_UI,
    NOZZLE_PROBE_OFFSET_UI,
    TOOL_UI,
    HARDWARE_TEST_UI,
    WIFI_LIST_UI,
    KEY_BOARD_UI,
    WIFI_TIPS_UI,
    MACHINE_PARA_UI,
    MACHINE_SETTINGS_UI,
    TEMPERATURE_SETTINGS_UI,
    MOTOR_SETTINGS_UI,
    MACHINETYPE_UI,
    STROKE_UI,
    HOME_DIR_UI,
    ENDSTOP_TYPE_UI,
    FILAMENT_SETTINGS_UI,
    LEVELING_SETTIGNS_UI,
    LEVELING_PARA_UI,
    DELTA_LEVELING_PARA_UI,
    MANUAL_LEVELING_POSIGION_UI,
    MAXFEEDRATE_UI,
    STEPS_UI,
    ACCELERATION_UI,
    JERK_UI,
    MOTORDIR_UI,
    HOMESPEED_UI,
    NOZZLE_CONFIG_UI,
    HOTBED_CONFIG_UI,
    ADVANCED_UI,
    DOUBLE_Z_UI,
    ENABLE_INVERT_UI,
    NUMBER_KEY_UI,
    BABY_STEP_UI,
    ERROR_MESSAGE_UI,
    PAUSE_POS_UI,
    TMC_CURRENT_UI,
    TMC_MODE_UI,
    EEPROM_SETTINGS_UI,
    WIFI_SETTINGS_UI,
    HOMING_SENSITIVITY_UI,
    ENCODER_SETTINGS_UI
} DISP_STATE;

typedef struct {
    DISP_STATE _disp_state[100];
    int _disp_index;
} DISP_STATE_STACK;

typedef struct {
    int16_t days;
    uint16_t hours;
    uint8_t minutes;
    volatile int8_t seconds;
    int8_t ms_10;
    int8_t start;
} PRINT_TIME;
extern PRINT_TIME print_time;

typedef enum {
    PrintAcceleration,
    RetractAcceleration,
    TravelAcceleration,
    XAcceleration,
    YAcceleration,
    ZAcceleration,
    E0Acceleration,
    E1Acceleration,

    XMaxFeedRate,
    YMaxFeedRate,
    ZMaxFeedRate,
    E0MaxFeedRate,
    E1MaxFeedRate,

    XJerk,
    YJerk,
    ZJerk,
    EJerk,

    Xstep,
    Ystep,
    Zstep,
    E0step,
    E1step,

    Xcurrent,
    Ycurrent,
    Zcurrent,
    E0current,
    E1current,

    pause_pos_x,
    pause_pos_y,
    pause_pos_z,

    level_pos_x1,
    level_pos_y1,
    level_pos_x2,
    level_pos_y2,
    level_pos_x3,
    level_pos_y3,
    level_pos_x4,
    level_pos_y4,
    level_pos_x5,
    level_pos_y5
#if HAS_BED_PROBE
    ,
    x_offset,
    y_offset,
    z_offset
#endif
    ,
    load_length,
    load_speed,
    unload_length,
    unload_speed,
    filament_temp,

    x_sensitivity,
    y_sensitivity,
    z_sensitivity,
    z2_sensitivity
} num_key_value_state;
extern num_key_value_state value;

typedef enum {
    wifiName,
    wifiPassWord,
    wifiConfig,
    gcodeCommand
} keyboard_value_state;
extern keyboard_value_state keyboard_value;

typedef enum {
    leveling_start,
    leveling_heat,
    leveling_home,
    leveling_doing,
    leveling_done,
    leveling_saved,
    leveling_recover,
    leveling_stop
} auto_leveling_state;

typedef enum {
    leveling_point_begin,
    leveling_point_1,
    leveling_point_2,
    leveling_point_3,
    leveling_point_4,
    leveling_point_5,
    leveling_point_6,
    leveling_point_7,
    leveling_point_8,
    leveling_point_9,
    leveling_point_10,
    leveling_point_11,
    leveling_point_12,
    leveling_point_13,
    leveling_point_14,
    leveling_point_15,
    leveling_point_16,
    leveling_point_end
} auto_leveling_point;


extern CFG_ITMES gCfgItems;
extern UI_CFG uiCfg;
extern DISP_STATE disp_state;
extern DISP_STATE last_disp_state;
extern DISP_STATE_STACK disp_state_stack;
extern bool lvgl_wait_for_move;

extern lv_style_t tft_style_scr;
extern lv_style_t tft_style_label_pre;
extern lv_style_t tft_style_label_rel;
extern lv_style_t style_line;
extern lv_style_t style_para_value_pre;
extern lv_style_t style_para_value_rel;
extern lv_style_t style_num_key_pre;
extern lv_style_t style_num_key_rel;
extern lv_style_t style_num_text;
extern lv_style_t style_sel_text;
extern lv_style_t style_para_value;
extern lv_style_t style_para_back;
extern lv_style_t lv_bar_style_indic;
extern lv_style_t style_yellow_label;


extern void gCfgItems_init();
extern void ui_cfg_init();
extern void tft_style_init();
extern char *creat_title_text(void);
extern void preview_gcode_prehandle(char *path);
extern void update_spi_flash();
extern void update_gcode_command(int addr, uint8_t *s);
extern void get_gcode_command(int addr, uint8_t *d);
#if HAS_GCODE_PREVIEW
extern void disp_pre_gcode(int xpos_pixel, int ypos_pixel);
#endif
extern void GUI_RefreshPage();
extern void clear_cur_ui();
extern void draw_return_ui();
extern void sd_detection();
extern void gCfg_to_spiFlah();
extern void print_time_count();

extern void LV_TASK_HANDLER();
extern void lv_ex_line(lv_obj_t * line, lv_point_t *points);
extern void lv_draw_sprayer_temp(lv_obj_t *labInfo);
extern void lv_draw_bed_temp(lv_obj_t *labInfo);

#ifdef __cplusplus
} /* C-declarations for C++ */
#endif
#endif

