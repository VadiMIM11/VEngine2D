GameOverManager = {

	cutscene_index = 0,
	desired_text = "",
	current_text = "",
	current_image_alpha = 0,
	current_image_to_draw = "",

	desired_vocal = "",
	played_vocal_index = -1,

	image_offset_x = 0,

	OnStart = function(self)
		-- Play some intro music
		--Audio.Play(0, "ThisGameIsOver_by_mccartneytm_ccby", false)
		game_over_pre_stage = false
	end,

	wait_timer = 180,
	OnUpdate = function(self)
		-- When the player clicks or presses spacebar, we advance the cutscene.
		if Input.GetMouseButtonDown(1) or Input.GetKeyDown("space") then
			
			if self.cutscene_index ~= 2 then
				self.cutscene_index = self.cutscene_index + 1

			else
				self.countdown_timer = 1
			end

			if self.cutscene_index == 2 and continues <= 0 then
				self.cutscene_index = 3
			end
		end

		-- Render the cutscene image
		local previous_image = self.current_image_to_draw

		if self.cutscene_index == 0 then 
			self.current_image_to_draw = "game_over_bad"
		end

		if self.cutscene_index == 1 then 
			self.current_image_to_draw = "intro4"
		end

		if self.cutscene_index == 2 then 
			self.current_image_to_draw = "continue1"
			--for i=0,continues do
			--	Image.DrawUIEx("hp_icon", 700 + i * 96, 300, 255, 255, 255, 255, 9)
			--end

			if self.countdown_number > 0 then
				self.image_offset_x = -150
				Text.Draw(self.countdown_number .. "", 700, 120, "NotoSans-Regular", 120, 255, 255, 255, 255)
			end
			self:DoContinueSequence()
		end

		if self.cutscene_index == 3 then 
			self.image_offset_x = 0
			self.current_image_to_draw = "gameover"
			self.desired_vocal = "gameover"
		end

		if self.cutscene_index == 4 then
			game_over = false
			next_stage = 1
			lives = 2
			continues = 2
			activated_dialogues = {}
			SaveGame(next_stage, lives, continues)
			local transition_component = Actor.Find("SceneTransitionManager"):GetComponent("SceneTransitionManager")
			transition_component:RequestChangeScene("intro")
		end

		-- If new image, do the white flash effect as a transition.
		if previous_image ~= self.current_image_to_draw then
			self.current_image_alpha = 0
		end

		-- Fade in new image
		if self.current_image_alpha < 255 then
			self.current_image_alpha = self.current_image_alpha + 5
			if self.current_image_alpha > 255 then
				self.current_image_alpha = 255
			end
		end

		-- Build the current string one character at a time.
		if Application.GetFrame() % 5 and self.current_text ~= self.desired_text then
			local current_text_length = string.len(self.current_text)
			self.current_text = self.current_text .. string.sub(self.desired_text, current_text_length+1, current_text_length+1)
		end

		-- Render
		Image.DrawUIEx(self.current_image_to_draw, self.image_offset_x, 0, 255, 255, 255, self.current_image_alpha, 0)
		--Text.Draw(self.current_text, 10, 320, "NotoSans-Regular", 22, 255, 255, 255, 255)

		if self.cutscene_index ~= self.played_vocal_index and self.desired_vocal ~= "" then
			Audio.Play(1, self.desired_vocal, false)
			self.played_vocal_index = self.cutscene_index			
		end
		self.desired_vocal = ""
	end,

	countdown_timer = 120,
	countdown_number = 9,
	said_initial_countdown_vocal = false,
	DoContinueSequence = function(self)
		self.countdown_timer = self.countdown_timer - 1

		if self.said_initial_countdown_vocal == false then
			Audio.Play(1, "9", false)
			self.said_initial_countdown_vocal = true
		end

		if self.countdown_timer <= 0 then
			self.countdown_timer = 120
			self.countdown_number = self.countdown_number - 1
			if self.countdown_number > 0 then
				Audio.Play(1, "" .. self.countdown_number, false)
			end
		end

		if self.countdown_number < 1 then 
			self.countdown_timer = self.countdown_timer - 2
			self.image_offset_x = self.image_offset_x + (0 - self.image_offset_x) * 0.1
		end

		if self.countdown_number == 0 then
			Image.DrawUIEx("continue2", self.image_offset_x, 0, 255, 255, 255, self.current_image_alpha, 1)
		end
		if self.countdown_number <= -1 then
			Image.DrawUIEx("continue3", self.image_offset_x, 0, 255, 255, 255, self.current_image_alpha, 1)
		end

		if self.countdown_number == -3 then
			self.cutscene_index = 3
		end

		-- Continue
		if Input.GetKeyDown("enter") then
			continues = continues - 1
			lives = 2
			activated_dialogues = {}
			game_over = false
			local transition_component = Actor.Find("SceneTransitionManager"):GetComponent("SceneTransitionManager")
			transition_component:RequestChangeScene("prestage")
		end
	end
}

