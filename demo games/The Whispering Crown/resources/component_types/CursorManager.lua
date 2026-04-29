CursorManager = {

	OnStart = function(self)
		-- The CursorManager should be ever-present and never go away, even when scenes change.
		Scene.DontDestroy(self.actor)
		Input.HideCursor()
	end,

	OnLateUpdate = function(self)

		local mouse_pos = Input.GetMousePosition()

		-- When the user clicks, we make the cursor more vibrant
		local is_currently_clicking = Input.GetMouseButton(1)
		local color_value = 100
		if is_currently_clicking then
			color_value = 255
		end

		-- Render atop everything, always
		Image.DrawUIEx("cursor", mouse_pos.x, mouse_pos.y, color_value, color_value, color_value, color_value, 999999)
	end
}

