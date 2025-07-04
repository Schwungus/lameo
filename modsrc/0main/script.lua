-- Pause UI
local functions = {}

functions.tick = function (this)
    ui_table(this).dummy = get_draw_time()
    if (get_ui_buttons(UII_BACK) and not get_last_ui_buttons(UII_BACK)) then destroy_ui(this) end
end

functions.draw = function (this)
    main_rectangle(0, 0, DEFAULT_DISPLAY_WIDTH, DEFAULT_DISPLAY_HEIGHT, 1, 0, 0, 0, 128)
    local paused = localized("paused")
    main_string(paused, 0, 16, (DEFAULT_DISPLAY_WIDTH - string_width(paused, 0, 16)) / 2, (DEFAULT_DISPLAY_HEIGHT - string_height(paused, 16)) / 2, UI_Z)
end

define_ui("Pause", nil, functions)
