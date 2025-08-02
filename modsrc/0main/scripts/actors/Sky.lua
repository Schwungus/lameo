define_actor("Sky", nil, {
    load = function (this)
        load_model("sky")
    end,

    create = function (this)
        local model = this:create_model(get_model("sky"))
        local color = this.color
        if color ~= nil then
            model:set_color(color[1], color[2], color[3], color[4])
        end
        this:to_sky()
    end,
})
