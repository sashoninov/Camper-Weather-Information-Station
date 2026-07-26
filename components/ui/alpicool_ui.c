#include "alpicool_ui.h"
#include "alpicool.h"
#include <stdio.h>

/* Local copy of status */
static alpicool_status_t g_ui_status;

/* ============================
 * POWER BUTTON (ImgButton3)
 * ============================ */
void ui_event_power(lv_event_t * e)
{
    lv_obj_t * btn = lv_event_get_target(e);

    /* Toggle power */
    g_ui_status.power = !g_ui_status.power;

    /* Send BLE command */
    alpicool_set_power(g_ui_status.power);

    /* Update icon */
    if (g_ui_status.power)
        lv_imgbtn_set_src(btn, LV_IMGBTN_STATE_RELEASED, NULL, &ui_img_488183857, NULL);
    else
        lv_imgbtn_set_src(btn, LV_IMGBTN_STATE_RELEASED, NULL, &ui_img_669777041, NULL);
}

/* ============================
 * MODE BUTTON (ImgButton4)
 * ============================ */
void ui_event_mode(lv_event_t * e)
{
    lv_obj_t * btn = lv_event_get_target(e);

    /* Toggle mode: 1=ECO, 2=MAX */
    g_ui_status.mode = (g_ui_status.mode == 1) ? 2 : 1;

    /* Send BLE command */
    alpicool_set_mode(g_ui_status.mode == 1 ? 1 : 0);

    /* Update icon */
    if (g_ui_status.mode == 1)
        lv_imgbtn_set_src(btn, LV_IMGBTN_STATE_RELEASED, NULL, &ui_img_147745890, NULL);   // ECO
    else
        lv_imgbtn_set_src(btn, LV_IMGBTN_STATE_RELEASED, NULL, &ui_img_1461797723, NULL);  // MAX
}

/* ============================
 * TEMPERATURE SLIDER
 * ============================ */
void ui_event_temp(lv_event_t * e)
{
    lv_obj_t * slider = lv_event_get_target(e);

    int temp = lv_slider_get_value(slider);
    g_ui_status.set_temp = temp;

    /* Update label */
    char buf[8];
    sprintf(buf, "%d°C", temp);
    lv_label_set_text(ui_LabelTemp, buf);

    /* Send BLE command */
    alpicool_set_temp(temp);
}

/* ============================
 * UPDATE UI FROM BLE STATUS
 * ============================ */
void alpicool_ui_update(const alpicool_status_t *s)
{
    g_ui_status = *s;

    /* Update real temperature */
    char buf[16];
    sprintf(buf, "Naw %d°C", s->real_temp);
    lv_label_set_text(ui_LabelTemp1, buf);

    /* Update set temperature */
    sprintf(buf, " Set %d°C", s->set_temp);
    lv_label_set_text(ui_LabelTemp, buf);
    lv_slider_set_value(ui_SliderTemp, s->set_temp, LV_ANIM_OFF);

    /* Update power icon */
    if (s->power)
        lv_imgbtn_set_src(ui_ImgButton3, LV_IMGBTN_STATE_RELEASED, NULL, &ui_img_488183857, NULL);
    else
        lv_imgbtn_set_src(ui_ImgButton3, LV_IMGBTN_STATE_RELEASED, NULL, &ui_img_669777041, NULL);

    /* Update mode icon */
    if (s->mode == 1)
        lv_imgbtn_set_src(ui_ImgButton4, LV_IMGBTN_STATE_RELEASED, NULL, &ui_img_147745890, NULL);
    else
        lv_imgbtn_set_src(ui_ImgButton4, LV_IMGBTN_STATE_RELEASED, NULL, &ui_img_1461797723, NULL);
}
