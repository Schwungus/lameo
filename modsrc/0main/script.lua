-- Pause UI
local functions = {}

functions.load = function ()
end

functions.create = function (this)
    ui_table(this).dummy = get_draw_time()
    print("Paused")
end

functions.cleanup = function (this)
    print("Unpaused")
end

functions.tick = function (this)
    ui_table(this).dummy = get_draw_time()
    if get_ui_button(UII_BACK) then destroy_ui(this) end
end

functions.draw = function (this)
    main_rectangle(0, 0, DEFAULT_DISPLAY_WIDTH, DEFAULT_DISPLAY_HEIGHT, 1, 0, 0, 0, 128)
    local paused = localized("paused")
    main_string(paused, 0, 16, 320 + math.cos(math.rad(get_draw_time() * 0.2)) * 128, 240 + math.sin(math.rad(ui_table(this).dummy * 0.175)) * 128, UI_Z)

    local cx, cy = get_ui_cursor()
    main_string(localized("loading"), 0, 16, cx, cy, UI_Z)
end

define_ui("Pause", nil, functions)
