CollectSecretOnTouch = {

	collected = false,

	OnStart = function(self)

		secrets_available = secrets_available + 1

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
		if distance < 0.2 and gameplay_enabled and self.collected == false then

			self.collected = true
			secrets_collected = secrets_collected + 1
			Actor.Destroy(self.actor)
		end
	end,

	Distance = function(p1, p2)
		local dx = p1.x - p2.x
		local dy = p1.y - p2.y
		return math.sqrt(dx*dx + dy*dy)
	end,
}

