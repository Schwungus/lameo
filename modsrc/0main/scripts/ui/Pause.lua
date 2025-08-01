define_ui("Pause", nil, {
    create = function (this)
        this.surface = create_surface(64, 64)
        this.surface2 = create_surface(16, 16)
    end,

    cleanup = function (this)
        this.surface:dispose()
        this.surface2:dispose()
    end,

    tick = function (this)
        if (get_ui_buttons(UII_BACK) and not get_last_ui_buttons(UII_BACK)) then destroy_ui(this) end
    end,

    draw = function (this)
        main_rectangle(0, 0, DEFAULT_DISPLAY_WIDTH, DEFAULT_DISPLAY_HEIGHT, UI_Z, 0, 0, 0, 128)
        local paused = localized("paused")
        main_string(paused, nil, 16, (DEFAULT_DISPLAY_WIDTH - string_width(paused, nil, 16)) / 2, (DEFAULT_DISPLAY_HEIGHT - string_height(paused, 16)) / 2, UI_Z)

        this.surface:push()
        clear_color()

        this.surface2:push()
        clear_color(255)
        pop_surface()

        this.surface2:main(8, 8, 0)
        pop_surface()

        this.surface:main(32, 32, UI_Z)
    end,
})
