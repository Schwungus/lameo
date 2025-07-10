-- Camera
local functions = {}

functions.create = function (this)
    this:create_camera()
end

define_actor("Camera", nil, functions)

-- Main Actor
dummy = 0

local functions = {}

functions.load = function()
    load_texture("logo/schwungus/full")
    load_texture("logo/sdl")
    load_texture("logo/fmod")
    load_texture("logo/lua")
    load_sound("logo/schwungus")
end

function Main_create(this)
    this.dummy = dummy
    dummy = math.fmod(dummy + 8, 160)
    this.dummy2 = 0
end

functions.create = Main_create

functions.on_destroy = function (this)
    play_ui_sound(get_sound("logo/schwungus"), 0, 0, 1, 1)
end

functions.tick = function (this)
    this.dummy2 = this.dummy2 + (this.dummy * 0.1)
end

functions.draw_ui = function (this)
    local dumb = this.dummy
    local dumb2 = math.fmod(this.dummy2, 128)
    main_string("I am main", nil, 16, 32 + dumb, 32 + dumb + dumb2, UI_Z)
end

define_actor("Main", nil, functions)

-- Pause UI
local functions = {}

functions.tick = function (this)
    if (get_ui_buttons(UII_BACK) and not get_last_ui_buttons(UII_BACK)) then destroy_ui(this) end
end

functions.draw = function (this)
    main_rectangle(0, 0, DEFAULT_DISPLAY_WIDTH, DEFAULT_DISPLAY_HEIGHT, 1, 0, 0, 0, 128)
    local paused = localized("paused")
    main_string(paused, nil, 16, (DEFAULT_DISPLAY_WIDTH - string_width(paused, nil, 16)) / 2, (DEFAULT_DISPLAY_HEIGHT - string_height(paused, 16)) / 2, UI_Z)
end

define_ui("Pause", nil, functions)
