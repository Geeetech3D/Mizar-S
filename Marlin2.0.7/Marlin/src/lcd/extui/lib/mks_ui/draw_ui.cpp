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

#include "tft_lvgl_configuration.h"

#include "pic_manager.h"

#include "draw_ui.h"
#include "mks_hardware_test.h"

#include <SPI.h>

#include "../../../../MarlinCore.h"
#include "../../../../sd/cardreader.h"
#include "../../../../module/motion.h"
#include "../../../../module/planner.h"
#include "../../../../module/temperature.h"
#include "../../../../lcd/ultralcd.h"

#if ENABLED(POWER_LOSS_RECOVERY)
#include "../../../../feature/powerloss.h"
#endif

#if ENABLED(PARK_HEAD_ON_PAUSE)
#include "../../../../feature/pause.h"
#endif

CFG_ITMES gCfgItems;
UI_CFG uiCfg;
DISP_STATE_STACK disp_state_stack;
DISP_STATE disp_state = MAIN_UI;
DISP_STATE last_disp_state;
PRINT_TIME print_time;
num_key_value_state value;
keyboard_value_state keyboard_value;

uint32_t To_pre_view;
uint8_t gcode_preview_over;
uint8_t flash_preview_begin;
uint8_t default_preview_flg;
uint32_t size = 809;
uint16_t row;
uint8_t temperature_change_frequency;
uint8_t printing_rate_update_flag;
uint8_t return_printui_chk;
uint8_t return_printui_flag;
bool lvgl_wait_for_move;

extern uint8_t once_flag;
extern uint8_t sel_id;
extern uint8_t public_buf[512];
extern uint8_t bmp_public_buf[];

extern void LCD_IO_WriteData(uint16_t RegValue);

static const char custom_gcode_command[][100] = {
    "G28\nG29\nM500",
    "G28",
    "G28",
    "G28",
    "G28"
};

lv_point_t line_points[4][2] = {
    {{PARA_UI_POS_X, PARA_UI_POS_Y + PARA_UI_SIZE_Y}, {TFT_WIDTH, PARA_UI_POS_Y + PARA_UI_SIZE_Y}},
    {{PARA_UI_POS_X, PARA_UI_POS_Y * 2 + PARA_UI_SIZE_Y}, {TFT_WIDTH, PARA_UI_POS_Y * 2 + PARA_UI_SIZE_Y}},
    {{PARA_UI_POS_X, PARA_UI_POS_Y * 3 + PARA_UI_SIZE_Y}, {TFT_WIDTH, PARA_UI_POS_Y * 3 + PARA_UI_SIZE_Y}},
    {{PARA_UI_POS_X, PARA_UI_POS_Y * 4 + PARA_UI_SIZE_Y}, {TFT_WIDTH, PARA_UI_POS_Y * 4 + PARA_UI_SIZE_Y}}
};
void gCfgItems_init()
{
    gCfgItems.multiple_language = MULTI_LANGUAGE_ENABLE;
#if 1 // LCD_LANGUAGE == en
    gCfgItems.language = LANG_ENGLISH;
#elif LCD_LANGUAGE == zh_CN
    gCfgItems.language = LANG_SIMPLE_CHINESE;
#elif LCD_LANGUAGE == zh_TW
    gCfgItems.language = LANG_COMPLEX_CHINESE;
#elif LCD_LANGUAGE == jp_kana
    gCfgItems.language = LANG_JAPAN;
#elif LCD_LANGUAGE == de
    gCfgItems.language = LANG_GERMAN;
#elif LCD_LANGUAGE == fr
    gCfgItems.language = LANG_FRENCH;
#elif LCD_LANGUAGE == ru
    gCfgItems.language = LANG_RUSSIAN;
#elif LCD_LANGUAGE == ko_KR
    gCfgItems.language = LANG_KOREAN;
#elif LCD_LANGUAGE == tr
    gCfgItems.language = LANG_TURKISH;
#elif LCD_LANGUAGE == es
    gCfgItems.language = LANG_SPANISH;
#elif LCD_LANGUAGE == el
    gCfgItems.language = LANG_GREEK;
#elif LCD_LANGUAGE == it
    gCfgItems.language = LANG_ITALY;
#elif LCD_LANGUAGE == pt
    gCfgItems.language = LANG_PORTUGUESE;
#endif
    gCfgItems.leveling_mode     = 0;
    gCfgItems.from_flash_pic    = 0;
    gCfgItems.curFilesize       = 0;
    gCfgItems.finish_power_off  = 0;
    gCfgItems.pausePosX         = 0;
    gCfgItems.pausePosY         = 0;
    gCfgItems.pausePosZ         = 0;
    gCfgItems.cloud_enable  = true;

    gCfgItems.filamentchange_load_length   = 600;
    gCfgItems.filamentchange_load_speed    = 200;
    gCfgItems.filamentchange_unload_length  = 700;
    gCfgItems.filamentchange_unload_speed  = 200;
    gCfgItems.filament_limit_temper        = 200;

    gCfgItems.encoder_enable = true;

		ZERO(gCfgItems.bk_mbl_z_value);

		gCfgItems.bk_abl_start = 0;
		gCfgItems.bk_abl_grid = 0;
		GRID_LOOP(x, y) {
			gCfgItems.bk_abl_z_values[x][y] = NAN;
		}

    W25QXX.SPI_FLASH_BufferRead((uint8_t *)&gCfgItems.spi_flash_flag, VAR_INF_ADDR, sizeof(gCfgItems.spi_flash_flag));
    if(gCfgItems.spi_flash_flag == FLASH_INF_VALID_FLAG) {
        W25QXX.SPI_FLASH_BufferRead((uint8_t *)&gCfgItems, VAR_INF_ADDR, sizeof(gCfgItems));
    } else {
        gCfgItems.spi_flash_flag = FLASH_INF_VALID_FLAG;
        W25QXX.SPI_FLASH_SectorErase(VAR_INF_ADDR);
        W25QXX.SPI_FLASH_BufferWrite((uint8_t *)&gCfgItems, VAR_INF_ADDR, sizeof(gCfgItems));
        //init gcode command
        W25QXX.SPI_FLASH_BufferWrite((uint8_t *)&custom_gcode_command[0], AUTO_LEVELING_COMMAND_ADDR, 100);
        W25QXX.SPI_FLASH_BufferWrite((uint8_t *)&custom_gcode_command[1], OTHERS_COMMAND_ADDR_1, 100);
        W25QXX.SPI_FLASH_BufferWrite((uint8_t *)&custom_gcode_command[2], OTHERS_COMMAND_ADDR_2, 100);
        W25QXX.SPI_FLASH_BufferWrite((uint8_t *)&custom_gcode_command[3], OTHERS_COMMAND_ADDR_3, 100);
        W25QXX.SPI_FLASH_BufferWrite((uint8_t *)&custom_gcode_command[4], OTHERS_COMMAND_ADDR_4, 100);
    }

    const byte rot = (TFT_ROTATION & TFT_ROTATE_180) ? 0xEE : 0x00;
    if(gCfgItems.disp_rotation_180 != rot) {
        gCfgItems.disp_rotation_180 = rot;
        update_spi_flash();
    }

    uiCfg.F[0] = 'N';
    uiCfg.F[1] = 'A';
    uiCfg.F[2] = 'N';
    uiCfg.F[3] = 'O';
    W25QXX.SPI_FLASH_BlockErase(REFLSHE_FLGA_ADD + 32 - 64 * 1024);
    W25QXX.SPI_FLASH_BufferWrite(uiCfg.F, REFLSHE_FLGA_ADD, 4);
}

void ui_cfg_init()
{
    uiCfg.curTempType         = 0;
    uiCfg.curSprayerChoose    = 0;
    uiCfg.stepHeat            = 10;
    uiCfg.para_ui_page        = 0;
    uiCfg.extruStep           = 5;
    uiCfg.extruSpeed          = 1;
    uiCfg.move_dist           = 1;
    uiCfg.moveSpeed           = 3000;
    uiCfg.stepPrintSpeed      = 10;
    uiCfg.command_send        = 0;
    uiCfg.dialogType          = 0;
    uiCfg.speedType						= 0;
    uiCfg.filament_heat_completed_load = 0;
    uiCfg.filament_rate                = 0;
    uiCfg.filament_loading_completed   = 0;
    uiCfg.filament_unloading_completed = 0;
    uiCfg.filament_loading_time_flg    = 0;
    uiCfg.filament_loading_time_cnt    = 0;
    uiCfg.filament_unloading_time_flg  = 0;
    uiCfg.filament_unloading_time_cnt  = 0;
    uiCfg.print_open_led = 0;
		uiCfg.dialog_temp_display = 0;
    uiCfg.storage_flag = 0;
    uiCfg.mintemp_except_flag = 0;
    uiCfg.maxtemp_except_flag = 0;
    uiCfg.temp_except_id = 0;
    uiCfg.moveSpeed_bak = 30.0;
    uiCfg.serial_stable_cnt = 0;
		uiCfg.stop_reprint_step	= 0;
		uiCfg.e_relative = 0;

#if ENABLED(USE_WIFI_FUNCTION)
    memset(&wifiPara, 0, sizeof(wifiPara));
    memset(&ipPara, 0, sizeof(ipPara));
    strcpy(wifiPara.ap_name, WIFI_AP_NAME);
    strcpy(wifiPara.keyCode, WIFI_KEY_CODE);
    //client
    strcpy(ipPara.ip_addr, IP_ADDR);
    strcpy(ipPara.mask, IP_MASK);
    strcpy(ipPara.gate, IP_GATE);
    strcpy(ipPara.dns, IP_DNS);

    ipPara.dhcp_flag = IP_DHCP_FLAG;

    //AP
    strcpy(ipPara.dhcpd_ip, AP_IP_ADDR);
    strcpy(ipPara.dhcpd_mask, AP_IP_MASK);
    strcpy(ipPara.dhcpd_gate, AP_IP_GATE);
    strcpy(ipPara.dhcpd_dns, AP_IP_DNS);
    strcpy(ipPara.start_ip_addr, IP_START_IP);
    strcpy(ipPara.end_ip_addr, IP_END_IP);

    ipPara.dhcpd_flag = AP_IP_DHCP_FLAG;

    strcpy((char*)uiCfg.cloud_hostUrl, "baizhongyun.cn");
    uiCfg.cloud_port = 10086;
#endif

    uiCfg.filament_loading_time = (uint32_t)((gCfgItems.filamentchange_load_length * 60.0 / gCfgItems.filamentchange_load_speed) + 0.5);
    uiCfg.filament_unloading_time = (uint32_t)((gCfgItems.filamentchange_unload_length * 60.0 / gCfgItems.filamentchange_unload_speed) + 0.5);
}

void update_spi_flash()
{
    uint8_t command_buf[512];

    W25QXX.init(SPI_QUARTER_SPEED);
    //read back the gcode command befor erase spi flash
    W25QXX.SPI_FLASH_BufferRead((uint8_t *)&command_buf, GCODE_COMMAND_ADDR, sizeof(command_buf));
    W25QXX.SPI_FLASH_SectorErase(VAR_INF_ADDR);
    W25QXX.SPI_FLASH_BufferWrite((uint8_t *)&gCfgItems, VAR_INF_ADDR, sizeof(gCfgItems));
    W25QXX.SPI_FLASH_BufferWrite((uint8_t *)&command_buf, GCODE_COMMAND_ADDR, sizeof(command_buf));
}

void update_gcode_command(int addr, uint8_t *s)
{
    uint8_t command_buf[512];

    W25QXX.init(SPI_QUARTER_SPEED);
    //read back the gcode command befor erase spi flash
    W25QXX.SPI_FLASH_BufferRead((uint8_t *)&command_buf, GCODE_COMMAND_ADDR, sizeof(command_buf));
    W25QXX.SPI_FLASH_SectorErase(VAR_INF_ADDR);
    W25QXX.SPI_FLASH_BufferWrite((uint8_t *)&gCfgItems, VAR_INF_ADDR, sizeof(gCfgItems));
    switch(addr) {
    case AUTO_LEVELING_COMMAND_ADDR:
        memcpy(&command_buf[0 * 100], s, 100);
        break;
    case OTHERS_COMMAND_ADDR_1:
        memcpy(&command_buf[1 * 100], s, 100);
        break;
    case OTHERS_COMMAND_ADDR_2:
        memcpy(&command_buf[2 * 100], s, 100);
        break;
    case OTHERS_COMMAND_ADDR_3:
        memcpy(&command_buf[3 * 100], s, 100);
        break;
    case OTHERS_COMMAND_ADDR_4:
        memcpy(&command_buf[4 * 100], s, 100);
        break;
    default:
        break;
    }
    W25QXX.SPI_FLASH_BufferWrite((uint8_t *)&command_buf, GCODE_COMMAND_ADDR, sizeof(command_buf));
}

void get_gcode_command(int addr, uint8_t *d)
{
    W25QXX.init(SPI_QUARTER_SPEED);
    W25QXX.SPI_FLASH_BufferRead((uint8_t *)d, addr, 100);
}

lv_style_t tft_style_scr;
lv_style_t tft_style_label_pre;
lv_style_t tft_style_label_rel;

lv_style_t style_yellow_label;

lv_style_t style_line;
lv_style_t style_para_value_pre;
lv_style_t style_para_value_rel;

lv_style_t style_num_key_pre;
lv_style_t style_num_key_rel;

lv_style_t style_num_text;
lv_style_t style_sel_text;

lv_style_t style_para_value;
lv_style_t style_para_back;

lv_style_t lv_bar_style_indic;

void tft_style_init()
{
    lv_style_copy(&tft_style_scr, &lv_style_scr);
    tft_style_scr.body.main_color   = LV_COLOR_BACKGROUND;
    tft_style_scr.body.grad_color   = LV_COLOR_BACKGROUND;
    tft_style_scr.text.color        = LV_COLOR_TEXT;
    tft_style_scr.text.sel_color    = LV_COLOR_TEXT;
    tft_style_scr.line.width        = 0;
    tft_style_scr.text.letter_space = 0;
    tft_style_scr.text.line_space   = 0;

    lv_style_copy(&tft_style_label_pre, &lv_style_scr);
    lv_style_copy(&tft_style_label_rel, &lv_style_scr);
    tft_style_label_pre.body.main_color = LV_COLOR_BACKGROUND;
    tft_style_label_pre.body.grad_color = LV_COLOR_BACKGROUND;
    tft_style_label_pre.text.color      = LV_COLOR_TEXT;
    tft_style_label_pre.text.sel_color  = LV_COLOR_TEXT;
    tft_style_label_rel.body.main_color = LV_COLOR_BACKGROUND;
    tft_style_label_rel.body.grad_color = LV_COLOR_BACKGROUND;
    tft_style_label_rel.text.color      = LV_COLOR_TEXT;
    tft_style_label_rel.text.sel_color  = LV_COLOR_TEXT;
    tft_style_label_pre.text.font       = TERN(HAS_SPI_FLASH_FONT, &gb2312_puhui32, LV_FONT_DEFAULT);
    tft_style_label_rel.text.font       = TERN(HAS_SPI_FLASH_FONT, &gb2312_puhui32, LV_FONT_DEFAULT);
    tft_style_label_pre.line.width        = 0;
    tft_style_label_rel.line.width        = 0;
    tft_style_label_pre.text.letter_space = 0;
    tft_style_label_rel.text.letter_space = 0;
    tft_style_label_pre.text.line_space   = 0;
    tft_style_label_rel.text.line_space   = 0;

    lv_style_copy(&style_yellow_label, &tft_style_label_rel);
    style_yellow_label.text.color      = LV_COLOR_YELLOW;
    style_yellow_label.text.sel_color  = LV_COLOR_YELLOW;

    lv_style_copy(&style_para_value_pre, &lv_style_scr);
    lv_style_copy(&style_para_value_rel, &lv_style_scr);
    style_para_value_pre.body.main_color = LV_COLOR_BACKGROUND;
    style_para_value_pre.body.grad_color = LV_COLOR_BACKGROUND;
    style_para_value_pre.text.color      = LV_COLOR_TEXT;
    style_para_value_pre.text.sel_color  = LV_COLOR_TEXT;
    style_para_value_rel.body.main_color = LV_COLOR_BACKGROUND;
    style_para_value_rel.body.grad_color = LV_COLOR_BACKGROUND;
    style_para_value_rel.text.color      = LV_COLOR_BLACK;
    style_para_value_rel.text.sel_color  = LV_COLOR_BLACK;
    style_para_value_pre.text.font       = TERN(HAS_SPI_FLASH_FONT, &gb2312_puhui32, LV_FONT_DEFAULT);
    style_para_value_rel.text.font       = TERN(HAS_SPI_FLASH_FONT, &gb2312_puhui32, LV_FONT_DEFAULT);
    style_para_value_pre.line.width        = 0;
    style_para_value_rel.line.width        = 0;
    style_para_value_pre.text.letter_space = 0;
    style_para_value_rel.text.letter_space = 0;
    style_para_value_pre.text.line_space   = -5;
    style_para_value_rel.text.line_space   = -5;

    lv_style_copy(&style_num_key_pre, &lv_style_scr);
    lv_style_copy(&style_num_key_rel, &lv_style_scr);
    style_num_key_pre.body.main_color = LV_COLOR_KEY_BACKGROUND;
    style_num_key_pre.body.grad_color = LV_COLOR_KEY_BACKGROUND;
    style_num_key_pre.text.color      = LV_COLOR_TEXT;
    style_num_key_pre.text.sel_color  = LV_COLOR_TEXT;
    style_num_key_rel.body.main_color = LV_COLOR_KEY_BACKGROUND;
    style_num_key_rel.body.grad_color = LV_COLOR_KEY_BACKGROUND;
    style_num_key_rel.text.color      = LV_COLOR_TEXT;
    style_num_key_rel.text.sel_color  = LV_COLOR_TEXT;
#if HAS_SPI_FLASH_FONT
    style_num_key_pre.text.font = &gb2312_puhui32;
    style_num_key_rel.text.font = &gb2312_puhui32;
#else
    style_num_key_pre.text.font = LV_FONT_DEFAULT;
    style_num_key_rel.text.font = LV_FONT_DEFAULT;
#endif

    style_num_key_pre.line.width        = 0;
    style_num_key_rel.line.width        = 0;
    style_num_key_pre.text.letter_space = 0;
    style_num_key_rel.text.letter_space = 0;
    style_num_key_pre.text.line_space   = -5;
    style_num_key_rel.text.line_space   = -5;
    lv_style_copy(&style_num_text, &lv_style_scr);

    style_num_text.body.main_color   = LV_COLOR_WHITE;
    style_num_text.body.grad_color   = LV_COLOR_WHITE;
    style_num_text.text.color        = LV_COLOR_BLACK;
    style_num_text.text.sel_color    = LV_COLOR_BLACK;
    style_num_text.text.font         = TERN(HAS_SPI_FLASH_FONT, &gb2312_puhui32, LV_FONT_DEFAULT);
    style_num_text.line.width        = 0;
    style_num_text.text.letter_space = 0;
    style_num_text.text.line_space   = -5;

    lv_style_copy(&style_sel_text, &lv_style_scr);
    style_sel_text.body.main_color  = LV_COLOR_BACKGROUND;
    style_sel_text.body.grad_color  = LV_COLOR_BACKGROUND;
    style_sel_text.text.color       = LV_COLOR_YELLOW;
    style_sel_text.text.sel_color   = LV_COLOR_YELLOW;
    style_sel_text.text.font        = &gb2312_puhui32;
    style_sel_text.line.width       = 0;
    style_sel_text.text.letter_space  = 0;
    style_sel_text.text.line_space    = -5;
    lv_style_copy(&style_line, &lv_style_plain);
    style_line.line.color   = LV_COLOR_MAKE(0x49, 0x54, 0xFF);
    style_line.line.width   = 1;
    style_line.line.rounded = 1;

    lv_style_copy(&style_para_value, &lv_style_plain);
    style_para_value.body.border.color = LV_COLOR_BACKGROUND;
    style_para_value.body.border.width = 1;
    style_para_value.body.main_color   = LV_COLOR_WHITE;
    style_para_value.body.grad_color   = LV_COLOR_WHITE;
    style_para_value.body.shadow.width = 0;
    style_para_value.body.radius       = 3;
    style_para_value.text.color        = LV_COLOR_BLACK;
    style_para_value.text.font         = &TERN(HAS_SPI_FLASH_FONT, gb2312_puhui32, lv_font_roboto_22);

    lv_style_copy(&style_para_back, &lv_style_plain);
    style_para_back.body.border.color = LV_COLOR_BACKGROUND;
    style_para_back.body.border.width = 1;
    style_para_back.body.main_color   = TFT_LV_PARA_BACK_BODY_COLOR;
    style_para_back.body.grad_color   = TFT_LV_PARA_BACK_BODY_COLOR;
    style_para_back.body.shadow.width = 0;
    style_para_back.body.radius       = 3;
    style_para_back.text.color        = LV_COLOR_WHITE;
    style_para_back.text.font         = &TERN(HAS_SPI_FLASH_FONT, gb2312_puhui32, lv_font_roboto_22);

    lv_style_copy(&lv_bar_style_indic, &lv_style_pretty_color);
    lv_bar_style_indic.text.color        = lv_color_hex3(0xADF);
    lv_bar_style_indic.image.color       = lv_color_hex3(0xADF);
    lv_bar_style_indic.line.color        = lv_color_hex3(0xADF);
    lv_bar_style_indic.body.main_color   = lv_color_hex3(0xADF);
    lv_bar_style_indic.body.grad_color   = lv_color_hex3(0xADF);
    lv_bar_style_indic.body.border.color = lv_color_hex3(0xADF);
    lv_bar_style_indic.text.font         = LV_FONT_DEFAULT;

}

#define MAX_TITLE_LEN 64

char public_buf_m[100] = {0};
char public_buf_l[30];

void titleText_cat(char *str, int strSize, char *addPart)
{
    if(str == 0 || addPart == 0) return;
    if((int)(strlen(str) + strlen(addPart)) >= strSize) return;
    strcat(str, addPart);
}

char *getDispText(int index)
{

    ZERO(public_buf_l);

    switch(disp_state_stack._disp_state[index]) {
    case PRINT_READY_UI:
        strcpy(public_buf_l, main_menu.title);
        break;
    case PRINT_FILE_UI:
        strcpy(public_buf_l, file_menu.title);
        break;
    case PRINTING_UI:
        if(disp_state_stack._disp_state[disp_state_stack._disp_index] == PRINTING_UI
#ifndef TFT35
           || disp_state_stack._disp_state[disp_state_stack._disp_index] == OPERATE_UI
           || disp_state_stack._disp_state[disp_state_stack._disp_index] == PAUSE_UI
#endif
          )    strcpy(public_buf_l, common_menu.print_special_title);
        else strcpy(public_buf_l, printing_menu.title);
        break;
    case MOVE_MOTOR_UI:
        strcpy(public_buf_l, move_menu.title);
        break;
    case OPERATE_UI:
        if(disp_state_stack._disp_state[disp_state_stack._disp_index] == PRINTING_UI
#ifndef TFT35
           || disp_state_stack._disp_state[disp_state_stack._disp_index] == OPERATE_UI
           || disp_state_stack._disp_state[disp_state_stack._disp_index] == PAUSE_UI
#endif
          )    strcpy(public_buf_l, common_menu.operate_special_title);
        else strcpy(public_buf_l, operation_menu.title);
        break;

    case PAUSE_UI:
        if(disp_state_stack._disp_state[disp_state_stack._disp_index] == PRINTING_UI
           || disp_state_stack._disp_state[disp_state_stack._disp_index] == OPERATE_UI
           || disp_state_stack._disp_state[disp_state_stack._disp_index] == PAUSE_UI
          )    strcpy(public_buf_l, common_menu.pause_special_title);
        else strcpy(public_buf_l, pause_menu.title);
        break;

    case EXTRUSION_UI:
        strcpy(public_buf_l, extrude_menu.title);
        break;
    case CHANGE_SPEED_UI:
        strcpy(public_buf_l, speed_menu.title);
        break;
    case FAN_UI:
        strcpy(public_buf_l, fan_menu.title);
        break;
    case PRE_HEAT_UI:
        strcpy(public_buf_l, preheat_menu.title);
        break;
    case HAND_HEAT_UI:
        if((disp_state_stack._disp_state[disp_state_stack._disp_index - 1] == OPERATE_UI))
            strcpy(public_buf_l, preheat_menu.adjust_title);
        else strcpy(public_buf_l, preheat_menu.handheat_title);
        break;
    case SET_UI:
        strcpy(public_buf_l, set_menu.title);
        break;
    case ZERO_UI:
        strcpy(public_buf_l, home_menu.title);
        break;
    case SPRAYER_UI:
        break;
    case MACHINE_UI:
        break;
    case LANGUAGE_UI:
        strcpy(public_buf_l, language_menu.title);
        break;
    case ABOUT_UI:
        strcpy(public_buf_l, about_menu.title);
        break;
    case LOG_UI:
        break;
    case DISK_UI:
        strcpy(public_buf_l, filesys_menu.title);
        break;
    case DIALOG_UI:
        strcpy(public_buf_l, common_menu.dialog_confirm_title);
        break;
    case WIFI_UI:
        strcpy(public_buf_l, wifi_menu.title);
        break;
    case MORE_UI:
        strcpy(public_buf_l, more_menu.title);
        break;
    case PRINT_MORE_UI:
        strcpy(public_buf_l, more_menu.title);
        break;
    case FILAMENTCHANGE_UI:
        strcpy(public_buf_l, filament_menu.title);
        break;
    case LEVELING_UI:
        strcpy(public_buf_l, tool_menu.manuleveling);
        break;
    case MESHLEVELING_UI:
        strcpy(public_buf_l, leveling_menu.title);
        break;
    case AUTOLEVELING_UI:
        strcpy(public_buf_l, leveling_menu.title);
        break;
    case AUTOLEVELING_MENU_UI:
        strcpy(public_buf_l, tool_menu.autoleveling);
        break;
    case NOZZLE_PROBE_OFFSET_UI:
        strcpy(public_buf_l, machine_menu.OffsetConfTitle);
        break;
    case BIND_UI:
        strcpy(public_buf_l, cloud_menu.title);
        break;
    case TOOL_UI:
        strcpy(public_buf_l, tool_menu.title);
        break;
    case WIFI_LIST_UI:
#if ENABLED(USE_WIFI_FUNCTION)
        strcpy(public_buf_l, list_menu.title);
        break;
#endif
    case MACHINE_PARA_UI:
        strcpy(public_buf_l, MachinePara_menu.title);
        break;
    case BABY_STEP_UI:
        strcpy(public_buf_l, operation_menu.babystep);
        break;
    case EEPROM_SETTINGS_UI:
        strcpy(public_buf_l, eeprom_menu.title);
        break;
    default:
        break;
    }

    return public_buf_l;
}

char *creat_title_text()
{
    int index     = 0;
    char *tmpText = 0;
    char tmpCurFileStr[20];

    ZERO(tmpCurFileStr);

#if _LFN_UNICODE
    //cutFileName((TCHAR *)curFileName, 16, 16, (TCHAR *)tmpCurFileStr);
#else
    cutFileName(list_file.long_name[sel_id], 16, 16, tmpCurFileStr);
#endif

    ZERO(public_buf_m);

    while(index <= disp_state_stack._disp_index) {
        tmpText = getDispText(index);
        if((*tmpText == 0) || (tmpText == 0)) {
            index++;
            continue;
        }

        titleText_cat(public_buf_m, sizeof(public_buf_m), tmpText);
        if(index < disp_state_stack._disp_index) titleText_cat(public_buf_m, sizeof(public_buf_m), (char *)">");

        index++;
    }

    if(disp_state_stack._disp_state[disp_state_stack._disp_index] == PRINTING_UI
       /*|| disp_state_stack._disp_state[disp_state_stack._disp_index] == OPERATE_UI
       || disp_state_stack._disp_state[disp_state_stack._disp_index] == PAUSE_UI*/
      ) {
        titleText_cat(public_buf_m, sizeof(public_buf_m), (char *)":");
        titleText_cat(public_buf_m, sizeof(public_buf_m), tmpCurFileStr);
    }

    if(strlen(public_buf_m) > MAX_TITLE_LEN) {
        ZERO(public_buf_m);
        tmpText = getDispText(0);
        if(*tmpText != 0) {
            titleText_cat(public_buf_m, sizeof(public_buf_m), tmpText);
            titleText_cat(public_buf_m, sizeof(public_buf_m), (char *)">...>");
            tmpText = getDispText(disp_state_stack._disp_index);
            if(*tmpText != 0) titleText_cat(public_buf_m, sizeof(public_buf_m), tmpText);
        }
    }

    return public_buf_m;
}

#if HAS_GCODE_PREVIEW

uint32_t gPicturePreviewStart = 0;

void preview_gcode_prehandle(char *path)
{
#if ENABLED(SDSUPPORT)
    //uint8_t re;
    //uint32_t read;
    uint32_t pre_read_cnt = 0;
    uint32_t *p1;
    char *cur_name;

    gPicturePreviewStart = 0;
    cur_name             = strrchr(path, '/');
    card.openFileRead(cur_name);
    card.read(public_buf, 512);
    p1 = (uint32_t *)strstr((char *)public_buf, ";simage:");

    if(p1) {
        pre_read_cnt = (uint32_t)p1 - (uint32_t)((uint32_t *)(&public_buf[0]));

        To_pre_view              = pre_read_cnt;
        gcode_preview_over       = 1;
        gCfgItems.from_flash_pic = 1;
        update_spi_flash();
    } else {
        gcode_preview_over       = 0;
        default_preview_flg      = 1;
        gCfgItems.from_flash_pic = 0;
        update_spi_flash();
    }
    card.closefile();
#endif
}

#if 0

void gcode_preview(char *path, int xpos_pixel, int ypos_pixel)
{
#if ENABLED(SDSUPPORT)
    //uint8_t ress;
    //uint32_t write;
    volatile uint32_t i, j;
    volatile uint16_t *p_index;
    //int res;
    char *cur_name;

    cur_name = strrchr(path, '/');
    card.openFileRead(cur_name);

    if(gPicturePreviewStart <= 0) {
        while(1) {
            uint32_t br  = card.read(public_buf, 400);
            uint32_t* p1 = (uint32_t *)strstr((char *)public_buf, ";gimage:");
            if(p1) {
                gPicturePreviewStart += (uint32_t)p1 - (uint32_t)((uint32_t *)(&public_buf[0]));
                break;
            } else {
                gPicturePreviewStart += br;
            }
            if(br < 400) break;
        }
    }

    card.setIndex((gPicturePreviewStart + To_pre_view) + size * row + 8);
    SPI_TFT.setWindow(xpos_pixel, ypos_pixel + row, 200, 1);

    j = i = 0;

    while(1) {
        card.read(public_buf, 400);
        for(i = 0; i < 400;) {
            bmp_public_buf[j] = ascii2dec_test((char*)&public_buf[i]) << 4 | ascii2dec_test((char*)&public_buf[i + 1]);
            i                += 2;
            j++;
        }
        if(j >= 400) break;
    }
    for(i = 0; i < 400; i += 2) {
        p_index  = (uint16_t *)(&bmp_public_buf[i]);
        if(*p_index == 0x0000) *p_index = LV_COLOR_BACKGROUND.full;
    }
    SPI_TFT.tftio.WriteSequence((uint16_t*)bmp_public_buf, 200);
#if HAS_BAK_VIEW_IN_FLASH
    W25QXX.init(SPI_QUARTER_SPEED);
    if(row < 20) W25QXX.SPI_FLASH_SectorErase(BAK_VIEW_ADDR_TFT35 + row * 4096);
    W25QXX.SPI_FLASH_BufferWrite(bmp_public_buf, BAK_VIEW_ADDR_TFT35 + row * 400, 400);
#endif
    row++;
    if(row >= 200) {
        size = 809;
        row  = 0;

        gcode_preview_over = 0;
        //flash_preview_begin = 1;

        card.closefile();

        /*
        if (gCurFileState.file_open_flag != 0xAA) {
          reset_file_info();
          res = f_open(file, curFileName, FA_OPEN_EXISTING | FA_READ);
          if (res == FR_OK) {
            f_lseek(file,PREVIEW_SIZE+To_pre_view);
            gCurFileState.file_open_flag = 0xAA;
            //bakup_file_path((uint8_t *)curFileName, strlen(curFileName));
            srcfp = file;
            mksReprint.mks_printer_state = MKS_WORKING;
            once_flag = 0;
          }
        }
        */
        char *cur_name;

        cur_name = strrchr(list_file.file_name[sel_id], '/');

        SdFile file;
        SdFile *curDir;
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
            planner.e_factor[0]        = planner.flow_percentage[0] * 0.01;
#if HAS_MULTI_EXTRUDER
            planner.flow_percentage[1] = 100;
            planner.e_factor[1]        = planner.flow_percentage[1] * 0.01;
#endif
            card.startFileprint();
            TERN_(POWER_LOSS_RECOVERY, recovery.prepare());
            once_flag = 0;
        }
        return;
    }
    card.closefile();
#endif // SDSUPPORT
}

//#else // if 1

void gcode_preview(char *path, int xpos_pixel, int ypos_pixel)
{
#if ENABLED(SDSUPPORT)
    //uint8_t ress;
    //uint32_t write;
    volatile uint32_t i, j;
    volatile uint16_t *p_index;
    //int res;
    char *cur_name;
    uint16_t Color;

    cur_name = strrchr(path, '/');
    card.openFileRead(cur_name);

    card.setIndex((PREVIEW_LITTLE_PIC_SIZE + To_pre_view) + size * row + 8);
#if ENABLED(TFT_LVGL_UI_SPI)
    SPI_TFT.setWindow(xpos_pixel, ypos_pixel + row, 200, 1);
#else
    LCD_setWindowArea(xpos_pixel, ypos_pixel + row, 200, 1);
    LCD_WriteRAM_Prepare();
#endif

    j = 0;
    i = 0;

    while(1) {
        card.read(public_buf, 400);
        for(i = 0; i < 400;) {
            bmp_public_buf[j] = ascii2dec_test((char*)&public_buf[i]) << 4 | ascii2dec_test((char*)&public_buf[i + 1]);
            i += 2;
            j++;
        }

        //if (i > 800) break;
        //#ifdef TFT70
        //  if (j > 400) {
        //    f_read(file, buff_pic, 1, &read);
        //    break;
        //  }
        //#elif defined(TFT35)
        if(j >= 400)
            //f_read(file, buff_pic, 1, &read);
            break;
        //#endif

    }
#if ENABLED(TFT_LVGL_UI_SPI)
    for(i = 0; i < 400;) {
        p_index = (uint16_t *)(&bmp_public_buf[i]);

        Color    = (*p_index >> 8);
        *p_index = Color | ((*p_index & 0xFF) << 8);
        i       += 2;
        if(*p_index == 0x0000) *p_index = 0xC318;
    }
    TFT_CS_L;
    TFT_DC_H;
    SPI.dmaSend(bmp_public_buf, 400, true);
    TFT_CS_H;

#else
    for(i = 0; i < 400;) {
        p_index = (uint16_t *)(&bmp_public_buf[i]);
        if(*p_index == 0x0000) *p_index = 0x18C3;
        LCD_IO_WriteData(*p_index);
        i = i + 2;
    }
#endif
    W25QXX.init(SPI_QUARTER_SPEED);
    if(row < 20)
        W25QXX.SPI_FLASH_SectorErase(BAK_VIEW_ADDR_TFT35 + row * 4096);
    W25QXX.SPI_FLASH_BufferWrite(bmp_public_buf, BAK_VIEW_ADDR_TFT35 + row * 400, 400);
    row++;
    if(row >= 200) {
        size = 809;
        row  = 0;

        gcode_preview_over = 0;
        //flash_preview_begin = 1;

        card.closefile();

        /*
        if (gCurFileState.file_open_flag != 0xAA) {
          reset_file_info();
          res = f_open(file, curFileName, FA_OPEN_EXISTING | FA_READ);
          if (res == FR_OK) {
            f_lseek(file,PREVIEW_SIZE+To_pre_view);
            gCurFileState.file_open_flag = 0xAA;
            //bakup_file_path((uint8_t *)curFileName, strlen(curFileName));
            srcfp = file;
            mksReprint.mks_printer_state = MKS_WORKING;
            once_flag = 0;
          }
        }
        */
        char *cur_name;

        cur_name = strrchr(list_file.file_name[sel_id], '/');

        SdFile file;
        SdFile *curDir;
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
            //saved_feedrate_percentage = feedrate_percentage;
            planner.flow_percentage[0] = 100;
            planner.e_factor[0]        = planner.flow_percentage[0] * 0.01;
#if HAS_MULTI_EXTRUDER
            planner.flow_percentage[1] = 100;
            planner.e_factor[1]        = planner.flow_percentage[1] * 0.01;
#endif
            card.startFileprint();
            TERN_(POWER_LOSS_RECOVERY, recovery.prepare());
            once_flag = 0;
        }
        return;
    }
    card.closefile();
#endif // SDSUPPORT
}

#endif // if 1

#if 0
void Draw_default_preview(int xpos_pixel, int ypos_pixel, uint8_t sel)
{
    int index;
    int y_off = 0;
    W25QXX.init(SPI_QUARTER_SPEED);
    for(index = 0; index < 10; index++) {  // 200*200
#if HAS_BAK_VIEW_IN_FLASH
        if(sel == 1) {
            flash_view_Read(bmp_public_buf, 8000); // 20k
        } else {
            default_view_Read(bmp_public_buf, DEFAULT_VIEW_MAX_SIZE / 10); // 8k
        }
#else
        default_view_Read(bmp_public_buf, DEFAULT_VIEW_MAX_SIZE / 10); // 8k
#endif

        SPI_TFT.setWindow(xpos_pixel, y_off * 20 + ypos_pixel, 200, 20); // 200*200
        SPI_TFT.tftio.WriteSequence((uint16_t*)(bmp_public_buf), DEFAULT_VIEW_MAX_SIZE / 20);

        y_off++;
    }
    W25QXX.init(SPI_QUARTER_SPEED);
}
#endif

void disp_pre_gcode(int xpos_pixel, int ypos_pixel)
{
    if(gcode_preview_over == 1) gcode_preview(list_file.file_name[sel_id], xpos_pixel, ypos_pixel);
#if HAS_BAK_VIEW_IN_FLASH
    if(flash_preview_begin == 1) {
        flash_preview_begin = 0;
        Draw_default_preview(xpos_pixel, ypos_pixel, 1);
    }
#endif
#if HAS_GCODE_DEFAULT_VIEW_IN_FLASH
    if(default_preview_flg == 1) {
        Draw_default_preview(xpos_pixel, ypos_pixel, 0);
        default_preview_flg = 0;
    }
#endif
}
#endif // HAS_GCODE_PREVIEW

void print_time_run()
{
    static uint8_t lastSec = 0;

    if(print_time.seconds >= 60) {
        print_time.seconds = 0;
        print_time.minutes++;
        if(print_time.minutes >= 60) {
            print_time.minutes = 0;
            print_time.hours++;
        }
    }
    if(disp_state == PRINTING_UI) {
        if(lastSec != print_time.seconds) disp_print_time();
        lastSec = print_time.seconds;
    }
}

void GUI_RefreshPage()
{
    if((systick_uptime_millis % 1000) == 0) {
        temperature_change_frequency = 1;
        return_printui_chk = 1;
    }
    if((systick_uptime_millis % 3000) == 0) printing_rate_update_flag = 1;

    if(disp_state == HAND_HEAT_UI || disp_state == OPERATE_UI || disp_state == CHANGE_SPEED_UI
       || disp_state == FAN_UI || disp_state == FILAMENTCHANGE_UI || disp_state == PRINTING_UI) {
        if(return_printui_chk == 1) {
            return_printui_chk = 0;
            if(return_printui_flag == 1 && once_flag == 0) {
                stop_print_time();
                flash_preview_begin = 0;
                default_preview_flg = 0;
                lv_clear_printing();
                lv_draw_dialog(DIALOG_TYPE_FINISH_PRINT);
                once_flag = 1;
                return_printui_flag = 0;
                return;
            }
        }
    }

    switch(disp_state) {
    case SET_UI:
        set_repeat_ops();
        break;
    case TOOL_UI:
        tool_repeat_ops();
        break;
    case PRE_HEAT_UI:
        if(temperature_change_frequency == 1) {
            temperature_change_frequency = 0;
            flash_preHeat_status();
        }
        break;
    case HAND_HEAT_UI:
        if(temperature_change_frequency == 1) {
            temperature_change_frequency = 0;
            disp_desire_temp();
        }
        break;
    case PRINT_FILE_UI:
        if(ui.media_change_flag) {
            ui.media_change_flag = 0;
            lv_clear_ready_print();
            lv_draw_print_file();
        }
        break;

    case PRINTING_UI:
        if(temperature_change_frequency) {
            temperature_change_frequency = 0;
            disp_ext_temp();
            disp_bed_temp();
            disp_fan_speed();
            disp_print_time();
            disp_fan_Zpos();
            disp_print_state();
            disp_move_Speed();
            disp_move_speed_mms();
            disp_extru_Speed();
        }
        if(printing_rate_update_flag || marlin_state == MF_SD_COMPLETE) {
            printing_rate_update_flag = 0;
            if(gcode_preview_over == 0) setProBarRate();
        }
        break;

    case OPERATE_UI:
        options_repeat_ops();
        break;


    case FAN_UI:
        if(temperature_change_frequency == 1) {
            temperature_change_frequency = 0;
            disp_fan_value();
        }
        break;

    case NOZZLE_PROBE_OFFSET_UI:
        if(temperature_change_frequency == 1) {
            flash_zoffset_status();
            temperature_change_frequency = 0;
        }
        break;

    case LEVELING_UI:
        if(temperature_change_frequency == 1) {
            lv_flash_manual_level();
            temperature_change_frequency = 0;
        }
        break;

    case AUTOLEVELING_UI:
        if(temperature_change_frequency == 1) {
            lv_flash_auto_level();
            temperature_change_frequency = 0;
        }
        break;



    case FILAMENTCHANGE_UI:
        if(temperature_change_frequency) {
            temperature_change_frequency = 0;
            disp_filament_temp();
            disp_filament_warn();
        }
        break;
    case DIALOG_UI:
				dialog_temperature_display();
        filament_dialog_handle();
        sensor_dialog_handle();
        TERN_(USE_WIFI_FUNCTION, wifi_scan_handle());
        break;
    case MESHLEVELING_UI:
        /*disp_zpos();*/
        break;
    case HARDWARE_TEST_UI:
        break;
    case WIFI_LIST_UI:
#if ENABLED(USE_WIFI_FUNCTION)
        if(printing_rate_update_flag == 1) {
            disp_wifi_list();
            printing_rate_update_flag = 0;
        }
#endif
        break;
    case KEY_BOARD_UI:
        /*update_password_disp();
        update_join_state_disp();*/
        break;
#if ENABLED(USE_WIFI_FUNCTION)
    case WIFI_TIPS_UI:
        switch(wifi_tips_type) {
        case TIPS_TYPE_JOINING:
            if(wifi_link_state == WIFI_CONNECTED && strcmp((const char *)wifi_list.wifiConnectedName, (const char *)wifi_list.wifiName[wifi_list.nameIndex]) == 0) {
                tips_disp.timer = TIPS_TIMER_STOP;
                tips_disp.timer_count = 0;

                lv_clear_wifi_tips();
                wifi_tips_type = TIPS_TYPE_WIFI_CONECTED;
                lv_draw_wifi_tips();

            }
            if(tips_disp.timer_count >= 30 * 1000) {
                tips_disp.timer = TIPS_TIMER_STOP;
                tips_disp.timer_count = 0;
                lv_clear_wifi_tips();
                wifi_tips_type = TIPS_TYPE_TAILED_JOIN;
                lv_draw_wifi_tips();
            }
            break;
        case TIPS_TYPE_TAILED_JOIN:
            if(tips_disp.timer_count >= 3 * 1000) {
                tips_disp.timer = TIPS_TIMER_STOP;
                tips_disp.timer_count = 0;

                last_disp_state = WIFI_TIPS_UI;
                lv_clear_wifi_tips();
                lv_draw_wifi_list();
            }
            break;
        case TIPS_TYPE_WIFI_CONECTED:
            if(tips_disp.timer_count >= 3 * 1000) {
                tips_disp.timer = TIPS_TIMER_STOP;
                tips_disp.timer_count = 0;

                last_disp_state = WIFI_TIPS_UI;
                lv_clear_wifi_tips();
                lv_draw_wifi();
            }
            break;
        default:
            break;
        }
        break;
#endif

    case BABY_STEP_UI:
        babystep_repeat_ops();
        if(temperature_change_frequency == 1) {
            temperature_change_frequency = 0;
            disp_z_offset_value();
        }
        break;
    default:
        break;
    }

    print_time_run();
}

void clear_cur_ui()
{
    last_disp_state = disp_state_stack._disp_state[disp_state_stack._disp_index];

    switch(disp_state_stack._disp_state[disp_state_stack._disp_index]) {
    case AUTOLEVELING_MENU_UI:
        lv_clear_autolevel_menu();
        break;
    case PRINT_READY_UI:
        //Get_Temperature_Flg = 0;
        lv_clear_ready_print();
        break;
    case PRINT_FILE_UI:
        lv_clear_print_file();
        break;
    case PRINTING_UI:
        lv_clear_printing();
        break;
    case MOVE_MOTOR_UI:
        lv_clear_move_motor();
        break;
    case OPERATE_UI:
        lv_clear_operation();
        break;

    case PRE_HEAT_UI:
        lv_clear_preHeat();
        break;
    case HAND_HEAT_UI:
        lv_clear_handHeat();
        break;
    case CHANGE_SPEED_UI:
        lv_clear_change_speed();
        break;
    case FAN_UI:
        lv_clear_fan();
        break;
    case SET_UI:
        lv_clear_set();
        break;
    case ZERO_UI:
        lv_clear_home();
        break;
    case ABOUT_UI:
        lv_clear_about();
        break;
#if ENABLED(USE_WIFI_FUNCTION)
    case WIFI_UI:
        lv_clear_wifi();
        break;
#endif
    case DIALOG_UI:
        lv_clear_dialog();
        break;
    case FILAMENTCHANGE_UI:
        lv_clear_filament_change();
        break;
    case LEVELING_UI:
        lv_clear_manual_level();
        break;
    case AUTOLEVELING_UI:
        lv_clear_auto_level();
        break;
    case NOZZLE_PROBE_OFFSET_UI:
        lv_clear_auto_level_offset_settings();
        break;
    case TOOL_UI:
        lv_clear_tool();
        break;
#if ENABLED(USE_WIFI_FUNCTION)
    case WIFI_LIST_UI:
        lv_clear_wifi_list();
        break;
#endif
#if ENABLED(USE_WIFI_FUNCTION)
    case WIFI_TIPS_UI:
        lv_clear_wifi_tips();
        break;
#endif
    case BABY_STEP_UI:
        lv_clear_baby_stepping();
        break;
#if HAS_TRINAMIC_CONFIG
    case TMC_CURRENT_UI:
        lv_clear_tmc_current_settings();
        break;
#endif
#if HAS_STEALTHCHOP
    case TMC_MODE_UI:
        lv_clear_tmc_step_mode_settings();
        break;
#endif
#if ENABLED(USE_WIFI_FUNCTION)
    case WIFI_SETTINGS_UI:
        lv_clear_wifi_settings();
        break;
#endif
#if USE_SENSORLESS
    case HOMING_SENSITIVITY_UI:
        lv_clear_homing_sensitivity_settings();
        break;
#endif
#if HAS_ROTARY_ENCODER
    case ENCODER_SETTINGS_UI:
        lv_clear_encoder_settings();
        break;
#endif
    default:
        break;
    }
    //GUI_Clear();
}

void draw_return_ui()
{
    if(disp_state_stack._disp_index > 0) {
        disp_state_stack._disp_index--;

        switch(disp_state_stack._disp_state[disp_state_stack._disp_index]) {
        case AUTOLEVELING_MENU_UI:
            lv_draw_autolevel_menu();
            break;
        case PRINT_READY_UI:
            lv_draw_ready_print();
            break;
        case PRINT_FILE_UI:
            lv_draw_print_file();
            break;
        case PRINTING_UI:
            if(gCfgItems.from_flash_pic == 1) flash_preview_begin = 1;
            else default_preview_flg = 1;
            lv_draw_printing();
            break;
        case MOVE_MOTOR_UI:
            lv_draw_move_motor();
            break;
        case OPERATE_UI:
            lv_draw_operation();
            break;
        case EXTRUSION_UI:
//        lv_draw_extrusion();
            break;
        case PRE_HEAT_UI:
            lv_draw_preHeat();
            break;
        case HAND_HEAT_UI:
            lv_draw_handHeat();
            break;
        case CHANGE_SPEED_UI:
            lv_draw_change_speed();
            break;
        case FAN_UI:
            lv_draw_fan();
            break;
        case SET_UI:
            lv_draw_set();
            break;
        case ZERO_UI:
            lv_draw_tool();
            break;
        case LANGUAGE_UI:
            lv_draw_set();
            break;
        case ABOUT_UI:
            lv_draw_about();
            break;
        case MORE_UI:
            lv_draw_ready_print();
            break;
        case FILAMENTCHANGE_UI:
            lv_draw_filament_change();
            break;
        case LEVELING_UI:
            lv_draw_manual_level();
            break;
        case AUTOLEVELING_UI:
            lv_draw_auto_level();
            break;
        case NOZZLE_PROBE_OFFSET_UI:
            lv_draw_auto_level_offset_settings();
            break;
        case TOOL_UI:
            lv_draw_tool();
            break;
        case WIFI_LIST_UI:
#if ENABLED(USE_WIFI_FUNCTION)
            lv_draw_wifi_list();
#endif
            break;
        case KEY_BOARD_UI:
//        lv_draw_keyboard();
            break;
        case WIFI_TIPS_UI:
#if ENABLED(USE_WIFI_FUNCTION)
            lv_draw_wifi_tips();
#endif
            break;
        case MACHINE_PARA_UI:
            //lv_draw_machine_para();
            break;
        case MACHINE_SETTINGS_UI:
//        lv_draw_machine_settings();
            break;
        case MOTOR_SETTINGS_UI:
//        lv_draw_motor_settings();
            break;
        case FILAMENT_SETTINGS_UI:
//        lv_draw_filament_settings();
            break;
        case LEVELING_PARA_UI:
            //lv_draw_level_settings();
            break;
        case MANUAL_LEVELING_POSIGION_UI:
//        lv_draw_manual_level_pos_settings();
            break;
        case MAXFEEDRATE_UI:
//        lv_draw_max_feedrate_settings();
            break;
        case ACCELERATION_UI:
//        lv_draw_acceleration_settings();
            break;
        case JERK_UI:
#if HAS_CLASSIC_JERK
//          lv_draw_jerk_settings();
#endif
            break;
        case ADVANCED_UI:
//        lv_draw_advance_settings();
            break;
        case NUMBER_KEY_UI:
            lv_draw_auto_level_offset_settings();
            break;
        case BABY_STEP_UI:
            lv_draw_baby_stepping();
            break;
        case PAUSE_POS_UI:
//        lv_draw_pause_position();
            break;
#if HAS_TRINAMIC_CONFIG
        case TMC_CURRENT_UI:
            lv_draw_tmc_current_settings();
            break;
#endif
        case EEPROM_SETTINGS_UI:
//        lv_draw_eeprom_settings();
            break;
#if HAS_STEALTHCHOP
        case TMC_MODE_UI:
            lv_draw_tmc_step_mode_settings();
            break;
#endif
#if ENABLED(USE_WIFI_FUNCTION)
        case WIFI_SETTINGS_UI:
            lv_draw_wifi_settings();
            break;
#endif
#if USE_SENSORLESS
        case HOMING_SENSITIVITY_UI:
            lv_draw_homing_sensitivity_settings();
            break;
#endif
#if HAS_ROTARY_ENCODER
        case ENCODER_SETTINGS_UI:
            lv_draw_encoder_settings();
            break;
#endif
        default:
            break;
        }
    }
}

#if ENABLED(SDSUPPORT)

void sd_detection()
{
    static bool last_sd_status;
    const bool sd_status = IS_SD_INSERTED();
    if(sd_status != last_sd_status) {
        last_sd_status = sd_status;
        if(sd_status) card.mount();
        else card.release();
    }
}

#endif

void lv_ex_line(lv_obj_t * line, lv_point_t *points)
{
    // Copy the previous line and apply the new style
    lv_line_set_points(line, points, 2);     // Set the points
    lv_line_set_style(line, LV_LINE_STYLE_MAIN, &style_line);
    lv_obj_align(line, NULL, LV_ALIGN_IN_TOP_MID, 0, 0);
}

extern volatile uint32_t systick_uptime_millis;

void print_time_count()
{
    if((systick_uptime_millis % 1000) == 0)
        if(print_time.start == 1) print_time.seconds++;
}

void LV_TASK_HANDLER()
{
    //lv_tick_inc(1);
    lv_task_handler();
    if(mks_test_flag == 0x1E) mks_hardware_test();

#if HAS_GCODE_PREVIEW
    disp_pre_gcode(2, 36);
#endif

    GUI_RefreshPage();

#if ENABLED(USE_WIFI_FUNCTION)
    get_wifi_commands();
#endif

    //sd_detection();

#if HAS_ROTARY_ENCODER
    if(gCfgItems.encoder_enable) lv_update_encoder();
#endif
}

void lv_draw_sprayer_temp(lv_obj_t *labInfo)
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
    lv_label_set_text(labInfo, public_buf_l);
}

void lv_draw_bed_temp(lv_obj_t *labInfo)
{
#if HAS_HEATED_BED
    char buf[20] = {0};

    public_buf_l[0] = '\0';
    strcat(public_buf_l, preheat_menu.hotbed);
    sprintf(buf, preheat_menu.value_state, (int)thermalManager.temp_bed.celsius, (int)thermalManager.temp_bed.target);
    strcat_P(public_buf_l, PSTR(": "));
    strcat(public_buf_l, buf);
    lv_label_set_text(labInfo, public_buf_l);
#endif
}

#endif // HAS_TFT_LVGL_UI
