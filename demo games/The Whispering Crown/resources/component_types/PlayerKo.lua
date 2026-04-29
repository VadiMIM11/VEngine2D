PlayerKo = {

	vel_y = -0.04,
	should_show_gameover_text = false,

	OnStart = function(self)
		self.spriterenderer = self.actor:GetComponent("SpriteRenderer")
		self.transform = self.actor:GetComponent("Transform")

		self.spriterenderer.sprite = "player_surprise"
		self.spriterenderer.sorting_order = 9999

		Camera.SetPosition(self.transform.x, self.transform.y - 1)
		Camera.SetZoom(1.0)
		if lives <= 0 then
			self.should_show_gameover_text = true
		end
	end,

	game_over_timer = 0.0,
	game_x = -200,
	game_vel = 0,
	over_x = 1152,
	over_vel = 0,
	gameover_bg_alpha = 0,
	red_isolate_val = 255,

	OnUpdate = function(self) 

		self.vel_y = self.vel_y + 0.002
		self.transform.y = self.transform.y + self.vel_y

		if self.vel_y < 0 then
			self.spriterenderer.sprite = "player_surprise"
			
		else
			self.spriterenderer.sprite = "player_ko"
		end


		-- Fallen off screen
		if self.should_show_gameover_text == false then
			if self.vel_y >= 0.3 then
				self:Exit()
			end
		else
			if self.vel_y >= 0.6 then
				self:Exit()
			end

			self.game_vel = self.game_vel + 0.2
			self.over_vel = self.over_vel - 0.2
			self.game_x = self.game_x + self.game_vel
			self.over_x = self.over_x + self.over_vel

			if self.game_x > 350 then
				self.game_x = 350
				self.game_vel = self.game_vel * -0.5
			end

			if self.over_x < 600 then
				self.over_x = 600
				self.over_vel = self.over_vel * -0.5
			end

			self.gameover_bg_alpha = self.gameover_bg_alpha + 1.2
			if self.gameover_bg_alpha > 255 then
				self.gameover_bg_alpha = 255

				self.red_isolate_val = self.red_isolate_val - 5
				if self.red_isolate_val < 0 then
					self.red_isolate_val = 0
				end
			end
			Image.DrawUIEx("loading", 0, 0, 0, 0, 0, self.gameover_bg_alpha, 0)

			

			Image.DrawUIEx("game_text", self.game_x, 175, 255, self.red_isolate_val, self.red_isolate_val, 255, 1)
			Image.DrawUIEx("over_text", self.over_x, 175, 255, self.red_isolate_val, self.red_isolate_val, 255, 1)

			--Image.DrawUI("game_text", self.game_x, 175)
			--Image.DrawUI("over_text", self.over_x, 175)
		end


	end,

	did_exit = false,
	Exit = function(self)

		if self.did_exit == false then
			self.did_exit = true

			lives = lives - 1

			local transition_component = Actor.Find("SceneTransitionManager"):GetComponent("SceneTransitionManager")

			if lives < 0 then
				Audio.Halt(0)
				game_over_pre_stage = true
				
			else
				fast_pre_stage = true
				
			end

			transition_component:RequestChangeScene("prestage")
			--transition_component:RequestChangeScene("dialogue")
			
		end
	end
}

