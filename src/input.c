#include "input.h"

static struct Verb verbs[VERB_SIZE] = {0};
static struct InputPlayer input_players[MAX_INPUT_PLAYERS] = {0};
static enum InputModes input_mode = INM_HOTSWAP;
