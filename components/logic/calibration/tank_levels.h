#pragma once

void update_battery_bar(float voltage);
void update_clean_water_bar(float liters);
void update_dirty_water_bar(int percent_step);

float convert_clean_water_liters(float voltage);
float convert_grey_water_percent(float voltage);