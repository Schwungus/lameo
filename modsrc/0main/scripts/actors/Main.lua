define_actor("Main", nil, {
    load = function (this)
        load_texture("video/eyes/open")
        load_texture("video/eyes/pupil")
        load_model("video")
        load_sound("kaupunki")
    end,

    create = function (this)
        this.dummy = get_local_int("dummy", 0)
        set_local_int("dummy", (get_local_int("dummy", 0) + 8) % 160)
        this.dummy2 = 0

        tex_video_eyes_open = get_texture("video/eyes/open")
        tex_video_eyes_pupil = get_texture("video/eyes/pupil")
        this.eyes = create_surface(64, 32)

        local mdl_video = get_model("video")
        if mdl_video ~= nil then
            local model = this:create_model(mdl_video)
            model:set_hidden(8, 1)
            model:set_hidden(9, 1)
            model:set_hidden(10, 1)
            model:override_texture_surface(2, this.eyes)
            model:set_animation(fetch_animation("player/walk"), this.dummy, 1)
        end

        this:play_sound(get_sound("kaupunki"), 1, 128)
    end,

    cleanup = function (this)
        this.eyes:dispose()
    end,

    on_destroy = function (this)
        play_ui_sound(fetch_sound("logo/schwungus"), 0, 0, 1, 1)
        print(get_static_string("description", "(null)"))
    end,

    tick = function (this)
        this.eyes:push()
        clear_color(0.9882352941176471, 0.8980392156862745, 0.7411764705882353)
        main_sprite(tex_video_eyes_pupil, 13, 13)
        main_sprite(tex_video_eyes_pupil, 35, 13)
        main_sprite(tex_video_eyes_open, 0, 0)
        pop_surface()

        this.dummy2 = this.dummy2 + (this.dummy * 0.1)
    end,

    draw_screen = function (this, camera)
        local dumb = this.dummy
        local dumb2 = math.fmod(this.dummy2, 128)
        main_string("I am main", nil, 16, 32 + dumb, 32 + dumb + dumb2, UI_Z)
    end,
})
