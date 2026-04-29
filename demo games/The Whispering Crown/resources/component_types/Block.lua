Block = {
	transform = nil,

	previous_x = -9999,
	previous_y = -9999,

	OnStart = function(self)
		self.transform = self.actor:GetComponent("Transform")
		self.previous_x = self.transform.x
		self.previous_y = self.transform.y

		blocking_map[Round(self.transform.x) .. "," .. Round(self.transform.y)] = true
	end,

	OnTurn = function(self)
		self.transform = self.actor:GetComponent("Transform")

		if self.transform.x ~= self.previous_x or self.transform.y ~= self.previous_y then

			-- Update new blocking position
			blocking_map[Round(self.transform.x) .. "," .. Round(self.transform.y)] = true

			-- Remove old blocking position
			blocking_map[Round(self.previous_x) .. "," .. (self.previous_y)] = false

			self.previous_x = self.transform.x
			self.previous_y = self.transform.y

		end
	end
}

