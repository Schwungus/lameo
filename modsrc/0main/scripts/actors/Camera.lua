define_actor("Camera", nil, {
    create = function (this)
        this:create_camera()
    end,

    create_camera = function (this, camera)
        if this.active then
            camera:set_active()
        end
    end,
})
