GameDirector = {}

local SCREEN_W = 1280
local SCREEN_H = 720
local PPM = 100
local TAU = math.pi * 2
local DEG = 180 / math.pi

local function clamp(v, lo, hi)
	if v < lo then return lo end
	if v > hi then return hi end
	return v
end

local function rnd(lo, hi)
	return lo + math.random() * (hi - lo)
end

local function dist_sq(ax, ay, bx, by)
	local dx = ax - bx
	local dy = ay - by
	return dx * dx + dy * dy
end

local function dist(ax, ay, bx, by)
	return math.sqrt(dist_sq(ax, ay, bx, by))
end

local function normalize(x, y)
	local l = math.sqrt(x * x + y * y)
	if l <= 0.0001 then return 0, 0, 0 end
	return x / l, y / l, l
end

local function approach(v, target, amount)
	if v < target then
		return math.min(v + amount, target)
	end
	return math.max(v - amount, target)
end

local function screen_to_world(cam_x, cam_y, sx, sy)
	return cam_x + (sx - SCREEN_W * 0.5) / PPM, cam_y + (sy - SCREEN_H * 0.5) / PPM
end

local function draw_beam(image, x1, y1, x2, y2, width_scale, r, g, b, a, order)
	local dx = x2 - x1
	local dy = y2 - y1
	local length = math.sqrt(dx * dx + dy * dy)
	if length <= 0.001 then return end

	local cx = (x1 + x2) * 0.5
	local cy = (y1 + y2) * 0.5
	local angle = math.atan(dx, dy) * DEG
	Image.DrawEx(image, cx, cy, angle, width_scale, length / 1.28, 0.5, 0.5, r, g, b, a, order)
end

local function dist_to_segment(px, py, x1, y1, x2, y2)
	local vx = x2 - x1
	local vy = y2 - y1
	local wx = px - x1
	local wy = py - y1
	local len2 = vx * vx + vy * vy
	if len2 <= 0.0001 then return dist(px, py, x1, y1), 0 end

	local t = clamp((wx * vx + wy * vy) / len2, 0, 1)
	local cx = x1 + vx * t
	local cy = y1 + vy * t
	return dist(px, py, cx, cy), t
end

local function set_message(g, text, frames)
	g.message = text
	g.message_timer = frames or 120
end

function GameDirector:Reset()
	self.g = {
		frame = 0,
		mode = "play",
		score = 0,
		best_combo = 1,
		combo = 1,
		combo_timer = 0,
		collected = 0,
		needed = 8,
		arena_radius = 11.8,
		shake = 0,
		cam_x = 0,
		cam_y = 0,
		message = "RIFTBOUND",
		message_timer = 160,
		spawn_timer = 70,
		hazard_timer = 0,
		rift_spin = 0,
		rift_open = false,
		win_frame = 0,
		player = {
			x = 0,
			y = 2.8,
			vx = 0,
			vy = -0.01,
			angle = -math.pi * 0.5,
			shield = 5,
			max_shield = 5,
			energy = 1,
			dash = 1,
			pulse = 1,
			fire_cd = 0,
			invuln = 0,
			trail = {}
		},
		shards = {},
		enemies = {},
		bullets = {},
		sparks = {},
		sentinels = {}
	}

	for i = 1, 12 do
		self:SpawnShard(i * TAU / 12 + rnd(-0.15, 0.15), rnd(3.0, 10.4))
	end

	for i = 1, 3 do
		local a = i * TAU / 3 + 0.5
		table.insert(self.g.sentinels, {
			angle = a,
			radius = 7.6 + i * 0.35,
			spin = (i == 2 and -0.0048 or 0.0042) * (1 + i * 0.18),
			beam = a + TAU * 0.25,
			beam_spin = (i == 2 and 0.018 or -0.014),
			hp = 3,
			alive = true,
			hit_flash = 0
		})
	end

	Audio.Halt(0)
	Audio.Halt(1)
	Audio.Halt(2)
	Audio.SetVolume(0, 54)
	Audio.SetVolume(1, 70)
	Audio.SetVolume(2, 82)
	Audio.Play(4, "hum", true)
end

function GameDirector:OnStart()
	math.randomseed(os.time() % 100000)
	Input.HideCursor()
	Camera.SetZoom(1.0)
	self:Reset()
end

function GameDirector:SpawnShard(angle, radius)
	local g = self.g
	local x = math.cos(angle) * radius
	local y = math.sin(angle) * radius
	table.insert(g.shards, {
		base_x = x,
		base_y = y,
		x = x,
		y = y,
		seed = rnd(0, TAU),
		rot = rnd(0, 360),
		pulse = rnd(0, TAU)
	})
end

function GameDirector:AddSpark(x, y, count, r, gg, b, power)
	local g = self.g
	for i = 1, count do
		local a = rnd(0, TAU)
		local s = rnd(0.015, power or 0.13)
		table.insert(g.sparks, {
			x = x,
			y = y,
			vx = math.cos(a) * s,
			vy = math.sin(a) * s,
			life = math.floor(rnd(18, 48)),
			max = 48,
			r = r,
			g = gg,
			b = b,
			size = rnd(0.05, 0.18)
		})
	end
end

function GameDirector:SpawnEnemy()
	local g = self.g
	local player = g.player
	local a = rnd(0, TAU)
	local edge = g.arena_radius + 1.1
	local kind = "seeker"
	if g.collected >= 4 and math.random() < 0.32 then kind = "wraith" end
	if g.collected >= 6 and math.random() < 0.20 then kind = "brute" end

	local speed = kind == "brute" and 0.012 or 0.018
	if kind == "wraith" then speed = 0.024 end
	table.insert(g.enemies, {
		kind = kind,
		x = player.x + math.cos(a) * edge,
		y = player.y + math.sin(a) * edge,
		vx = -math.cos(a) * speed,
		vy = -math.sin(a) * speed,
		hp = kind == "brute" and 3 or 1,
		phase = rnd(0, TAU),
		hit_flash = 0
	})
end

function GameDirector:Shoot(ax, ay)
	local g = self.g
	local p = g.player
	if p.fire_cd > 0 or p.energy < 0.105 then return end

	local bx = p.x + ax * 0.44
	local by = p.y + ay * 0.44
	table.insert(g.bullets, {
		x = bx,
		y = by,
		vx = ax * 0.29 + p.vx * 0.35,
		vy = ay * 0.29 + p.vy * 0.35,
		life = 62,
		rot = math.atan(ay, ax) * DEG + 90
	})
	p.fire_cd = 8
	p.energy = p.energy - 0.105
	g.shake = math.max(g.shake, 4)
	Audio.Play(0, "laser", false)
	self:AddSpark(bx, by, 5, 100, 235, 255, 0.07)
end

function GameDirector:Pulse()
	local g = self.g
	local p = g.player
	if p.pulse < 1 then return end

	p.pulse = 0
	g.shake = math.max(g.shake, 16)
	set_message(g, "GRAVITY BLOOM", 70)
	Audio.Play(1, "pulse", false)
	self:AddSpark(p.x, p.y, 48, 255, 220, 104, 0.19)

	for _, e in ipairs(g.enemies) do
		local nx, ny, d = normalize(e.x - p.x, e.y - p.y)
		if d < 3.0 then
			local push = (3.0 - d) * 0.04
			e.vx = e.vx + nx * push
			e.vy = e.vy + ny * push
			e.hit_flash = 14
			e.hp = e.hp - 1
		end
	end

	for i = #g.shards, 1, -1 do
		local s = g.shards[i]
		if dist(p.x, p.y, s.x, s.y) < 2.4 then
			self:CollectShard(i)
		end
	end
end

function GameDirector:CollectShard(index)
	local g = self.g
	local s = g.shards[index]
	if not s then return end

	g.collected = g.collected + 1
	g.combo = g.combo + 1
	g.best_combo = math.max(g.best_combo, g.combo)
	g.combo_timer = 160
	g.score = g.score + 150 * g.combo
	table.remove(g.shards, index)
	self:AddSpark(s.x, s.y, 28, 113, 255, 199, 0.15)
	Audio.Play(2, "collect", false)

	if g.collected == g.needed then
		g.rift_open = true
		g.shake = math.max(g.shake, 24)
		set_message(g, "RIFT UNLOCKED", 150)
		Audio.Play(3, "warp", false)
	elseif g.collected < g.needed then
		set_message(g, "SIGNAL SHARD +" .. tostring(g.combo), 62)
	end
end

function GameDirector:DamagePlayer(amount, nx, ny)
	local g = self.g
	local p = g.player
	if p.invuln > 0 or g.mode ~= "play" then return end

	p.shield = p.shield - amount
	p.invuln = 82
	p.vx = p.vx + (nx or 0) * 0.13
	p.vy = p.vy + (ny or 0) * 0.13
	g.combo = 1
	g.combo_timer = 0
	g.shake = 30
	self:AddSpark(p.x, p.y, 36, 255, 79, 116, 0.20)
	Audio.Play(2, "hit", false)

	if p.shield <= 0 then
		g.mode = "over"
		set_message(g, "SIGNAL LOST", 9999)
		Audio.Play(3, "fail", false)
	else
		set_message(g, "HULL BREACH", 80)
	end
end

function GameDirector:UpdatePlayer()
	local g = self.g
	local p = g.player

	local ax = 0
	local ay = 0
	if Input.GetKey("w") or Input.GetKey("up") then ay = ay - 1 end
	if Input.GetKey("s") or Input.GetKey("down") then ay = ay + 1 end
	if Input.GetKey("a") or Input.GetKey("left") then ax = ax - 1 end
	if Input.GetKey("d") or Input.GetKey("right") then ax = ax + 1 end
	local nx, ny, moving = normalize(ax, ay)

	if moving > 0 then
		p.vx = p.vx + nx * 0.0062
		p.vy = p.vy + ny * 0.0062
		self:AddSpark(p.x - nx * 0.22, p.y - ny * 0.22, 1, 80, 210, 255, 0.035)
	end

	p.vx = p.vx * 0.986
	p.vy = p.vy * 0.986
	local sx, sy, speed = normalize(p.vx, p.vy)
	local max_speed = 0.115
	if speed > max_speed then
		p.vx = sx * max_speed
		p.vy = sy * max_speed
	end

	local mx = Input.GetMousePosition().x
	local my = Input.GetMousePosition().y
	if mx == 0 and my == 0 then
		mx = SCREEN_W * 0.5 + math.cos(p.angle) * 170
		my = SCREEN_H * 0.5 + math.sin(p.angle) * 170
	end
	local aim_x, aim_y = screen_to_world(g.cam_x, g.cam_y, mx, my)
	local aim_dx = aim_x - p.x
	local aim_dy = aim_y - p.y
	local aim_nx, aim_ny, aim_len = normalize(aim_dx, aim_dy)
	if aim_len <= 0.08 then
		aim_nx = math.cos(p.angle)
		aim_ny = math.sin(p.angle)
	end
	p.angle = math.atan(aim_ny, aim_nx)

	if Input.GetKeyDown("lshift") or Input.GetKeyDown("rshift") then
		if p.dash >= 1 then
			local dx = moving > 0 and nx or aim_nx
			local dy = moving > 0 and ny or aim_ny
			p.vx = p.vx + dx * 0.22
			p.vy = p.vy + dy * 0.22
			p.dash = 0
			g.shake = math.max(g.shake, 18)
			Audio.Play(1, "dash", false)
			self:AddSpark(p.x, p.y, 32, 86, 188, 255, 0.18)
		end
	end

	if Input.GetMouseButtonDown(1) or Input.GetKeyDown("space") then
		self:Shoot(aim_nx, aim_ny)
	end

	if Input.GetMouseButtonDown(3) or Input.GetKeyDown("e") then
		self:Pulse()
	end

	p.x = p.x + p.vx
	p.y = p.y + p.vy
	local edge = math.sqrt(p.x * p.x + p.y * p.y)
	if edge > g.arena_radius then
		local bx, by = normalize(p.x, p.y)
		p.x = bx * g.arena_radius
		p.y = by * g.arena_radius
		p.vx = p.vx - bx * 0.025
		p.vy = p.vy - by * 0.025
		if g.frame % 38 == 0 then
			self:DamagePlayer(1, -bx, -by)
		end
	end

	p.fire_cd = math.max(0, p.fire_cd - 1)
	p.invuln = math.max(0, p.invuln - 1)
	p.energy = clamp(p.energy + 0.010, 0, 1)
	p.dash = clamp(p.dash + 0.014, 0, 1)
	p.pulse = clamp(p.pulse + 0.0065, 0, 1)

	table.insert(p.trail, 1, { x = p.x, y = p.y, a = p.angle })
	if #p.trail > 18 then table.remove(p.trail) end
end

function GameDirector:UpdateShards()
	local g = self.g
	local p = g.player
	for i = #g.shards, 1, -1 do
		local s = g.shards[i]
		s.pulse = s.pulse + 0.05
		s.rot = s.rot + 2.2
		s.x = s.base_x + math.cos(g.frame * 0.017 + s.seed) * 0.18
		s.y = s.base_y + math.sin(g.frame * 0.014 + s.seed) * 0.18
		if dist_sq(p.x, p.y, s.x, s.y) < 0.24 then
			self:CollectShard(i)
		end
	end
end

function GameDirector:UpdateBullets()
	local g = self.g
	for i = #g.bullets, 1, -1 do
		local b = g.bullets[i]
		b.x = b.x + b.vx
		b.y = b.y + b.vy
		b.life = b.life - 1

		local removed = false
		for j = #g.enemies, 1, -1 do
			local e = g.enemies[j]
			local radius = e.kind == "brute" and 0.52 or 0.37
			if dist_sq(b.x, b.y, e.x, e.y) < radius * radius then
				e.hp = e.hp - 1
				e.hit_flash = 12
				self:AddSpark(b.x, b.y, 18, 255, 95, 145, 0.14)
				table.remove(g.bullets, i)
				removed = true
				if e.hp <= 0 then
					g.score = g.score + (e.kind == "brute" and 350 or 160) * g.combo
					self:AddSpark(e.x, e.y, 34, 255, 97, 132, 0.18)
					Audio.Play(2, "pop", false)
					table.remove(g.enemies, j)
				end
				break
			end
		end

		if not removed then
			for _, s in ipairs(g.sentinels) do
				if s.alive then
					local sx = math.cos(s.angle) * s.radius
					local sy = math.sin(s.angle) * s.radius
					if dist_sq(b.x, b.y, sx, sy) < 0.26 then
						s.hp = s.hp - 1
						s.hit_flash = 16
						self:AddSpark(sx, sy, 20, 255, 206, 105, 0.14)
						table.remove(g.bullets, i)
						removed = true
						if s.hp <= 0 then
							s.alive = false
							g.score = g.score + 900
							g.shake = math.max(g.shake, 24)
							self:AddSpark(sx, sy, 70, 255, 186, 76, 0.22)
							Audio.Play(3, "warp", false)
						end
						break
					end
				end
			end
		end

		if not removed and (b.life <= 0 or dist(0, 0, b.x, b.y) > g.arena_radius + 2.4) then
			table.remove(g.bullets, i)
		end
	end
end

function GameDirector:UpdateEnemies()
	local g = self.g
	local p = g.player

	g.spawn_timer = g.spawn_timer - 1
	if g.spawn_timer <= 0 then
		self:SpawnEnemy()
		local pace = 112 - g.collected * 6 - math.floor(g.frame / 680)
		g.spawn_timer = math.max(32, pace)
	end

	for i = #g.enemies, 1, -1 do
		local e = g.enemies[i]
		e.phase = e.phase + 0.05
		e.hit_flash = math.max(0, e.hit_flash - 1)

		local tx = p.x
		local ty = p.y
		if e.kind == "wraith" then
			tx = tx + math.cos(e.phase * 1.9) * 1.2
			ty = ty + math.sin(e.phase * 1.4) * 1.2
		end
		local nx, ny = normalize(tx - e.x, ty - e.y)
		local accel = e.kind == "brute" and 0.0022 or 0.0036
		if e.kind == "wraith" then accel = 0.0045 end
		e.vx = (e.vx + nx * accel) * 0.992
		e.vy = (e.vy + ny * accel) * 0.992
		local vx, vy, sp = normalize(e.vx, e.vy)
		local max_sp = e.kind == "brute" and 0.055 or 0.075
		if e.kind == "wraith" then max_sp = 0.093 end
		if sp > max_sp then
			e.vx = vx * max_sp
			e.vy = vy * max_sp
		end

		e.x = e.x + e.vx
		e.y = e.y + e.vy
		local hit_radius = e.kind == "brute" and 0.58 or 0.42
		if dist_sq(p.x, p.y, e.x, e.y) < hit_radius * hit_radius then
			local hx, hy = normalize(p.x - e.x, p.y - e.y)
			self:DamagePlayer(e.kind == "brute" and 2 or 1, hx, hy)
			table.remove(g.enemies, i)
		elseif dist(0, 0, e.x, e.y) > g.arena_radius + 5 then
			table.remove(g.enemies, i)
		end
	end
end

function GameDirector:UpdateSentinels()
	local g = self.g
	local p = g.player
	for _, s in ipairs(g.sentinels) do
		if s.alive then
			s.angle = s.angle + s.spin
			s.beam = s.beam + s.beam_spin
			s.hit_flash = math.max(0, s.hit_flash - 1)

			local x = math.cos(s.angle) * s.radius
			local y = math.sin(s.angle) * s.radius
			local bx = math.cos(s.beam)
			local by = math.sin(s.beam)
			local dseg = dist_to_segment(p.x, p.y, x, y, x + bx * 7.6, y + by * 7.6)
			if dseg < 0.12 and p.invuln == 0 then
				self:DamagePlayer(1, -bx, -by)
			end
		end
	end
end

function GameDirector:UpdateSparks()
	local g = self.g
	for i = #g.sparks, 1, -1 do
		local s = g.sparks[i]
		s.x = s.x + s.vx
		s.y = s.y + s.vy
		s.vx = s.vx * 0.965
		s.vy = s.vy * 0.965
		s.life = s.life - 1
		if s.life <= 0 then
			table.remove(g.sparks, i)
		end
	end
end

function GameDirector:UpdateObjective()
	local g = self.g
	local p = g.player
	if g.rift_open and g.mode == "play" then
		if dist_sq(p.x, p.y, 0, 0) < 0.72 then
			g.mode = "won"
			g.win_frame = g.frame
			g.score = g.score + 5000 + g.best_combo * 400
			g.shake = 42
			set_message(g, "SIGNAL BLOOM", 9999)
			Audio.Play(3, "win", false)
			self:AddSpark(0, 0, 160, 130, 245, 255, 0.34)
		end
	end
end

function GameDirector:OnUpdate()
	local g = self.g
	if Input.GetKeyDown("escape") then
		Application.Quit()
	end
	if Input.GetKeyDown("r") then
		self:Reset()
		return
	end

	g.frame = g.frame + 1
	g.rift_spin = g.rift_spin + (g.rift_open and 1.8 or 0.65)
	g.message_timer = math.max(0, g.message_timer - 1)
	if g.combo_timer > 0 then
		g.combo_timer = g.combo_timer - 1
		if g.combo_timer == 0 then g.combo = 1 end
	end

	if g.mode == "play" then
		self:UpdatePlayer()
		self:UpdateShards()
		self:UpdateBullets()
		self:UpdateEnemies()
		self:UpdateSentinels()
		self:UpdateObjective()
	elseif g.mode == "won" then
		local p = g.player
		p.vx = p.vx * 0.94
		p.vy = p.vy * 0.94
		p.x = approach(p.x, 0, 0.035)
		p.y = approach(p.y, 0, 0.035)
		p.angle = p.angle + 0.05
		if g.frame % 5 == 0 then self:AddSpark(0, 0, 9, 120, 245, 255, 0.24) end
	else
		g.player.vx = g.player.vx * 0.96
		g.player.vy = g.player.vy * 0.96
	end

	self:UpdateSparks()

	g.shake = math.max(0, g.shake - 1)
	local shake = g.shake * 0.006
	g.cam_x = g.player.x + math.sin(g.frame * 1.7) * shake
	g.cam_y = g.player.y + math.cos(g.frame * 1.3) * shake
	Camera.SetPosition(g.cam_x, g.cam_y)

	self:Render()
end

function GameDirector:RenderWorld()
	local g = self.g
	local p = g.player

	Image.DrawEx("backdrop", g.cam_x, g.cam_y, 0, 1, 1, 0.5, 0.5, 255, 255, 255, 255, -1000)
	Image.DrawEx("backdrop", g.cam_x * 0.35, g.cam_y * 0.35, 180, 1, 1, 0.5, 0.5, 60, 98, 145, 74, -999)

	for i = 1, 5 do
		local scale = 1.5 + i * 1.45
		local alpha = 24 + i * 10
		Image.DrawEx("ring", 0, 0, -g.rift_spin * (0.35 + i * 0.06), scale, scale, 0.5, 0.5, 48, 121, 181, alpha, -120 + i)
	end

	local core_r = g.rift_open and 114 or 74
	local core_g = g.rift_open and 245 or 168
	local core_b = g.rift_open and 255 or 255
	local core_scale = g.rift_open and (1.25 + math.sin(g.frame * 0.09) * 0.09) or 1.04
	Image.DrawEx("glow", 0, 0, 0, 3.8, 3.8, 0.5, 0.5, core_r, core_g, core_b, g.rift_open and 106 or 88, -80)
	Image.DrawEx("core", 0, 0, g.rift_spin, core_scale, core_scale, 0.5, 0.5, core_r, core_g, core_b, 255, 4)

	for _, s in ipairs(g.shards) do
		draw_beam("beam", 0, 0, s.x, s.y, 0.13, 66, 230, 212, 45, -40)
		local pulse_scale = 0.58 + math.sin(s.pulse) * 0.07
		Image.DrawEx("glow", s.x, s.y, 0, 1.05, 1.05, 0.5, 0.5, 70, 255, 204, 74, 0)
		Image.DrawEx("shard", s.x, s.y, s.rot, pulse_scale, pulse_scale, 0.5, 0.5, 115, 255, 210, 255, 18)
	end

	for _, s in ipairs(g.sentinels) do
		local x = math.cos(s.angle) * s.radius
		local y = math.sin(s.angle) * s.radius
		if s.alive then
			local bx = math.cos(s.beam)
			local by = math.sin(s.beam)
			local flash = s.hit_flash > 0 and 255 or 168
			draw_beam("beam", x, y, x + bx * 7.6, y + by * 7.6, 0.18, 255, 70, 118, 118, 1)
			Image.DrawEx("glow", x, y, 0, 1.25, 1.25, 0.5, 0.5, 255, 78, 118, 92, 8)
			Image.DrawEx("sentinel", x, y, g.frame * 2.1, 0.78, 0.78, 0.5, 0.5, flash, 90, 120, 255, 20)
		else
			Image.DrawEx("ring", x, y, -g.frame, 0.45, 0.45, 0.5, 0.5, 255, 190, 90, 42, -20)
		end
	end

	for _, b in ipairs(g.bullets) do
		Image.DrawEx("glow", b.x, b.y, 0, 0.5, 0.5, 0.5, 0.5, 91, 224, 255, 78, 34)
		Image.DrawEx("projectile", b.x, b.y, b.rot, 0.78, 0.78, 0.5, 0.5, 155, 244, 255, 255, 36)
	end

	for _, e in ipairs(g.enemies) do
		local rot = math.atan(e.vy, e.vx) * DEG + 90
		local scale = e.kind == "brute" and 1.06 or 0.82
		local r = e.hit_flash > 0 and 255 or (e.kind == "wraith" and 185 or 255)
		local gg = e.hit_flash > 0 and 235 or (e.kind == "wraith" and 78 or 72)
		local b = e.hit_flash > 0 and 235 or (e.kind == "wraith" and 255 or 116)
		Image.DrawEx("glow", e.x, e.y, 0, scale * 1.2, scale * 1.2, 0.5, 0.5, r, gg, b, 78, 22)
		Image.DrawEx("drone", e.x, e.y, rot + math.sin(e.phase) * 12, scale, scale, 0.5, 0.5, r, gg, b, 255, 32)
	end

	for i = #p.trail, 1, -1 do
		local t = p.trail[i]
		local alpha = 90 * (1 - i / #p.trail)
		if alpha > 0 then
			Image.DrawEx("ship", t.x, t.y, t.a * DEG + 90, 0.72, 0.72, 0.5, 0.5, 65, 205, 255, alpha, 38)
		end
	end

	if p.invuln % 8 < 4 then
		Image.DrawEx("glow", p.x, p.y, 0, 1.1 + p.energy * 0.18, 1.1 + p.energy * 0.18, 0.5, 0.5, 71, 220, 255, 74, 42)
		Image.DrawEx("ship", p.x, p.y, p.angle * DEG + 90, 0.86, 0.86, 0.5, 0.5, 170, 244, 255, 255, 50)
	end

	for _, sp in ipairs(g.sparks) do
		local alpha = clamp((sp.life / sp.max) * 255, 0, 255)
		Image.DrawEx("glow", sp.x, sp.y, 0, sp.size, sp.size, 0.5, 0.5, sp.r, sp.g, sp.b, alpha, 70)
	end
end

function GameDirector:RenderHud()
	local g = self.g
	local p = g.player
	local mx = Input.GetMousePosition().x
	local my = Input.GetMousePosition().y
	if mx == 0 and my == 0 then
		mx = SCREEN_W * 0.5 + math.cos(p.angle) * 170
		my = SCREEN_H * 0.5 + math.sin(p.angle) * 170
	end
	Image.DrawUIEx("reticle", mx - 24, my - 24, 115, 245, 255, 210, 500)

	Text.Draw("RIFTBOUND", 28, 20, "ui", 34, 128, 237, 255, 255)
	Text.Draw("SCORE " .. tostring(g.score), 30, 62, "ui", 22, 232, 244, 255, 255)
	Text.Draw("SHARDS " .. tostring(math.min(g.collected, g.needed)) .. "/" .. tostring(g.needed), 30, 90, "ui", 22, 116, 255, 204, 255)

	local integrity = ""
	for i = 1, p.max_shield do
		integrity = integrity .. (i <= p.shield and "|" or ".")
	end
	Text.Draw("INTEGRITY " .. integrity, 30, 118, "ui", 22, 255, 107, 137, 255)

	local energy_width = math.floor(p.energy * 18)
	local dash_width = math.floor(p.dash * 18)
	local pulse_width = math.floor(p.pulse * 18)
	Text.Draw("LANCE [" .. string.rep("=", energy_width) .. string.rep(" ", 18 - energy_width) .. "]", 30, 146, "ui", 18, 122, 225, 255, 255)
	Text.Draw("DASH  [" .. string.rep("=", dash_width) .. string.rep(" ", 18 - dash_width) .. "]", 30, 171, "ui", 18, 176, 209, 255, 255)
	Text.Draw("BLOOM [" .. string.rep("=", pulse_width) .. string.rep(" ", 18 - pulse_width) .. "]", 30, 196, "ui", 18, 255, 214, 112, 255)

	if g.combo > 1 and g.combo_timer > 0 then
		Text.Draw("x" .. tostring(g.combo) .. " SIGNAL CHAIN", 1040, 26, "ui", 24, 255, 224, 118, 255)
	end

	if g.message_timer > 0 then
		local alpha = clamp(g.message_timer * 4, 0, 255)
		Text.Draw(g.message, 522, 628, "ui", 34, 214, 244, 255, alpha)
	end

	if g.rift_open and g.mode == "play" then
		Text.Draw("RIFT STABLE", 1052, 654, "ui", 26, 130, 255, 235, 255)
	end

	if g.mode == "over" then
		Text.Draw("SIGNAL LOST", 492, 300, "ui", 54, 255, 92, 126, 255)
		Text.Draw("FINAL SCORE " .. tostring(g.score), 514, 360, "ui", 28, 230, 238, 255, 255)
	elseif g.mode == "won" then
		Text.Draw("SIGNAL BLOOM", 472, 294, "ui", 56, 122, 249, 255, 255)
		Text.Draw("FINAL SCORE " .. tostring(g.score), 508, 356, "ui", 30, 240, 252, 255, 255)
	end
end

function GameDirector:Render()
	self:RenderWorld()
	self:RenderHud()
end
