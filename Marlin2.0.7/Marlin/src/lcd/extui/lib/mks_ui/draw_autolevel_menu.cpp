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
#include "../../../../gcode/queue.h"
#include "../../../../gcode/gcode.h"
#include "../../../../module/motion.h"
#include "../../../../module/planner.h"
#include "../../../../feature/bedlevel/bedlevel.h"
#include "../../../../feature/bltouch.h"

extern lv_group_t * g;
static lv_obj_t * scr;
static lv_obj_t *labelInfo;

#define ID_ALEVELMENU_ZOFFSET     1
#define ID_ALEVELMENU_OPTION  	  2
#define ID_ALEVELMENU_RETURN		  3

static void event_handler(lv_obj_t * obj, lv_event_t event)
{
    if(event == LV_EVENT_RELEASED) {
        switch(obj->mks_obj_id) {
        case ID_ALEVELMENU_OPTION:
            clear_cur_ui();
            lv_draw_dialog(DIALOG_CHECK_SENSOR);
            break;
        case ID_ALEVELMENU_ZOFFSET:
            if(leveling_is_valid()) {
                clear_cur_ui();
                lv_draw_auto_level_offset_settings();
            }
            break;
        case ID_ALEVELMENU_RETURN:
            clear_cur_ui();
            draw_return_ui();
            break;
        }
    }
}

void lv_draw_autolevel_menu(void)
{
    lv_obj_t *buttonZOffset, *buttonOption, *buttonBack;

    if(disp_state_stack._disp_state[disp_state_stack._disp_index] != AUTOLEVELING_MENU_UI) {
        disp_state_stack._disp_index++;
        disp_state_stack._disp_state[disp_state_stack._disp_index] = AUTOLEVELING_MENU_UI;
    }
    disp_state = AUTOLEVELING_MENU_UI;

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
    buttonZOffset   = lv_imgbtn_create(scr, NULL);
    buttonOption  = lv_imgbtn_create(scr, NULL);
    buttonBack = lv_imgbtn_create(scr, NULL);

    lv_obj_set_event_cb_mks(buttonZOffset, event_handler, ID_ALEVELMENU_ZOFFSET, NULL, 0);
    if(auto_manu_level_sel == 1) {
        if(leveling_is_valid()) {
            lv_imgbtn_set_src(buttonZOffset, LV_BTN_STATE_REL, "F:/bmp_machine_para.bin");
            lv_obj_set_click(buttonZOffset, 1);
        } else {
            lv_imgbtn_set_src(buttonZOffset, LV_BTN_STATE_REL, "F:/bmp_machine_para_dis.bin");
            lv_obj_set_click(buttonZOffset, 0);
        }
    } else {
        lv_imgbtn_set_src(buttonZOffset, LV_BTN_STATE_REL, "F:/bmp_machine_para_dis.bin");
        lv_obj_set_click(buttonZOffset, 0);
    }
    lv_imgbtn_set_src(buttonZOffset, LV_BTN_STATE_PR, "F:/bmp_clear.bin");
    lv_imgbtn_set_style(buttonZOffset, LV_BTN_STATE_PR, &tft_style_label_pre);
    lv_imgbtn_set_style(buttonZOffset, LV_BTN_STATE_REL, &tft_style_label_rel);
    lv_obj_clear_protect(buttonZOffset, LV_PROTECT_FOLLOW);

    lv_obj_set_event_cb_mks(buttonOption, event_handler, ID_ALEVELMENU_OPTION, NULL, 0);
    lv_imgbtn_set_src(buttonOption, LV_BTN_STATE_REL, "F:/bmp_leveling.bin");
    lv_imgbtn_set_src(buttonOption, LV_BTN_STATE_PR, "F:/bmp_clear.bin");
    lv_imgbtn_set_style(buttonOption, LV_BTN_STATE_PR, &tft_style_label_pre);
    lv_imgbtn_set_style(buttonOption, LV_BTN_STATE_REL, &tft_style_label_rel);

    lv_obj_set_event_cb_mks(buttonBack, event_handler, ID_ALEVELMENU_RETURN, NULL, 0);
    lv_imgbtn_set_src(buttonBack, LV_BTN_STATE_REL, "F:/bmp_return.bin");
    lv_imgbtn_set_src(buttonBack, LV_BTN_STATE_PR, "F:/bmp_clear.bin");
    lv_imgbtn_set_style(buttonBack, LV_BTN_STATE_PR, &tft_style_label_pre);
    lv_imgbtn_set_style(buttonBack, LV_BTN_STATE_REL, &tft_style_label_rel);

    lv_obj_set_pos(buttonOption, BTN_ROW1_COL1_POS);
    lv_obj_set_pos(buttonZOffset, BTN_ROW1_COL4_POS);
    lv_obj_set_pos(buttonBack, BTN_ROW2_COL4_POS);

    // Create labels on the image buttons
    lv_btn_set_layout(buttonZOffset, LV_LAYOUT_OFF);
    lv_btn_set_layout(buttonOption, LV_LAYOUT_OFF);
    lv_btn_set_layout(buttonBack, LV_LAYOUT_OFF);

    lv_obj_t *labelZOffset  = lv_label_create(buttonZOffset, NULL);
    lv_obj_t *labelOption = lv_label_create(buttonOption, NULL);
    lv_obj_t *labelBack = lv_label_create(buttonBack, NULL);
    labelInfo = lv_label_create(scr, NULL);

    if(gCfgItems.multiple_language != 0) {
        lv_label_set_text(labelZOffset, set_menu.machine_para);
        lv_obj_align(labelZOffset, buttonZOffset, LV_ALIGN_IN_BOTTOM_MID, 0, BUTTON_TEXT_Y_OFFSET);

        lv_label_set_text(labelOption, tool_menu.leveling);
        lv_obj_align(labelOption, buttonOption, LV_ALIGN_IN_BOTTOM_MID, 0, BUTTON_TEXT_Y_OFFSET);

        lv_label_set_text(labelBack, common_menu.text_back);
        lv_obj_align(labelBack, buttonBack, LV_ALIGN_IN_BOTTOM_MID, 0, BUTTON_TEXT_Y_OFFSET);

        lv_obj_set_style(labelInfo, &tft_style_label_rel);
        lv_label_set_text(labelInfo, leveling_menu.leveloptionssel);
        lv_obj_align(labelInfo, NULL, LV_ALIGN_CENTER, 0, -50);
    }
}

void lv_clear_autolevel_menu()
{
    lv_obj_del(scr);
}

#endif // HAS_TFT_LVGL_UI
