CameraManager = {
	zoom_factor = 0.1,
	desired_zoom_factor = 1.0,
	x = 0.0,
	y = 0.0,
	ease_factor = 0.2,
	offset_y = -1,

	OnStart = function(self)
		Camera.SetZoom(self.zoom_factor)
	end,

	OnLateUpdate = function(self)
		local player = Actor.Find("Player")
		if player == nil then
			return
		end

		-- Desired zoom factor is determined by the scroll wheel
		self.desired_zoom_factor = self.desired_zoom_factor + Input.GetMouseScrollDelta() * 0.1
		self.desired_zoom_factor = math.min(math.max (0.1, self.desired_zoom_factor), 2)

		-- Ease zoom
		self.zoom_factor = self.zoom_factor + (self.desired_zoom_factor - self.zoom_factor) * 0.1

		Camera.SetZoom(self.zoom_factor)

		-- Desired camera position is the player's position (with an offset)
		local player_transform = player:GetComponent("Transform")
		local desired_x = player_transform.x
		local desired_y = player_transform.y + self.offset_y

		-- Mouse influences camera position
		local mouse_pos = Input.GetMousePosition()
		desired_x = desired_x + (mouse_pos.x - 576) * 0.01
		desired_y = desired_y + (mouse_pos.y - 256) * 0.01

		-- Ease position
		self.x = self.x + (desired_x - self.x) * self.ease_factor
		self.y = self.y + (desired_y - self.y) * self.ease_factor

		Camera.SetPosition(self.x, self.y)
	end
}

