BasicAIMovement = {

	turns_between_actions = 1,
	vel_x = 0,
	vel_y = 0,

	OnStart = function(self)
		self.turn_timer = self.turns_between_actions
		self.transform = self.actor:GetComponent("Transform")
	end,

	OnTurn = function(self)

		if gameplay_enabled then

		    self.turn_timer = self.turn_timer - 1
			if self.turn_timer > 0 then
				return
			end
			self.turn_timer = self.turns_between_actions

			for i=0,1 do -- Try to move to a new position twice.
				-- Collect Inputs
				local new_x = self.transform.x + self.vel_x
				local new_y = self.transform.y + self.vel_y

				-- Check if our position actually changed
				if new_x ~= self.transform.x or new_y ~= self.transform.y then

					-- Apply to transform
					if not blocking_map[Round(new_x) .. "," .. Round(new_y)] or blocking_map[Round(new_x) .. "," .. Round(new_y)] == false then
						self.transform.x = new_x
						self.transform.y = new_y
						break
					else
						self.vel_x = self.vel_x * -1
						self.vel_y = self.vel_y * -1
					end
				end
			end

		end

	end
}

