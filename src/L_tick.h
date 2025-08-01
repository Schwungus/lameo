#pragma once

#define TICKRATE 30

void tick_init();
void tick_update();
void tick_teardown();

void reset_ticks();
float get_ticks();
