IntroManager = {

	cutscene_index = 0,
	desired_text = "",
	current_text = "",
	current_image_alpha = 0,
	current_image_to_draw = "",

	desired_vocal = "",
	played_vocal_index = -1,

	OnStart = function(self)

		Round = function(v)
			return math.floor(v+0.5)
		end

		SaveGame = function(stage, lives, continues)
			
			--local file, err = io.open("save.lua", "w")
			--if not file then
			--	print("Save failed:", err)
			--	return
			--end

			--file:write("return {\n")
			--file:write("  stage = ", stage, ",\n")
			--file:write("  lives = ", lives, ",\n")
			--file:write("  continues = ", continues, "\n")
			--file:write("}\n")

			--file:close()
		end

		gameplay_enabled = false
		continues = 2
		lives = 2
		next_stage = 1
		current_stage = 1
		activated_dialogues = {}

		-- Load game
		local ok, data = pcall(dofile, "save.lua")
		if ok and type(data) == "table" then
			next_stage = data.stage

			if next_stage ~= 1 then
				continues = data.continues
				lives = data.lives				
			end
		end

		if next_stage ~= 1 then
			local transition_component = Actor.Find("SceneTransitionManager"):GetComponent("SceneTransitionManager")
			transition_component:RequestChangeScene("prestage")
		else
			-- Play some intro music
			Audio.Play(0, "sky_bgm", true)
		end
	end,

	OnUpdate = function(self)
		if next_stage ~= 1 then
			return
		end

		-- When the player clicks or presses spacebar, we advance the cutscene.
		if Input.GetMouseButtonDown(1) or Input.GetKeyDown("space") then
			self.cutscene_index = self.cutscene_index + 1
			self.current_text = ""
		end

		-- Render the cutscene image
		local previous_image = self.current_image_to_draw

		if self.cutscene_index == 0 then 
			self.current_image_to_draw = "crown_intro"
		end

		if self.cutscene_index == 1 then 
			self.current_image_to_draw = "intro1"
		end

		if self.cutscene_index == 2 then 
			self.current_image_to_draw = "intro2"
		end

		if self.cutscene_index == 3 then 
			self.current_image_to_draw = "intro3"
		end

		-- If the player advances the cutscene enough, we move to the gameplay scene.
		-- The SceneTransitionManager component of the SceneTransitionManager actor has some special functionality for this.
		if self.cutscene_index >= 4 then
			self.current_image_to_draw = "intro3"
			self.desired_text = ""
			local transition_component = Actor.Find("SceneTransitionManager"):GetComponent("SceneTransitionManager")
			transition_component:RequestChangeScene("prestage")
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
		Image.DrawUIEx(self.current_image_to_draw, 0, 0, 255, 255, 255, self.current_image_alpha, 0)
		Text.Draw(self.current_text, 10, 320, "NotoSans-Regular", 22, 255, 255, 255, 255)

		if self.cutscene_index ~= self.played_vocal_index and self.desired_vocal ~= "" then
			Audio.Play(1, self.desired_vocal, false)
			self.played_vocal_index = self.cutscene_index
		end
		self.desired_vocal = ""
	end
}

