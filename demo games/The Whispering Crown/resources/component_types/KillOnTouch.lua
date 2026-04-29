KillOnTouch = {

	OnStart = function(self)
		if game_over then
			return
		end
		
		self.transform = self.actor:GetComponent("Transform")
		self.spriterenderer = self.actor:GetComponent("SpriteRenderer")
	end,

	OnTurn = function(self)
		local player = Actor.Find("Player")
		if player == nil then
			return
		end

		local player_t = player:GetComponent("Transform")

		-- Check for collision with player.
		local player_pos = {x = player_t.x, y = player_t.y}
		local our_pos = {x = self.transform.x, y = self.transform.y}

		local distance = self.Distance(our_pos, player_pos)

		if distance < 0.1 then
			player:GetComponent("Player"):TakeDamage()
		end

		-- Check for collision with Cuff.
		if cuff_following then
			local cuff = Actor.Find("Cuff")
			if cuff ~= nil then
				local cuff_pos = {x = cuff_position_x, y = cuff_position_y}
				distance = self.Distance(our_pos, cuff_pos)

				if distance < 0.1 then
					cuff:GetComponent("Cuff"):TakeDamage()
				end
			end

		end



	end,

	Distance = function(p1, p2)
		local dx = p1.x - p2.x
		local dy = p1.y - p2.y
		return math.sqrt(dx*dx + dy*dy)
	end,
}

