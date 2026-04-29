PreStageManager = {

	distance_from_crown = 6,

	OnStart = function(self)

		SaveGame(next_stage, lives, continues)
		current_stage = next_stage
		turn_based_actors = {}

		Camera.SetPosition(0, 0)
		Camera.SetZoom(1.0)

		-- Fast pre-stage
		if fast_pre_stage == nil or fast_pre_stage == false then
			self.step = 0
			activated_dialogues = {}
		else
			fast_pre_stage = false
			self.step = 1
		end

		Audio.Halt(0)
	end,

	step = 0,

	OnUpdate = function(self)
		
		if self.step == 0 then
			self:DrawCutscene()
		elseif self.step == 1 then
			self:DrawInfo()
		elseif self.step == 2 then
			self:DrawInfoExit()
		end
		
	end,

	cutscene_advanced = false,
	wait_counter_1 = 60,
	DrawCutscene = function(self)
		Image.DrawEx("crown", 0, 2.5, 0, 1.0, 1.0, 0.5, 1, 255, 255, 255, 255, 1)

		local character_rot = 0
		local character_hop = 0

		if self.distance_from_crown > 2 then
			self.distance_from_crown = self.distance_from_crown - (1.0 / 60.0) * 2.0
			character_rot = math.sin(self.distance_from_crown * 5) * 5
			character_hop = math.abs(character_rot) * 0.01

		else
			-- MOVE ON
			self.wait_counter_1 = self.wait_counter_1 - 1
			if Input.GetMouseButtonDown(1) or Input.GetKeyDown("space") or self.wait_counter_1 <= 0 then
				if game_over_pre_stage then
					
					local transition_component = Actor.Find("SceneTransitionManager"):GetComponent("SceneTransitionManager")
					transition_component:RequestChangeScene("gameover")
				else
					self.cutscene_advanced = true
				end
				
			end
		end

				-- Game over pre stage
		if game_over_pre_stage == nil or game_over_pre_stage == false then
			-- Render Spot on left
			Image.DrawEx("player_big", -self.distance_from_crown, 2.5 - character_hop, character_rot, 1, 1, 0.5, 1, 255, 255, 255, 255, 1)
		end


		-- Render Villy on right
		Image.DrawEx("villy_big", self.distance_from_crown, 2.5 - character_hop, -character_rot, -1, 1, 0.5, 1, 255, 255, 255, 255, 1)

		-- Move camera
		if self.cutscene_advanced == true then

			local cam_y = Camera.GetPositionY()
			Camera.SetPosition(0, cam_y - 0.08)
			if cam_y < -3 then
				self.step = self.step + 1
			end
		end
	end,

	GetStageName = function(self, stage_number)
		if stage_number == 1 then
			return "Gallery of the Whispering Crown"
		end

		if stage_number == 2 then
			return "The Winding Exhibit"
		end

		if stage_number == 3 then
			return "Checker Hall"
		end

		return "???"
	end,

	stage_title_y = -100,
	lives_icon_y = 600,
	lives_text_y = 600,

	stage_title_dest_y = 150,
	lives_icon_dest_y = 250,
	lives_text_dest_y = 275,

	alpha = 255,
	wait_counter_2 = 180,

	DrawInfo = function(self)
		self.alpha = self.alpha + (255 - self.alpha) * 0.1

		self.lives_icon_y = self.lives_icon_y + (self.lives_icon_dest_y - self.lives_icon_y) * 0.1
		self.lives_text_y = self.lives_text_y + (self.lives_text_dest_y - self.lives_text_y) * 0.1
		self.stage_title_y = self.stage_title_y + (self.stage_title_dest_y - self.stage_title_y) * 0.1

		-- Render stage name text
		Text.Draw("" .. next_stage .. ". " .. self:GetStageName(next_stage), 310, self.stage_title_y, "NotoSans-Regular", 32, 255, 255, 255, self.alpha)

		-- Render Spot lives remaining
		Image.DrawUIEx("lives_icon", 450, self.lives_icon_y, 255, 255, 255, self.alpha, 1)
		Text.Draw(lives, 600, self.lives_text_y, "NotoSans-Regular", 60, 255, 255, 255, self.alpha)

		-- MOVE ON
		self.wait_counter_2 = self.wait_counter_2 - 1
		if Input.GetMouseButtonDown(1) or Input.GetKeyDown("space") or self.wait_counter_2 <= 0 then
			self.step = self.step + 1
		end

	end,

	wait_counter_3 = 30,
	DrawInfoExit = function(self)

		self.lives_icon_y = self.lives_icon_y + (600 - self.lives_icon_y) * 0.1
		self.lives_text_y = self.lives_text_y + (600 - self.lives_text_y) * 0.1
		self.stage_title_y = self.stage_title_y + (-100 - self.stage_title_y) * 0.1

		-- Render stage name text
		Text.Draw("" .. next_stage .. ". " .. self:GetStageName(next_stage), 310, self.stage_title_y, "NotoSans-Regular", 32, 255, 255, 255, self.alpha)

		-- Render Spot lives remaining
		Image.DrawUIEx("lives_icon", 450, self.lives_icon_y, 255, 255, 255, self.alpha, 1)
		Text.Draw(lives, 600, self.lives_text_y, "NotoSans-Regular", 60, 255, 255, 255, self.alpha)

		self.wait_counter_3 = self.wait_counter_3 - 1
		if self.wait_counter_3 <= 0 then
			local transition_component = Actor.Find("SceneTransitionManager"):GetComponent("SceneTransitionManager")
			transition_component:RequestChangeScene("gameplay")
		end
	end
}

