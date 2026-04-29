Cuff = {

	saw_player = false,
	dead = false,

	OnStart = function(self)


    	self.transform = self.actor:GetComponent("Transform")

		cuff_position_x = self.transform.x
		cuff_position_y = self.transform.y
		cuff_following = false

		self.player_t = Actor.Find("Player"):GetComponent("Transform")
	end,

	Distance = function(p1, p2)
		local dx = p1.x - p2.x
		local dy = p1.y - p2.y
		return math.sqrt(dx*dx + dy*dy)
	end,

	TakeDamage = function(self, force_x, force_y)
		if self.dead == false then
			self.dead = true

			local new_cuff_ko = Actor.Instantiate("CuffKo")
			local t = new_cuff_ko:GetComponent("Transform")

			if force_x ~= nil then
				t.x = force_x
			else
				t.x = self.transform.x
			end

			if force_y ~= nil then
				t.y = force_y
			else
				t.y = self.transform.y
			end
			


			local remove_index = -1
			for i, actor in ipairs(turn_based_actors) do
				local cuff_component = actor:GetComponent("Cuff")
			
				if cuff_component ~= nil then
					remove_index = i
					break
				end
			end
			if remove_index ~= -1 then
				table.remove(turn_based_actors, remove_index)
			end
			
			Actor.Destroy(self.actor)
		end
	end,

	OnTurn = function(self)

		if self.enabled == false then
			return
		end

		local px, py = self.player_t.x, self.player_t.y
		local distance = self.Distance({x = px, y = py}, {x = self.transform.x, y = self.transform.y})

		if distance < 1.1 then
			-- Spotted player. Begin following.
			follow_ai_movement = self.actor:AddComponent("FollowAIMovement")
			follow_ai_movement.friendly = true
			cuff_following = true

			self.enabled = false
		end
	end,

	Distance = function(p1, p2)
		local dx = p1.x - p2.x
		local dy = p1.y - p2.y
		return math.sqrt(dx*dx + dy*dy)
	end

}

