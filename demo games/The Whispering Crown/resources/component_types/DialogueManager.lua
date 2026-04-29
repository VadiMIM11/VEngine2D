DialogueManager = {

	current_image = "",

	current_text = "",
	desired_text = "",

	slide_x = 0,
	dialogue_index = 0,
	dialogue_set_name = "",
	frames_until_skip_possible = 60,

	LaunchDialogue = function(self, dialogue_set_name)
		dialogue_running = true
		self.current_image = ""
		self.current_text = ""
		self.desired_text = ""
		self.dialogue_set_name = dialogue_set_name
		self.dialogue_index = 0
		self.slide_x = 0
		self:BeginNextLine()
	end,

	OnStart = function(self)

		dialogue_running = false

		self.data = {
			crown_stage_1 = {"(the crown shines with ancient aura)", "(It's the crown jewel of the museum)" },
			
			cuff_stage_1 = {"{switch}cuff_dialogue1", "Oh!", "Hey Spot.", "Guess what I saw on the CCTV?", "Some folks snuck in after hours.", "Let's go check em out." },
			assassin_stage_1 = {"{switch}assassin_dialogue", "Crown... crown... crown...", "It must be somewhere...", "Oh!", "I'll bet you know.", "And now you're gonna tell me."},
			intern_stage_1 = {"{switch}intern_dialogue", "Hey Spot!", "Hey Cuff!", "There's a woman down the hall.", "She looks ready for action.", "IIIIII... would keep some distance."},

			janitor_stage_2 = {"{switch}janitor_dialogue", "Heyyyy!", "Spot... my dude...", "We got some proooblems here.", "Ya gotta watch carefully, mannnn.", "Use your SCROLL WHEEL (mouse) to zoooom!"},
			ninja_stage_2 = {"{switch}ninja_dialogue", "...?", "(the ninja bows to you)", "(then, he strikes a combat pose)"},
			assassin_stage_2 = {"{switch}assassin_dialogue", "Oh darling, you can't be serious...", "YOU protect the crown?", "Don't make me laugh."},
			infiltrator_stage_2 = {"{switch}infiltrator_dialogue", "Ah!", "Eh???", "Why are you so small?", "Who hired you?" },
			cuff_stage_2 = {"{switch}cuff_dialogue1", "Shhhhhhhh!", "Spot, be quiet", "There's a DUDE up ahead.", "Don't even worry.", "I'ma take him down!", "Have some faith in your boyfriend!" },
			intern_stage_2 = {"{switch}intern_dialogue", "Wooow!", "Things are getting so crazy!", "Spot, Cuff... please be careful!", "You only have so many lives..", "And if you run out...?", "GAME OVER for the cute couple!", "Earn more lives by exploring and being smart!" },

			manager_stage_3 = {"{switch}manager_dialogue", "Spot! There you are.", "The situation is beyond our control.", "I need you to sneak past these guys...", "Get outside where reception is good.", "and call the cops.", "I'll try and distract them.", "Use these binoculars to help you plan."},
			villy_stage_3 = {"{switch}villy_dialogue2", "Oh ho ho!", "{switch}villy_dialogue", "Now what do we have here?", "{switch}villy_dialogue", "Boy, we got ourselves a ginger!", "{switch}villy_dialogue3", "and a cute one at that...", "{switch}villy_dialogue2", "Alright, girly!", "{switch}villy_dialogue3", "Game on!"},
			assassin_stage_3 = {"{switch}assassin_dialogue", "Impeccable timing, dear.", "You got away last time.", "This time, I'll hand you to Villianne myself!"},
			infiltrator_stage_3 = {"{switch}infiltrator_dialogue", "Come on, now.", "Be a doll, and come with me.", "Miss Villy is looking for you."},
			janitor_stage_3 = {"{switch}janitor_dialogue", "Whoa whoa whoa!", "Bro!", "Don't lead her over here!", "I'm hiding!"}
		}
		self.current_image = ""

	end,

	OnUpdate = function(self)
		
		if dialogue_running ~= true then
			return
		end

		local finished = #self.current_text == #self.desired_text

		-- When the player clicks or presses spacebar, we advance the cutscene.
		if Input.GetMouseButtonDown(1) or Input.GetKeyDown("space") then
			if finished then
				self:BeginNextLine()
			else
				while(#self.current_text < #self.desired_text) do
					local next_index = #self.current_text + 1
					self.current_text = self.current_text .. self.desired_text:sub(next_index, next_index)
				end
			end
		end

		local talking = #self.current_text < #self.desired_text

		-- Build text
		if Application.GetFrame() % 2 == 0 and (self.current_image == "" or self.slide_x > 511) then
			if talking then
				local next_index = #self.current_text + 1
				self.current_text = self.current_text .. self.desired_text:sub(next_index, next_index)
			end
		end


		-- Render
		local talking_bounce_offset = 0
		if talking and self.slide_x > 511 then
			talking_bounce_offset = -math.abs(math.sin(Application.GetFrame() * 0.2)) * 20
		end

		Image.DrawUIEx("loading", 0, 0, 255, 255, 255, 150, 9999)

		if self.current_image ~= "" then
			self.slide_x = self.slide_x + (512 - self.slide_x) * 0.1
			Image.DrawUIEx(self.current_image, 1152 - self.slide_x, talking_bounce_offset + 20, 255, 255, 255, 255, 9999)
		end

		Image.DrawUIEx("hud_bottom_shell", 0, 0, 255, 255, 255, 255, 9999)
		Text.Draw(self.current_text, 20, 430, "NotoSans-Regular", 50, 255, 255, 255, 255)
	end,

	BeginNextLine = function(self)
		self.dialogue_index = self.dialogue_index + 1
		
		if self.dialogue_index > #self.data[self.dialogue_set_name] then
			dialogue_running = false
			return
		end

		self.current_text = ""
		self.desired_text = self.data[self.dialogue_set_name][self.dialogue_index]

		-- Check for image switch
		local image_name = self.desired_text:match("^%{switch%}(.+)$")
		if image_name ~= nil then
			self.current_image = image_name
			self.dialogue_index = self.dialogue_index + 1
			self.desired_text = self.data[self.dialogue_set_name][self.dialogue_index]
		end

		self.desired_text = "\"" .. self.desired_text .. "\""

		
	end
}

