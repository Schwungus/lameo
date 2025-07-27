dummy = 0

define_actor("Main", nil, {
    create = function (this)
        this.dummy = dummy
        dummy = math.fmod(dummy + 8, 160)
        this.dummy2 = 0

        local model = this:create_model(fetch_model("video"))
        model:set_hidden(8, 1)
        model:set_hidden(9, 1)
        model:set_hidden(10, 1)
        if ((this.dummy % 25) < 9) then
            model:set_animation(fetch_animation("player/walk"), 0, 1)
        end
    end,

    on_destroy = function (this)
        play_ui_sound(fetch_sound("logo/schwungus"), 0, 0, 1, 1)
        print(get_static_string("description", "(null)"))
    end,

    tick = function (this)
        this.dummy2 = this.dummy2 + (this.dummy * 0.1)

        local player = get_player(0)

        local buttons = player:get_buttons()
        local last_buttons = player:get_last_buttons()
        if (buttons & PB_INVENTORY1) == PB_INVENTORY1 and (last_buttons & PB_INVENTORY1) ~= PB_INVENTORY1 then
            print(player:get_int("hp", 0))
        end
        if (buttons & PB_INVENTORY2) == PB_INVENTORY2 and (last_buttons & PB_INVENTORY2) ~= PB_INVENTORY2 then
            player:set_int("hp", math.floor(player:get_int("hp") / 2))
        end

        local move = {player:get_move()}
        this:set_pos(this:get_x() - (this:get_roll() * 0.01) + (move[1] / 32767), this:get_y() + (move[2] / 32767))

        local aim = {player:get_aim()}
        this:set_angle(this:get_yaw() + (aim[1] / 32767) * 10, this:get_pitch() + (aim[2] / 32767) * 10)
    end,

    draw_ui = function (this)
        local dumb = this.dummy
        local dumb2 = math.fmod(this.dummy2, 128)
        main_string("I am main", nil, 16, 32 + dumb, 32 + dumb + dumb2, UI_Z)
    end,
})
