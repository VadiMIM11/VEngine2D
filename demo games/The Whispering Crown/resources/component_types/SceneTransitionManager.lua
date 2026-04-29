SceneTransitionManager = {
	current_scene_name = nil,
	desired_scene_name = nil,

	current_alpha = 0,

	OnStart = function(self)
		Scene.DontDestroy(self.actor)
	end,

	OnUpdate = function(self)
		-- Fade in loading image
		if self.current_scene_name ~= self.desired_scene_name then
			if self.current_alpha < 255 then
				self.current_alpha = self.current_alpha + 5
			end
		end

		if self.current_alpha >= 255 and self.current_scene_name ~= self.desired_scene_name then
			Scene.Load(self.desired_scene_name)
			self.current_scene_name = self.desired_scene_name
		end

		-- Fade out loading image
		if self.current_scene_name == self.desired_scene_name then
			if self.current_alpha > 0 then
				self.current_alpha = self.current_alpha - 5
			end
		end

		-- Clamp current_alpha in the 0 255 range
		self.current_alpha = math.max(math.min(self.current_alpha, 255), 0)

		-- Render
		Image.DrawUIEx("loading", 0, 0, 255, 255, 255, self.current_alpha, 999)
	end,

	RequestChangeScene = function(self, scene_name)
		if self.current_scene_name == scene_name or scene_name == nil then
			return
		end

		self.desired_scene_name = scene_name
	end
}

