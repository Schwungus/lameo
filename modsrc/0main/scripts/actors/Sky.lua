define_actor("Sky", nil, {
    load = function (this)
        load_model("sky")
    end,

    create = function (this)
        this:create_model(get_model("sky"))
        this:to_sky()
    end,
})
