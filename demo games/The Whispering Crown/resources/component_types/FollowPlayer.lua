FollowPlayer = {
	frames_until_move = 60,

	OnStart = function(self)
		self.transform = self.actor:GetComponent("Transform")
	end,

	OnUpdate = function(self)
	
		self.frames_until_move = self.frames_until_move - 1
		
		if self.frames_until_move <= 0 then
			-- Determine where player is, and how we need to move to follow them naively.
			-- ie, we will run into walls trying to follow the player. No pathfinding.
			local player_actor = Actor.Find("Player")
			if player_actor == nil then
				return
			end
			
			local player_transform = player_actor:GetComponent("Transform")
			local delta_x  = player_transform.x - self.transform.x
			local delta_y = player_transform.y - self.transform.y
			local final_movement_x = 0
			local final_movement_y = 0
			
			if delta_x == 0 and delta_y == 0 then
				return
			end

			-- Determine whether to move horizontally or vertically.
			if math.abs(delta_x) > math.abs(delta_y) then
				if delta_x > 0 then
					final_movement_x = 1
				else
					final_movement_x = -1
				end
			else
				if delta_y > 0 then
					final_movement_y = 1
				else
					final_movement_y = -1
				end
			end
			
			-- Apply our movement decision
			self.transform.x = self.transform.x + final_movement_x
			self.transform.y = self.transform.y + final_movement_y
			
			-- Set up for the next movement to happen in the future
			self.frames_until_move = 60
		end
	end
}

