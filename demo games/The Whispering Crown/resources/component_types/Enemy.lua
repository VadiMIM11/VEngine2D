Enemy = {
	hp = 3,
	damage_flash_cooldown = 5,

	OnStart = function(self)
		if game_over then
			return
		end
		
		self.donna = Actor.Find("donna"):GetComponent("Donna")
		self.donna_transform = Actor.Find("donna"):GetComponent("Transform")
		self.transform = self.actor:GetComponent("Transform")
		self.spriterenderer = self.actor:GetComponent("SpriteRenderer")

		-- Register this enemy in the global table.
		table.insert(all_enemies, self)
	end,

	OnUpdate = function(self)
		if game_over then
			return
		end

		-- Check for collision with player.
		local player_pos = {x = self.donna_transform.x, y = self.donna_transform.y}
		local our_pos = {x = self.transform.x, y = self.transform.y}

		local distance = self.Distance(our_pos, player_pos)
		if distance < 50 then
			self.donna:TakeDamage()
		end

		-- check for collision with projectile.
		for i, projectile in ipairs(all_projectiles) do
			local projectile_transform = projectile.actor:GetComponent("Transform")
			local projectile_position = {x = projectile_transform.x, y = projectile_transform.y}
			distance = self.Distance(our_pos, projectile_position)
			if distance <= 50 then
				self:TakeDamage()
				projectile.destroyed = true
			end
		end

		-- Damage Flash
		if self.damage_flash_cooldown > 0 then
			self.damage_flash_cooldown = self.damage_flash_cooldown - 1
			if Application.GetFrame() % 4 > 1 then
				self.spriterenderer.g = 0
				self.spriterenderer.b = 0
			else
				self.spriterenderer.g = 255
				self.spriterenderer.b = 255
			end
		else
			self.spriterenderer.g = 255
			self.spriterenderer.b = 255
		end

	end,

	-- This OnDestroy function is not a lifecycle function, and is not automatically called from the engine.
	-- It is called manually by other lua components.
	-- In homework 8, you'll make this an automatically-called lifecycle function.
	OnDestroy = function(self)
		-- Remove this enemy from the global table.
		for i, enemy in ipairs(all_enemies) do
			if enemy == self then
				table.remove(all_enemies, i)
				break
			end
		end
	end,

	Distance = function(p1, p2)
		local dx = p1.x - p2.x
		local dy = p1.y - p2.y
		return math.sqrt(dx*dx + dy*dy)
	end,

	TakeDamage = function(self)
		self.damage_flash_cooldown = 5
		self.hp = self.hp - 1

		if self.hp <= 0 then
			score = score + 1
			Actor.Destroy(self.actor)
		end
	end
}

