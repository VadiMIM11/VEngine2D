PostStageManager = {

	distance_from_crown = 6,

	OnStart = function(self)

		post_stage_config = {
			[1] = {
				movement_par = 101
			},
			[2] = {
				movement_par = 98
			},
			[3] = {
				movement_par = 140
			}
		}

		Camera.SetPosition(0, 0)
		Camera.SetZoom(1.0)
		Audio.Halt(0)
	end,

	step = 0,

	OnUpdate = function(self)
		
		self:DrawCutscene()
		
	end,

	cutscene_advanced = false,
	wait_counter_1 = 60,
	spot_sprite = "player_big",
	spot_jump_vel = 0,
	spot_jumped = false,
	spot_y = 0,

	secrets_y = -1000,
	secrets_yvel = 0,
	secrets_bounced = false,
	movements_y = -50,
	movements_yvel = 0,
	movements_bounced = false,

	cuff_y = -1700,
	cuff_yvel = 0,
	cuff_bounced = false,

	spot_x_scale = 1,

	movements_life_flash_timer = 60,
	secrets_life_flash_timer = 60,
	cuff_life_flash_timer = 60,

	should_spot_fall = false,
	spot_fall_y = 0,
	spot_fall_vel = 0.12,
	played_damage_vocal = false,

	step_3_timer = 270,

	did_start_leaving = false,
	option_mode = false,
	option_mode_selection = 0,
	leave_timer = 120,
	spot_option_sprite = "spot_cursor",
	spot_option_yvel = -0.08,
	spot_option_y = 0,
	spot_option_x = 0,
	spot_option_fall_yelled = false,
	option_menu_banner = "Mission Incomplete",
	option_retry_text = "Retry",
	option_proceed_text = "Next Stage",

	did_init_options_menu = false,

	did_succeed_movements = false,
	did_succeed_secrets = false,
	did_succeed_cuff = false,

	DrawCutscene = function(self)

		local movements_par_number = post_stage_config[current_stage].movement_par
		self.did_succeed_movements = movements <= movements_par_number
		self.did_succeed_secrets = secrets_collected >= secrets_available
		self.did_succeed_cuff = cuff_following
		local cuff_success_string = "is safe"
		if cuff_following == false then
			cuff_success_string = "is RIP"
		end

		if self.did_succeed_movements and self.did_succeed_secrets and self.did_succeed_cuff and self.did_init_options_menu == false then

			self.did_init_options_menu = true
			self.option_mode_selection = 1 -- The retry menu will default to proceeding
			self.option_menu_banner = "Mission Complete"
			self.option_proceed_text = "Next Stage"
		end

		local character_rot = 0

		if self.step == 0 then
			if self.distance_from_crown > 0 then
				self.distance_from_crown = self.distance_from_crown - (1.0 / 60.0) * 2.0
				character_rot = math.sin(self.distance_from_crown * 5) * 5
				self.spot_y = math.abs(character_rot) * 0.01
				self.spot_sprite = "player_big"
			else
				self.spot_sprite = "winpose_big"
				if self.spot_jumped == false then
					self.spot_jumped = true
					self.spot_jump_vel = 0.1
				end

				-- Jumping
				self.spot_jump_vel = self.spot_jump_vel - 0.004
				self.spot_y = self.spot_y + self.spot_jump_vel

				if self.spot_y < 0 then
					self.spot_y = 0
					self.step = 1
					self.spot_sprite = "player_big"
				end
			end
		end

		if self.step == 1 or self.step == 2 then

			-- Text movement
			self.secrets_yvel = self.secrets_yvel + 0.1
			self.movements_yvel = self.movements_yvel + 0.1
			self.cuff_yvel = self.cuff_yvel + 0.1

			if self.secrets_yvel > 9.5 then
				self.secrets_yvel = 9.5
			end
			if self.cuff_yvel > 9.5 then
				self.cuff_yvel = 9.5
			end

			self.secrets_y = self.secrets_y + self.secrets_yvel
			self.movements_y = self.movements_y + self.movements_yvel
			self.cuff_y = self.cuff_y + self.cuff_yvel

			-- Movement text bounce
			if self.movements_y > 400 then
				self.movements_y = 400
				self.movements_yvel = -self.movements_yvel * 0.25
				self.movements_bounced = true
				if self.did_succeed_movements == false then
					self.should_spot_fall = true
				end
			end

			-- Secrets text bounce
			if self.secrets_y > 400 then
				self.secrets_y = 400
				self.secrets_yvel = -self.secrets_yvel * 0.25
				self.secrets_bounced = true
				if self.did_succeed_secrets == false then
					self.should_spot_fall = true
				end
			end

			-- Cuff text bounce
			if self.cuff_y > 150 then
				self.cuff_y = 150
				self.cuff_yvel = -self.cuff_yvel * 0.25
				self.cuff_bounced = true
				self.step = 2
				if self.did_succeed_cuff == false then
					self.should_spot_fall = true
				end
			end
		end

		if self.step == 2 then

			if self.should_spot_fall == false then
				self.spot_sprite = "winpose_big"
			end

			self.step_3_timer = self.step_3_timer - 1
			if self.step_3_timer <= 0 then
				
				if self.did_start_leaving == false then
					-- Display retry menu if the player did not complete all objectives.
					self.option_mode = true
					Text.Draw(self.option_menu_banner, 400, 50, "NotoSans-Regular", 40, 255, 255, 255, 255)

					-- text options
					Text.Draw(self.option_retry_text, 600, 250, "NotoSans-Regular", 40, 255, 255, 255, 255)
					Text.Draw(self.option_proceed_text, 600, 300, "NotoSans-Regular", 40, 255, 255, 255, 255)

					-- cursor
					
					-- Mouse
					local mouse_being_used = false
					local cursor_pos = Input.GetMousePosition()
					if cursor_pos.x > 550 and cursor_pos.x < 900 then
						if cursor_pos.y > 250 and cursor_pos.y < 306 then
							self.option_mode_selection = 0
							mouse_being_used = true
						end
						if cursor_pos.y > 306 and cursor_pos.y < 350 then
							self.option_mode_selection = 1
							mouse_being_used = true
						end
					end

					if mouse_being_used and Input.GetMouseButtonDown(1) then
						self.did_start_leaving = true
					end

					-- Keyboard
					if Input.GetKeyDown("up") or Input.GetKeyDown("w") then
						self.option_mode_selection = self.option_mode_selection - 1
					end

					if Input.GetKeyDown("down") or Input.GetKeyDown("s") then
						self.option_mode_selection = self.option_mode_selection + 1
					end

					self.option_mode_selection = self.option_mode_selection % 2 -- wrap

					if Input.GetKeyDown("enter") then
						self.did_start_leaving = true
					end

				else
					-- Animate
					if self.option_mode_selection == 0 then
						-- Kick animation
						self.spot_option_x = self.spot_option_x + 0.4
						if self.spot_option_x > 6 then
							self:Exit(true)
						end
					else
						if self.should_spot_fall == false then
							-- Kick animation
							self.spot_option_x = self.spot_option_x + 0.4
							if self.spot_option_x > 6 then
								self:Exit(false)
							end
						else
							-- Fall animation
							if self.spot_option_fall_yelled == false then
								Audio.Play(1, "player_damage", false)
								self.spot_option_fall_yelled = true
							end

							self.spot_option_yvel = self.spot_option_yvel + 0.003
							self.spot_option_y = self.spot_option_y + self.spot_option_yvel

							if self.spot_option_y > 10 then
								self:Exit(false)
							end

							if self.spot_option_yvel < 0 then
								self.spot_option_sprite = "spot_cursor_surprised"
							else
								self.spot_option_sprite = "spot_cursor_ko"
							end
						end
					end

				end

				Image.DrawEx(self.spot_option_sprite, -1.5 + self.spot_option_x, self.option_mode_selection * 0.5 + 1.2 + self.spot_option_y, 0, 1, 1, 0.5, 1, 255, 255, 255, 255, 1)
			
			end
		end

		if self.should_spot_fall then
			if self.played_damage_vocal == false then
				self.played_damage_vocal = true
				Audio.Play(1, "player_damage", false)
			end

			self.spot_fall_vel = self.spot_fall_vel - 0.004
			self.spot_y = self.spot_y + self.spot_fall_vel
			if self.spot_fall_vel > 0 then
				self.spot_sprite = "player_surprise_big"
			end

			if self.spot_fall_vel <= 0 then
				self.spot_sprite = "player_ko_big"
			end
		end

		if self.option_mode == false then
			Image.DrawEx(self.spot_sprite, -self.distance_from_crown, 2.5 - self.spot_y, character_rot, self.spot_x_scale, 1, 0.5, 1, 255, 255, 255, 255, 1)
			Text.Draw("Movements : " .. movements .. "/" .. movements_par_number, 50, self.movements_y, "NotoSans-Regular", 40, 255, 255, 255, 255)
			Text.Draw("Secrets : " .. secrets_collected .. "/" .. secrets_available, 800, self.secrets_y, "NotoSans-Regular", 40, 255, 255, 255, 255)
			Text.Draw("Boyfriend " .. cuff_success_string, 450, self.cuff_y, "NotoSans-Regular", 40, 255, 255, 255, 255)

			-- Left life
			if self.movements_bounced then
				self.movements_life_flash_timer = self.movements_life_flash_timer - 1
				if Application.GetFrame() % 20 > 10 or self.movements_life_flash_timer < 0 then
					if self.did_succeed_movements then
						Image.DrawEx("1up", -3.5, 1, 0, 1, 1, 0.5, 1, 255, 255, 255, 255, 0.2)
					else
						Image.DrawEx("red_x", -3.5, 1, 0, 1, 1, 0.5, 1, 255, 255, 255, 255, 0.2)
					end
				end
			end

			-- Right life
			if self.secrets_bounced then
				self.secrets_life_flash_timer = self.secrets_life_flash_timer - 1
				if Application.GetFrame() % 20 > 10 or self.secrets_life_flash_timer < 0 then
					if self.did_succeed_secrets then
						Image.DrawEx("1up", 3.5, 1, 0, 1, 1, 0.5, 1, 255, 255, 255, 255, 0.2)
					else
						Image.DrawEx("red_x", 3.5, 1, 0, 1, 1, 0.5, 1, 255, 255, 255, 255, 0.2)
					end
				end
			end

			-- Top life
			if self.cuff_bounced then
				self.cuff_life_flash_timer = self.cuff_life_flash_timer - 1
				if Application.GetFrame() % 20 > 10 or self.cuff_life_flash_timer < 0 then
					if self.did_succeed_cuff then
						Image.DrawEx("1up", 0, -1, 0, 1, 1, 0.5, 1, 255, 255, 255, 255, 0.2)
					else
						Image.DrawEx("red_x", 0, -1, 0, 1, 1, 0.5, 1, 255, 255, 255, 255, 0.2)
					end
				end
			end
		end

	end,

	exiting = false,

	Exit = function(self, retry)

		if self.exiting then
			return
		end
		self.exiting = true

		if retry then
			fast_pre_stage = true
			next_stage = previous_stage
		else
			fast_pre_stage = false
			if self.did_succeed_movements then
				lives = lives + 1
			end
			if self.did_succeed_secrets then
				lives = lives + 1
			end
			if self.did_succeed_cuff then
				lives = lives + 1
			end
		end

		SaveGame(next_stage, lives, continues)
		local transition_component = Actor.Find("SceneTransitionManager"):GetComponent("SceneTransitionManager")
		transition_component:RequestChangeScene("prestage")
	end
}

