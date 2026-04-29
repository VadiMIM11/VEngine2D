FollowAIMovement = {

  turns_between_actions = 1,
  friendly = false,

  dirs = {
    { 1,  0},
    {-1,  0},
    { 0,  1},
    { 0, -1},
  },

  distance_field = nil,

  OnStart = function(self)

  self.turn_timer = self.turns_between_actions

    self.transform = self.actor:GetComponent("Transform")

    local player = Actor.Find("Player")
    if player then
      self.player_transform = player:GetComponent("Transform")
    end
  end,

  IsBlocked = function(self, x, y)
    local k = Round(x) .. "," .. Round(y)
    return blocking_map[k] == true
  end,

  BuildDistanceField = function(self, px, py)

    local dist = {}
    local qx, qy = {}, {}
    local head, tail = 1, 1

    local startKey = px .. "," .. py
    dist[startKey] = 0
    qx[tail], qy[tail] = px, py

    while head <= tail do
      local x, y = qx[head], qy[head]
      head = head + 1
      local base = dist[x .. "," .. y]

      for _, d in ipairs(self.dirs) do
        local nx, ny = x + d[1], y + d[2]
        local k = nx .. "," .. ny

        if dist[k] == nil and not self:IsBlocked(nx, ny) then
          dist[k] = base + 1
          tail = tail + 1
          qx[tail], qy[tail] = nx, ny
        end
      end
    end

    self.distance_field = dist
  end,

  NextStepTowardPlayer = function(self, sx, sy)

    local bestDx, bestDy = 0, 0
    local bestDist = math.huge

    for _, d in ipairs(self.dirs) do
      local nx, ny = sx + d[1], sy + d[2]
      local k = nx .. "," .. ny
      local dval = self.distance_field[k]

      if dval and dval < bestDist then
        bestDist = dval
        bestDx, bestDy = d[1], d[2]
      end

	  -- If two choices are equal, sometimes choose one or the other.
	  if dval and dval == bestDist then
		if math.random() > 0.5 then
			bestDist = dval
      bestDx, bestDy = d[1], d[2]
		end
	  end
    end

    return bestDx, bestDy
  end,

	Distance = function(p1, p2)
		local dx = p1.x - p2.x
		local dy = p1.y - p2.y
		return math.sqrt(dx*dx + dy*dy)
	end,

  OnTurn = function(self)
    if not gameplay_enabled then return end
    if not self.player_transform then return end

    self.turn_timer = self.turn_timer - 1
    if self.turn_timer > 0 then
      return
    end
    self.turn_timer = self.turns_between_actions

    local px, py = self.player_transform.x, self.player_transform.y

	local distance = self.Distance({x = px, y = py}, {x = self.transform.x, y = self.transform.y})

  if distance < 1.1 and self.friendly == false then
    	self.transform.x = px
			self.transform.y = py
			return
  end

  if distance < 1.5 and self.friendly == true then
    self.transform.x = player_previous_x
    self.transform.y = player_previous_y

  elseif self.friendly == false or distance > 1.1 then -- if friendly, we only move if we're not too close to the player
      -- Build distance field every turn outward from the player.
      self:BuildDistanceField(px, py)

      local sx, sy = self.transform.x, self.transform.y
      local dx, dy = self:NextStepTowardPlayer(sx, sy)

      if dx ~= 0 or dy ~= 0 then
        local nx, ny = sx + dx, sy + dy
        if not self:IsBlocked(nx, ny) then
          self.transform.x = nx
          self.transform.y = ny
        end
      end
    end

    if self.friendly then
    	cuff_position_x = self.transform.x
		  cuff_position_y = self.transform.y
    end
  end
}