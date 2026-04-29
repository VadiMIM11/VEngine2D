RevealMapWhileTouching = {

	OnStart = function(self)

		self.transform = self.actor:GetComponent("Transform")
	end,

	OnUpdate = function(self)
		
		local player = Actor.Find("Player")
		if player == nil then
			return
		end

		local player_t = player:GetComponent("Transform")

		-- Check for collision with player.
		local player_pos = {x = player_t.x, y = player_t.y}
		local our_pos = {x = self.transform.x, y = self.transform.y}

		local distance = self.Distance(our_pos, player_pos)
		if distance < 0.2 and gameplay_enabled then

			if fog_cleared_requesters[self] ~= true then
				fog_cleared_requesters[self] = true
			end
		else
			fog_cleared_requesters[self] = nil
		end
	end,

	Distance = function(p1, p2)
		local dx = p1.x - p2.x
		local dy = p1.y - p2.y
		return math.sqrt(dx*dx + dy*dy)
	end,
}

