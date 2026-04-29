MovingBackground = {
	OnStart = function(self)
		self.transform = self.actor:GetComponent("Transform")
	end,

	OnUpdate = function(self)
		self.transform.x = self.transform.x - 1

		if self.transform.x <= -667 then
			self.transform.x = 667
		end
	end
}

