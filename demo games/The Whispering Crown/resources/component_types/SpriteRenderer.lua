SpriteRenderer = {
	sprite = "???",
	r = 255,
	g = 255,
	b = 255,
	a = 255,
	pivot_x = 0.5,
	pivot_y = 0.5,
	sorting_order = 0,
	auto_sorting_order = false,

	OnStart = function(self)

		self.transform = self.actor:GetComponent("Transform")
	end,

	OnUpdate = function(self)

		-- determine sorting order via y position
		if self.auto_sorting_order then
			self.sorting_order = 0 + self.transform.y
		end

		if table_count(fog_cleared_requesters) <= 0 then
			if fog_map[Round(self.transform.x) .. "," .. Round(self.transform.y)] then
				return
			end
		end

		Image.DrawEx(self.sprite, self.transform.x, self.transform.y, 0, 1, 1, self.pivot_x, self.pivot_y, self.r, self.g, self.b, self.a, self.sorting_order)
	end
}

