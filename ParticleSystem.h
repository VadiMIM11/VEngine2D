#pragma once

#include <string>
#include <vector>
#include <queue>
#include <limits>
#include "Helper.h"

class Actor;
class ImageDB;

class ParticleSystem
{
public:
	std::string key;
	std::string type;
	bool enabled = true;

	Actor* actor = nullptr;
	std::string actor_name;
	int id = -1;

	float x = 0.0f;
	float y = 0.0f;

	int frames_between_bursts = 1;
	int burst_quantity = 1;
	int duration_frames = 300;

	float start_scale_min = 1.0f;
	float start_scale_max = 1.0f;
	float rotation_min = 0.0f;
	float rotation_max = 0.0f;
	float end_scale = std::numeric_limits<float>::quiet_NaN();

	int start_color_r = 255;
	int start_color_g = 255;
	int start_color_b = 255;
	int start_color_a = 255;
	int end_color_r = -1;
	int end_color_g = -1;
	int end_color_b = -1;
	int end_color_a = -1;

	float emit_angle_min = 0.0f;
	float emit_angle_max = 360.0f;
	float emit_radius_min = 0.0f;
	float emit_radius_max = 0.5f;

	float start_speed_min = 0.0f;
	float start_speed_max = 0.0f;
	float rotation_speed_min = 0.0f;
	float rotation_speed_max = 0.0f;
	float gravity_scale_x = 0.0f;
	float gravity_scale_y = 0.0f;
	float drag_factor = 1.0f;
	float angular_drag_factor = 1.0f;

	std::string image = "";
	int sorting_order = 9999;

	void OnStart();
	void OnUpdate();
	void Stop();
	void Play();
	void Burst();

	static void SetImageDB(ImageDB* image_db);

private:
	int local_frame_number = 0;
	int active_particle_count = 0;
	bool emission_allowed = true;

	RandomEngine emit_angle_distribution;
	RandomEngine emit_radius_distribution;
	RandomEngine rotation_distribution;
	RandomEngine scale_distribution;
	RandomEngine speed_distribution;
	RandomEngine rotation_speed_distribution;

	std::vector<bool> is_active;
	std::vector<int> start_frame;
	std::queue<int> free_list;

	std::vector<float> particle_x_positions;
	std::vector<float> particle_y_positions;
	std::vector<float> particle_x_velocities;
	std::vector<float> particle_y_velocities;
	std::vector<float> particle_rotations;
	std::vector<float> particle_angular_velocities;
	std::vector<float> particle_scales;
	std::vector<float> particle_initial_scales;
	std::vector<float> particle_start_color_r;
	std::vector<float> particle_start_color_g;
	std::vector<float> particle_start_color_b;
	std::vector<float> particle_start_color_a;
	std::vector<float> particle_color_r;
	std::vector<float> particle_color_g;
	std::vector<float> particle_color_b;
	std::vector<float> particle_color_a;

	void CreateParticle();
	void CreateBurst(int count);
	void EnsureDefaultTexture();

	inline static const std::string default_particle_texture_name = "__default_particle";
	inline static ImageDB* s_image_db = nullptr;
};
