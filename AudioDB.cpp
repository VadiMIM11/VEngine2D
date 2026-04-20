#include "AudioDB.h"
#include <iostream>

AudioDB::AudioDB()
{
	// Initialize SDL_Mixer via AudioHelper
	// Standard High Quality Audio Settings:
	// Frequency: 44100 Hz
	// Format: Default (16-bit)
	// Channels: 2 (Stereo)
	// Chunksize: 2048
	AudioHelper::Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048);
	AudioHelper::Mix_AllocateChannels(50); // Allocate more channels (50) to reduce clipping
}

AudioDB::~AudioDB()
{
	AudioHelper::Mix_CloseAudio();
}

Mix_Chunk* AudioDB::LoadChunk(const std::string& path)
{
	auto it = chunk_map.find(path);
	if (it != chunk_map.end())
	{
		return it->second;
	}

	Mix_Chunk* chunk = AudioHelper::Mix_LoadWAV(path.c_str());
	if (chunk)
	{
		chunk_map[path] = chunk;
	}
	return chunk;
}

void AudioDB::Play(int channel, Mix_Chunk* chunk, int loops)
{
	if (chunk)
	{
		AudioHelper::Mix_PlayChannel(channel, chunk, loops);
	}
}

void AudioDB::Halt(int channel)
{
	AudioHelper::Mix_HaltChannel(channel);
}

void AudioDB::SetVolume(int channel, int volume)
{
	AudioHelper::Mix_Volume(channel, volume);
}