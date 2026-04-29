NPC = {
	dialogue = "...",
	dialogue_distance = 2,

	OnStart = function(self)
		if game_over then
			return
		end
		
		self.transform = self.actor:GetComponent("Transform")
		self.spriterenderer = self.actor:GetComponent("SpriteRenderer")
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
		if distance < self.dialogue_distance then
			table.insert(dialogue_messages, self.dialogue)
		end
	end,

	Distance = function(p1, p2)
		local dx = p1.x - p2.x
		local dy = p1.y - p2.y
		return math.sqrt(dx*dx + dy*dy)
	end,
}

