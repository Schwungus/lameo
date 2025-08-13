local Light_create = function (this)
    local light = this:create_light()
    if light ~= nil then
        light:no_lightmap()

        local color = this.color
        if color ~= nil then
            light:set_color(color[1], color[2], color[3], color[4])
        end
    end

    return light
end

define_actor("Light", nil, {create = Light_create})

define_actor("SunLight", "Light", {
    create = function (this)
        local light = Light_create(this)
        if light ~= nil then
            local nx, ny, nz
            local normal = this.normal
            if normal ~= nil then
                nx = normal[1]
                ny = normal[2]
                nz = normal[3]
            else
                nx = 0
                ny = 0
                nz = 1
            end

            light:set_sun(nx, ny, nz)
            light:snap()
        end
    end
})

define_actor("PointLight", "Light", {
    create = function (this)
        local light = Light_create(this)
        if light ~= nil then
            local near, far
            local range = this.range
            if range ~= nil then
                near = range[1]
                far = range[2]
            else
                near = 0
                far = 1
            end

            light:set_point(near, far)
            light:snap()
        end
    end
})
