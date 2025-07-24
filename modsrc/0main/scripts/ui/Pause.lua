define_ui("Pause", nil, {
    tick = function (this)
        if (get_ui_buttons(UII_BACK) and not get_last_ui_buttons(UII_BACK)) then destroy_ui(this) end
    end,

    draw = function (this)
        main_rectangle(0, 0, DEFAULT_DISPLAY_WIDTH, DEFAULT_DISPLAY_HEIGHT, UI_Z, 0, 0, 0, 128)
        local paused = localized("paused")
        main_string(paused, nil, 16, (DEFAULT_DISPLAY_WIDTH - string_width(paused, nil, 16)) / 2, (DEFAULT_DISPLAY_HEIGHT - string_height(paused, 16)) / 2, UI_Z)
    end,
})
