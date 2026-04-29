ClearFog = {

	radius = 2,
	previous_x = -999,
	previous_y = -999,

	OnStart = function(self)
		self.transform = self.actor:GetComponent("Transform")
	end,

	OnUpdate = function(self)

		if self.transform.x ~= self.previous_x or self.transform.y ~= self.previous_y then
			self.previous_x = self.transform.x
			self.previous_y = self.transform.y

			-- Clear fog near position.
			for xfog=-self.radius,self.radius do
				for yfog=-self.radius,self.radius do
					fog_map[Round(self.transform.x + xfog) .. "," .. Round(self.transform.y + yfog)] = false
				end
			end
		end
	end
}

