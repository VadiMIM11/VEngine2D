Hud = {

	OnStart = function(self)
		dialogue_messages = {}
		danger = false
	end,

	OnLateUpdate = function(self)

		-- Draw shell
		Image.DrawUI("hud_shell", 0, 0)

		if danger and dialogue_running == false then
			Image.DrawUIEx("danger_shell", 0, 0, 255, 255, 255, (math.sin(Application.GetFrame() * 0.1) + 1 / 2.0) * 100, 1)
		end

		dialogue_messages = {}
	end
}

