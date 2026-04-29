TurnBased = {

	priority = 0,

	OnStart = function(self)

		-- Register ourselves to be called by the gamemanager when a new turn occurs (ie, when the player moves).
		table.insert(turn_based_actors, self.actor)

	end

}

