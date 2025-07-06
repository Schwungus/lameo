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

functions.create = function (this)
    local table = actor_table(this)
    table.dummy = dummy
    dummy = math.fmod(dummy + 8, 160)
    table.dummy2 = 0
end

functions.on_destroy = function (this)
    play_ui_sound(get_sound("logo/schwungus"), 0, 0, 1, 1)
end

functions.cleanup = function (this)
    print("YOU HAVE KILLED " .. tostring(this))
end

functions.tick = function (this)
    local table = actor_table(this)
    table.dummy2 = table.dummy2 + (table.dummy * 0.1)
end

functions.draw_ui = function (this)
    local table = actor_table(this)
    local dumb = table.dummy
    local dumb2 = math.fmod(table.dummy2, 128)
    main_string("I am main", 0, 16, 32 + dumb, 32 + dumb + dumb2, UI_Z)
end

define_actor("Main", nil, functions)

-- Fake Actor
local functions = {}

functions.draw_ui = function (this)
    local table = actor_table(this)
    local dumb = table.dummy
    local dumb2 = math.fmod(table.dummy2, 128)
    main_string("I am FAKE!!!", 0, 16, 256 - dumb, 256 - dumb - dumb2, UI_Z)
end

define_actor("Fake", "Main", functions)

-- Pause UI
local functions = {}

functions.tick = function (this)
    if (get_ui_buttons(UII_BACK) and not get_last_ui_buttons(UII_BACK)) then destroy_ui(this) end
end

functions.draw = function (this)
    main_rectangle(0, 0, DEFAULT_DISPLAY_WIDTH, DEFAULT_DISPLAY_HEIGHT, 1, 0, 0, 0, 128)
    local paused = localized("paused")
    main_string(paused, 0, 16, (DEFAULT_DISPLAY_WIDTH - string_width(paused, 0, 16)) / 2, (DEFAULT_DISPLAY_HEIGHT - string_height(paused, 16)) / 2, UI_Z)
end

define_ui("Pause", nil, functions)
