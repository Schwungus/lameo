define_actor("Player", nil, {
    create = function (this)
        this:create_camera()
    end,

    tick = function (this)
        local player = this:get_player()
        if (player == nil) then
            return
        end

        local aim = {player:get_aim()}
        this:set_angle(this:get_yaw() - aim[1], this:get_pitch() - aim[2])

        local move = {player:get_move()}
        local dx = {lengthdir_3d(move[1] * 2, this:get_yaw(), this:get_pitch())}
        local dy = {lengthdir(move[2] * 2, this:get_yaw() + 90)}
        this:set_pos(this:get_x() - dx[1] - dy[1], this:get_y() - dx[2] - dy[2], this:get_z() - dx[3])
    end,
})
