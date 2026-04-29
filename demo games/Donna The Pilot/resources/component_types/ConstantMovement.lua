ConstantMovement = {
	x_vel = 0,
	y_vel = 0,

	OnStart = function(self)
		self.transform = self.actor:GetComponent("Transform")
	end,

	OnUpdate = function(self)
		self.transform.x = self.transform.x + self.x_vel
		self.transform.y = self.transform.y + self.y_vel
	end
}

