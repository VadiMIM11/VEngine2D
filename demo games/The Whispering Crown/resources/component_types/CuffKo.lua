CuffKo = {

	vel_y = -0.04,

	OnStart = function(self)
		self.spriterenderer = self.actor:GetComponent("SpriteRenderer")
		self.transform = self.actor:GetComponent("Transform")

		self.spriterenderer.sprite = "cuff_surprise"
		self.spriterenderer.sorting_order = 9999

		cuff_following = false
	end,

	OnUpdate = function(self) 

		self.vel_y = self.vel_y + 0.002
		self.transform.y = self.transform.y + self.vel_y

		if self.vel_y < 0 then
			self.spriterenderer.sprite = "cuff_surprise"
		else
			self.spriterenderer.sprite = "cuff_ko"
		end

		-- Fallen off screen
		if self.vel_y >= 0.9 then
			Actor.Destroy(self.actor)
		end
	end
}

