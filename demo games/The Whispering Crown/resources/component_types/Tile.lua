Tile = {
	transform = nil,

	OnStart = function(self)
		self.transform = self.actor:GetComponent("Transform")

		tile_map[self.transform.x .. "," .. self.transform.y] = true
	end
}

