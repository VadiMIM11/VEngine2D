KeyboardControl = {

	OnStart = function(self)
		self.transform = self.actor:GetComponent("Transform")
		player_previous_x = self.transform.x
		player_previous_y = self.transform.y
	end,

	OnUpdate = function(self)

		if gameplay_enabled and not dialogue_running then

			-- Collect Inputs
			local vel_x = 0
			local vel_y = 0

			if Input.GetKeyDown("right") or Input.GetKeyDown("d") then
				vel_x = vel_x + 1
		
			elseif Input.GetKeyDown("left") or Input.GetKeyDown("a") then
				vel_x = vel_x - 1
			
			elseif Input.GetKeyDown("up") or Input.GetKeyDown("w") then
				vel_y = vel_y - 1
			
			elseif Input.GetKeyDown("down") or Input.GetKeyDown("s") then
				vel_y = vel_y + 1
			end

			local new_x = self.transform.x + vel_x
			local new_y = self.transform.y + vel_y

			-- Check if our position actually changed
			if new_x ~= self.transform.x or new_y ~= self.transform.y then

				-- Apply to transform
				if not blocking_map[Round(new_x) .. "," .. Round(new_y)] or blocking_map[Round(new_x) .. "," .. Round(new_y)] == false then

					player_previous_x = self.transform.x
					player_previous_y = self.transform.y

					self.transform.x = new_x
					self.transform.y = new_y

					ProcessTurn()
				end

			end

		end

	end
}

