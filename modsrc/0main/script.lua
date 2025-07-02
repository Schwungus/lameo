-- Pause UI
local functions = {}

functions.load = function ()
end

functions.create = function (self)
    print("Paused")
end

functions.cleanup = function (self)
    print("Unpaused")
end

functions.draw = function (self)
    main_rectangle(0, 0, 640, 480, 1, 0, 0, 0, 128)
    local paused = localized("paused")
    main_string(paused, 0, 16, 320 + math.cos(math.rad(get_draw_time() * 0.25)) * 128, 240, 1)
end

define_ui("Pause", nil, functions)
