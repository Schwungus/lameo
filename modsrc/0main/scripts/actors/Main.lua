dummy = 0

define_actor("Main", nil, {
    load = function (this)
        load_model("video")
        load_sound("kaupunki")
    end,

    create = function (this)
        this.dummy = get_local_int("dummy", 0)
        set_local_int("dummy", (get_local_int("dummy", 0) + 8) % 160)
        this.dummy2 = 0

        local mdl_video = get_model("video")
        if mdl_video ~= nil then
            local model = this:create_model(fetch_model("video"))
            model:set_hidden(8, 1)
            model:set_hidden(9, 1)
            model:set_hidden(10, 1)
            if ((this.dummy % 25) < 9) then
                model:set_animation(fetch_animation("player/walk"), 0, 1)
            end
        end

        this:play_sound(get_sound("kaupunki"), 1, 128)
    end,

    on_destroy = function (this)
        play_ui_sound(fetch_sound("logo/schwungus"), 0, 0, 1, 1)
        print(get_static_string("description", "(null)"))
    end,

    tick = function (this)
        this.dummy2 = this.dummy2 + (this.dummy * 0.1)
    end,

    draw_ui = function (this)
        local dumb = this.dummy
        local dumb2 = math.fmod(this.dummy2, 128)
        main_string("I am main", nil, 16, 32 + dumb, 32 + dumb + dumb2, UI_Z)
    end,
})
