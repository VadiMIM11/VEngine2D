LaunchDialogueWhenNear = {
	dialogue_set_name = "...",
	dialogue_distance = 2,

	OnStart = function(self)
		if game_over then
			return
		end
		
		self.transform = self.actor:GetComponent("Transform")
	end,

	OnUpdate = function(self)
		
		local player = Actor.Find("Player")
		if player == nil or dialogue_running or activated_dialogues[self.dialogue_set_name] == true then
			return
		end

		local player_t = player:GetComponent("Transform")

		-- Check for collision with player.
		local player_pos = {x = player_t.x, y = player_t.y}
		local our_pos = {x = self.transform.x, y = self.transform.y}

		local distance = self.Distance(our_pos, player_pos)
		if distance < self.dialogue_distance then
			self:LaunchDialogue()
		end
	end,

	LaunchDialogue = function(self)

		-- Check if we've already launched this dialogue
		if activated_dialogues[self.dialogue_set_name] == true then
			return
		end

		activated_dialogues[self.dialogue_set_name] = true

		local dialogue_manager_component = Actor.Find("DialogueManager"):GetComponent("DialogueManager")
		dialogue_manager_component:LaunchDialogue(self.dialogue_set_name)
	end,

	Distance = function(p1, p2)
		local dx = p1.x - p2.x
		local dy = p1.y - p2.y
		return math.sqrt(dx*dx + dy*dy)
	end,
}

