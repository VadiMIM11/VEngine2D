GameManager = {
	num_enemies = 10,
	base_enemy_movement_speed = -1.5,

	OnStart = function(self)

		table_count = function (t)
			local count = 0
			for _ in pairs(t) do
				count = count + 1
			end
			return count
		end

		-- Establish global function
		ProcessTurn = function(self)

			if gameplay_enabled then

				movements = movements + 1

				-- Sort the actors by priority
				table.sort(turn_based_actors, function(a, b)

					local turn_based_a = a:GetComponent("TurnBased")
					local turn_based_b = b:GetComponent("TurnBased")

					return turn_based_a.priority < turn_based_b.priority
				end)

				-- Iterate through all registered actors that have a TurnBasedMovement component on them.
				for i,turn_based_actor in ipairs(turn_based_actors) do

					-- MOVEMENT
					local basic_ai_movement = turn_based_actor:GetComponent("BasicAIMovement")
					if basic_ai_movement ~= nil then
						basic_ai_movement:OnTurn()
					end

					local follow_ai_movement = turn_based_actor:GetComponent("FollowAIMovement")
					if follow_ai_movement ~= nil then
						follow_ai_movement:OnTurn()
					end

					local cuff_movement = turn_based_actor:GetComponent("Cuff")
					if cuff_movement ~= nil then
						cuff_movement:OnTurn()
					end

					-- DAMAGE
					local kill_on_touch = turn_based_actor:GetComponent("KillOnTouch")
					if kill_on_touch ~= nil then
						kill_on_touch:OnTurn()
					end

					-- VISION
					local cone_vision = turn_based_actor:GetComponent("ConeVision")
					if cone_vision ~= nil then
						cone_vision:OnTurn()
					end

					-- AT END of turn for an actor, please update blocking
					local blocking = turn_based_actor:GetComponent("Block")
					if blocking ~= nil then
						blocking:OnTurn()
					end
					
				end
			end

		end

		math.randomseed(404)
		self:CreateStage2(next_stage)
		gameplay_enabled = true
	end,

	CreateStage2 = function(self, stage_number)

		fog_map = {}
		fog_cleared_requesters = {}
		blocking_map = {}
		tile_map = {}
		score = 0
		game_over = false
		movements = 0
		secrets_collected = 0
		secrets_available = 0

		-- Load Data from file
		local map_file_path = "resources/data/map" .. stage_number .. ".lua"
		local map = dofile(map_file_path)

		for _, layer in ipairs(map.layers) do
			if layer.type == "objectgroup" then
				for _, obj in ipairs(layer.objects) do
					local x = obj.x or 0
					local y = obj.y or 0

					Debug.Log()

					local gid = obj.gid - 1 -- Lua is one-indexed, so we need to subtract 1 to match up with Tiled's ID system

					-- Normalize X and Y positions
					x = x / 100
					y = y / 100

					-- Add fog to this part of the map
					fog_map[Round(x) .. "," .. Round(y)] = true

					local new_actor = nil

					if gid == 0 then -- janitor
						new_actor = Actor.Instantiate("NPC")
						local new_npc_t = new_actor:GetComponent("Transform")
						new_actor:GetComponent("SpriteRenderer").sprite = "janitor"
					end

					if gid == 15 then -- floor_tile
						new_actor = Actor.Instantiate("Tile")
						local new_tile_t = new_actor:GetComponent("Transform")
						new_actor:GetComponent("SpriteRenderer").sprite = "tile"
					end

					if gid == 4 then -- Villy 
						new_actor = Actor.Instantiate("Villy")
						local new_enemy_t = new_actor:GetComponent("Transform")
					end

					if gid == 5 then -- block
						new_actor = Actor.Instantiate("Block")
						local new_box_t = new_actor:GetComponent("Transform")
					end

					if gid == 2 then -- player
						new_actor = Actor.Instantiate("Player")
						local new_player_t = new_actor:GetComponent("Transform")
					end
					
					if gid == 6 then -- manager
						new_actor = Actor.Instantiate("NPC")
						local new_npc_t = new_actor:GetComponent("Transform")
						new_actor:GetComponent("SpriteRenderer").sprite = "boss"
					end

					if gid == 8 then -- cuff
						new_actor = Actor.Instantiate("Cuff")
						local new_npc_t = new_actor:GetComponent("Transform")
					end
					
					if gid == 7 then -- crown
						new_actor = Actor.Instantiate("NPC")
						local new_npc_t = new_actor:GetComponent("Transform")
						new_actor:GetComponent("SpriteRenderer").sprite = "crown"
					end

					if gid == 9 then -- doors 
						new_actor = Actor.Instantiate("Door")
						local new_door_t = new_actor:GetComponent("Transform")

						new_actor:GetComponent("AdvanceStageOnTouch").stage = obj.properties.stage
					end

					if gid == 10 then -- ninja 
						new_actor = Actor.Instantiate("Enemy")
						local new_enemy_t = new_actor:GetComponent("Transform")
						new_actor:GetComponent("SpriteRenderer").sprite = "enemy1"

						local movement = new_actor:GetComponent("BasicAIMovement")
						movement.vel_x = obj.properties.vx
						movement.vel_y = obj.properties.vy
						movement.turns_between_actions = obj.properties.turns_between_actions
					end

					if gid == 11 then -- infiltrator dude
						new_actor = Actor.Instantiate("Enemy")
						local new_enemy_t = new_actor:GetComponent("Transform")
						new_actor:GetComponent("SpriteRenderer").sprite = "enemy2"

						local movement = new_actor:GetComponent("BasicAIMovement")
						movement.vel_x = obj.properties.vx
						movement.vel_y = obj.properties.vy
						movement.turns_between_actions = obj.properties.turns_between_actions
					end

					if gid == 12 then -- burglar
						new_actor = Actor.Instantiate("Enemy")
						local new_enemy_t = new_actor:GetComponent("Transform")
						new_actor:GetComponent("SpriteRenderer").sprite = "enemy3"

						local movement = new_actor:GetComponent("BasicAIMovement")
						movement.vel_x = obj.properties.vx
						movement.vel_y = obj.properties.vy
						movement.turns_between_actions = obj.properties.turns_between_actions
					end

					if gid == 13 then -- spy lady
						new_actor = Actor.Instantiate("Enemy")
						local new_enemy_t = new_actor:GetComponent("Transform")
						new_actor:GetComponent("SpriteRenderer").sprite = "enemy4"

						local movement = new_actor:GetComponent("BasicAIMovement")
						movement.vel_x = obj.properties.vx
						movement.vel_y = obj.properties.vy
						movement.turns_between_actions = obj.properties.turns_between_actions
					end

					if gid == 16 then -- Secret
						new_actor = Actor.Instantiate("Secret")
						local new_item_t = new_actor:GetComponent("Transform")
					end

					if gid == 17 then -- binoculars
						new_actor = Actor.Instantiate("Binoculars")
						local new_item_t = new_actor:GetComponent("Transform")
					end

					if gid == 18 then -- intern
						new_actor = Actor.Instantiate("NPC")
						local new_npc_t = new_actor:GetComponent("Transform")
						new_actor:GetComponent("SpriteRenderer").sprite = "intern"
					end



					-- General setup
					if new_actor ~= nil then
						-- POSITION
						local new_actor_t = new_actor:GetComponent("Transform")
						new_actor_t.x = x
						new_actor_t.y = y

						-- DIALOGUE
						if obj.properties.dialogue ~= nil and obj.properties.dialogue ~= "" then
							local launch_dialogue_when_near = new_actor:AddComponent("LaunchDialogueWhenNear")
							launch_dialogue_when_near.dialogue_set_name = obj.properties.dialogue
						end
					end
				end
			end
		end
	end
}

