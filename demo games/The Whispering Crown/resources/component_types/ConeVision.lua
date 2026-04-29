ConeVision = {

	direction = "n",
	vision_tiles = {},
	saw_player = false,

	OnStart = function(self)

    	self.transform = self.actor:GetComponent("Transform")

		self.previous_x = self.transform.x
		self.previous_y = self.transform.y

		self.player_t = Actor.Find("Player"):GetComponent("Transform")
	end,

	OnTurn = function(self)

		if self.enabled == false then
			return
		end

		-- Delete vision tiles from previous turn
		self:DestroyTiles()

		-- Determine direction
		if self.transform.y < self.previous_y then
			self.direction = "n"
		end

		if self.transform.x > self.previous_x then
			self.direction = "e"
		end

		if self.transform.y > self.previous_y then
			self.direction = "s"
		end

		if self.transform.x < self.previous_x then
			self.direction = "w"
		end

		self.previous_x = self.transform.x
		self.previous_y = self.transform.y

		self.saw_player = false

		-- Enemies always have visibility around them.
		self:CreateTileAndCheckForPlayer(self.transform.x, self.transform.y - 1)
		self:CreateTileAndCheckForPlayer(self.transform.x+1, self.transform.y - 1)
		self:CreateTileAndCheckForPlayer(self.transform.x+1, self.transform.y)
		self:CreateTileAndCheckForPlayer(self.transform.x+1, self.transform.y + 1)
		self:CreateTileAndCheckForPlayer(self.transform.x, self.transform.y + 1)
		self:CreateTileAndCheckForPlayer(self.transform.x-1, self.transform.y + 1)
		self:CreateTileAndCheckForPlayer(self.transform.x-1, self.transform.y)
		self:CreateTileAndCheckForPlayer(self.transform.x-1, self.transform.y-1)

		-- create cone of tiles
		if self.direction == "n" then
			self:CreateTileAndCheckForPlayer(self.transform.x, self.transform.y - 2) 
			self:CreateTileAndCheckForPlayer(self.transform.x-1, self.transform.y - 2) 
			self:CreateTileAndCheckForPlayer(self.transform.x+1, self.transform.y - 2) 

			self:CreateTileAndCheckForPlayer(self.transform.x, self.transform.y - 3) 
			self:CreateTileAndCheckForPlayer(self.transform.x-1, self.transform.y - 3) 
			self:CreateTileAndCheckForPlayer(self.transform.x+1, self.transform.y - 3) 
		end

		if self.direction == "e" then
			self:CreateTileAndCheckForPlayer(self.transform.x+2, self.transform.y) 
			self:CreateTileAndCheckForPlayer(self.transform.x+2, self.transform.y+1) 
			self:CreateTileAndCheckForPlayer(self.transform.x+2, self.transform.y-1) 


			self:CreateTileAndCheckForPlayer(self.transform.x+3, self.transform.y) 
			self:CreateTileAndCheckForPlayer(self.transform.x+3, self.transform.y+1) 
			self:CreateTileAndCheckForPlayer(self.transform.x+3, self.transform.y-1) 
		end

		if self.direction == "s" then
			self:CreateTileAndCheckForPlayer(self.transform.x, self.transform.y + 2) 
			self:CreateTileAndCheckForPlayer(self.transform.x-1, self.transform.y + 2) 
			self:CreateTileAndCheckForPlayer(self.transform.x+1, self.transform.y + 2) 


			self:CreateTileAndCheckForPlayer(self.transform.x, self.transform.y + 3) 
			self:CreateTileAndCheckForPlayer(self.transform.x-1, self.transform.y + 3) 
			self:CreateTileAndCheckForPlayer(self.transform.x+1, self.transform.y + 3) 
		end

		if self.direction == "w" then
			self:CreateTileAndCheckForPlayer(self.transform.x-2, self.transform.y) 
			self:CreateTileAndCheckForPlayer(self.transform.x-2, self.transform.y+1) 
			self:CreateTileAndCheckForPlayer(self.transform.x-2, self.transform.y-1) 


			self:CreateTileAndCheckForPlayer(self.transform.x-3, self.transform.y) 
			self:CreateTileAndCheckForPlayer(self.transform.x-3, self.transform.y+1) 
			self:CreateTileAndCheckForPlayer(self.transform.x-3, self.transform.y-1) 
		end

		-- React to player
		if self.saw_player then

			danger = true

			-- Clear fog near position.
			self.radius = 2
			for xfog=-self.radius,self.radius do
				for yfog=-self.radius,self.radius do
					fog_map[Round(self.transform.x + xfog) .. "," .. Round(self.transform.y + yfog)] = false
				end
			end

			-- If we have dialogue, run it.
			local dialogue_component = self.actor:GetComponent("LaunchDialogueWhenNear")
			if dialogue_component ~= nil then
				dialogue_component:LaunchDialogue()
			end

			-- Switch this actor from BasicAIMovement to FollowAIMovement
			local basic_ai_movement = self.actor:GetComponent("BasicAIMovement")
			if basic_ai_movement ~= nil then
				self.actor:RemoveComponent(basic_ai_movement)
			end

			local follow_ai_movement = self.actor:GetComponent("FollowAIMovement")
			if follow_ai_movement == nil then
				self.actor:AddComponent("FollowAIMovement")
			end

			-- Disable Vision Cone
			self.saw_player = false
			self:DestroyTiles()
			self.enabled = false
		end
	end,

	DestroyTiles = function(self)
		for i,actor in ipairs(self.vision_tiles) do
			Actor.Destroy(actor)
		end
		self.vision_tiles = {}
	end,

	Distance = function(p1, p2)
		local dx = p1.x - p2.x
		local dy = p1.y - p2.y
		return math.sqrt(dx*dx + dy*dy)
	end,

	HasLineOfSightToCell = function(self, target_x, target_y)
		local current_x = self.transform.x
		local current_y = self.transform.y

		for i=0,50 do
			-- Move towards target
			local step_size = 0.25
			if current_x < target_x then
				current_x = current_x + step_size
			end

			if current_x > target_x then
				current_x = current_x - step_size
			end

			if current_y < target_y then
				current_y = current_y + step_size
			end

			if current_y > target_y then
				current_y = current_y - step_size
			end

			-- did we hit a blocking tile?
			local check_x = Round(current_x)
			local check_y = Round(current_y)

			if blocking_map[check_x .. "," .. check_y] == true then
				return false
			end

			-- Are we close enough to call it a day?
			local distance = self.Distance({x = current_x, y = current_y}, {x = target_x, y = target_y})
			if distance <= 0.25 then
				return true
			end
		end
	end,

	CreateTileAndCheckForPlayer = function(self, xx, yy)

		-- Don't bother if there is a blocking tile there.
		if blocking_map[Round(xx) .. "," .. Round(yy)] == true then
			return
		end

		if tile_map[xx .. "," .. yy] == nil or tile_map[xx .. "," .. yy] == false then
			return
		end

		-- TODO : Ensure that it is possible to see the target tile xx,yy (ie, enemies cannot see through walls). Bail if we cannot see the target tile from the enemy.
		-- the actor's position may be gained via self.transform.x and self.transform.y

		if self:HasLineOfSightToCell(xx, yy) == false then
			return
		end

		-- Actually create the tile actor and position it.
		local new_tile = Actor.Instantiate("VisionTile")
		local new_tile_t = new_tile:GetComponent("Transform")
		new_tile_t.x = xx
		new_tile_t.y = yy

		-- We need to track our tiles so we can delete them later.
		table.insert(self.vision_tiles, new_tile)

		-- Check for player in cell. Raise the alarm if so.
		local distance = self.Distance({x = self.player_t.x, y = self.player_t.y}, {x = xx, y = yy})
		if(distance < 0.2) then
			self.saw_player = true
		end

		-- Check for cuff distance in cell
		if cuff_following then
			distance = self.Distance({x = cuff_position_x, y = cuff_position_y}, {x = xx, y = yy})
			if(distance < 0.2) then
				self.saw_player = true
			end
		end
	end

}

