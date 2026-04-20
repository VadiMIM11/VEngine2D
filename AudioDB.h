#pragma once
#include "AudioHelper.h"
#include <string>
#include <unordered_map>

class AudioDB
{
public:
	AudioDB();
	~AudioDB();

	// Load a wav/ogg file into a chunk. 
	// Returns nullptr if load fails (handled by Engine).
	Mix_Chunk* LoadChunk(const std::string& path);

	// Play a chunk on a specific channel.
	void Play(int channel, Mix_Chunk* chunk, int loops);

	// Halt a specific channel.
	void Halt(int channel);

	// Set channel volume (0-128).
	void SetVolume(int channel, int volume);	

private:
	std::unordered_map<std::string, Mix_Chunk*> chunk_map;
};