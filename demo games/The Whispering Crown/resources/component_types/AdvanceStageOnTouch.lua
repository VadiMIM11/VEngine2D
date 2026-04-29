AdvanceStageOnTouch = {

	stage = 1,

	OnStart = function(self)

		self.transform = self.actor:GetComponent("Transform")
	end,

	FileExists = function(self, path)
		local f = io.open(path, "r")
		if f then
			f:close()
			return true
		end
		return false
	end,

	OnUpdate = function(self)
		
		local player = Actor.Find("Player")
		if player == nil then
			return
		end

		if self.stage == next_stage then
			return
		end

		local player_t = player:GetComponent("Transform")

		-- Check for collision with player.
		local player_pos = {x = player_t.x, y = player_t.y}
		local our_pos = {x = self.transform.x, y = self.transform.y}

		local distance = self.Distance(our_pos, player_pos)
		if (distance < 0.2 and gameplay_enabled) then

			self.transitioning = true
			local transition_component = Actor.Find("SceneTransitionManager"):GetComponent("SceneTransitionManager")
			previous_stage = next_stage
			next_stage = self.stage

			local map_file_path = "resources/data/map" .. next_stage .. ".lua"
			if self:FileExists(map_file_path) == false then
				next_stage = 1
			end
			
			transition_component:RequestChangeScene("poststage")
			gameplay_enabled = false
		end
	end,

	Distance = function(p1, p2)
		local dx = p1.x - p2.x
		local dy = p1.y - p2.y
		return math.sqrt(dx*dx + dy*dy)
	end,
}

