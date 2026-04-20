#include "ParticleSystem.h"

#include <cmath>
#include "glm/glm.hpp"
#include "ImageDB.h"
#include "ScriptBindings.h"

namespace
{
	int ClampColorChannel(int value)
	{
		if (value < 0)
		{
			return 0;
		}
		if (value > 255)
		{
			return 255;
		}
		return value;
	}

	int ClampMinOne(int value)
	{
		return (value < 1) ? 1 : value;
	}
}

void ParticleSystem::SetImageDB(ImageDB* image_db)
{
	s_image_db = image_db;
}

void ParticleSystem::EnsureDefaultTexture()
{
	if (s_image_db != nullptr)
	{
		s_image_db->CreateDefaultParticleTextureWithName(default_particle_texture_name);
	}
}

void ParticleSystem::OnStart()
{
	frames_between_bursts = ClampMinOne(frames_between_bursts);
	burst_quantity = ClampMinOne(burst_quantity);
	duration_frames = ClampMinOne(duration_frames);

	start_color_r = ClampColorChannel(start_color_r);
	start_color_g = ClampColorChannel(start_color_g);
	start_color_b = ClampColorChannel(start_color_b);
	start_color_a = ClampColorChannel(start_color_a);

	if (end_color_r >= 0) end_color_r = ClampColorChannel(end_color_r);
	if (end_color_g >= 0) end_color_g = ClampColorChannel(end_color_g);
	if (end_color_b >= 0) end_color_b = ClampColorChannel(end_color_b);
	if (end_color_a >= 0) end_color_a = ClampColorChannel(end_color_a);

	emit_angle_distribution.Configure(emit_angle_min, emit_angle_max, 298);
	emit_radius_distribution.Configure(emit_radius_min, emit_radius_max, 404);
	rotation_distribution.Configure(rotation_min, rotation_max, 440);
	scale_distribution.Configure(start_scale_min, start_scale_max, 494);
	speed_distribution.Configure(start_speed_min, start_speed_max, 498);
	rotation_speed_distribution.Configure(rotation_speed_min, rotation_speed_max, 305);

	EnsureDefaultTexture();
}

void ParticleSystem::CreateParticle()
{
	float angle_radians = glm::radians(emit_angle_distribution.Sample());
	float radius = emit_radius_distribution.Sample();

	float cos_angle = glm::cos(angle_radians);
	float sin_angle = glm::sin(angle_radians);

	float starting_x_pos = x + cos_angle * radius;
	float starting_y_pos = y + sin_angle * radius;

	float speed = speed_distribution.Sample();
	float starting_x_vel = cos_angle * speed;
	float starting_y_vel = sin_angle * speed;

	float starting_rotation = rotation_distribution.Sample();
	float starting_scale = scale_distribution.Sample();
	float starting_rotation_speed = rotation_speed_distribution.Sample();

	int slot_index = -1;
	if (!free_list.empty())
	{
		slot_index = free_list.front();
		free_list.pop();
	}
	else
	{
		slot_index = static_cast<int>(is_active.size());
		is_active.push_back(false);
		start_frame.push_back(0);

		particle_x_positions.push_back(0.0f);
		particle_y_positions.push_back(0.0f);
		particle_x_velocities.push_back(0.0f);
		particle_y_velocities.push_back(0.0f);
		particle_rotations.push_back(0.0f);
		particle_angular_velocities.push_back(0.0f);
		particle_scales.push_back(1.0f);
		particle_initial_scales.push_back(1.0f);
		particle_start_color_r.push_back(255.0f);
		particle_start_color_g.push_back(255.0f);
		particle_start_color_b.push_back(255.0f);
		particle_start_color_a.push_back(255.0f);
		particle_color_r.push_back(255.0f);
		particle_color_g.push_back(255.0f);
		particle_color_b.push_back(255.0f);
		particle_color_a.push_back(255.0f);
	}

	is_active[slot_index] = true;
	start_frame[slot_index] = local_frame_number;

	particle_x_positions[slot_index] = starting_x_pos;
	particle_y_positions[slot_index] = starting_y_pos;
	particle_x_velocities[slot_index] = starting_x_vel;
	particle_y_velocities[slot_index] = starting_y_vel;
	particle_rotations[slot_index] = starting_rotation;
	particle_angular_velocities[slot_index] = starting_rotation_speed;
	particle_scales[slot_index] = starting_scale;
	particle_initial_scales[slot_index] = starting_scale;

	particle_start_color_r[slot_index] = static_cast<float>(start_color_r);
	particle_start_color_g[slot_index] = static_cast<float>(start_color_g);
	particle_start_color_b[slot_index] = static_cast<float>(start_color_b);
	particle_start_color_a[slot_index] = static_cast<float>(start_color_a);
	particle_color_r[slot_index] = static_cast<float>(start_color_r);
	particle_color_g[slot_index] = static_cast<float>(start_color_g);
	particle_color_b[slot_index] = static_cast<float>(start_color_b);
	particle_color_a[slot_index] = static_cast<float>(start_color_a);

	active_particle_count++;
}

void ParticleSystem::CreateBurst(int count)
{
	int clamped_count = ClampMinOne(count);
	for (int i = 0; i < clamped_count; ++i)
	{
		CreateParticle();
	}
}

void ParticleSystem::Stop()
{
	emission_allowed = false;
}

void ParticleSystem::Play()
{
	emission_allowed = true;
}

void ParticleSystem::Burst()
{
	CreateBurst(burst_quantity);
}

void ParticleSystem::OnUpdate()
{
	frames_between_bursts = ClampMinOne(frames_between_bursts);
	burst_quantity = ClampMinOne(burst_quantity);
	duration_frames = ClampMinOne(duration_frames);

	start_color_r = ClampColorChannel(start_color_r);
	start_color_g = ClampColorChannel(start_color_g);
	start_color_b = ClampColorChannel(start_color_b);
	start_color_a = ClampColorChannel(start_color_a);

	if (end_color_r >= 0) end_color_r = ClampColorChannel(end_color_r);
	if (end_color_g >= 0) end_color_g = ClampColorChannel(end_color_g);
	if (end_color_b >= 0) end_color_b = ClampColorChannel(end_color_b);
	if (end_color_a >= 0) end_color_a = ClampColorChannel(end_color_a);

	if (local_frame_number % frames_between_bursts == 0 && emission_allowed)
	{
		CreateBurst(burst_quantity);
	}

	const bool has_end_scale = !std::isnan(end_scale);
	const bool has_end_color_r = end_color_r >= 0;
	const bool has_end_color_g = end_color_g >= 0;
	const bool has_end_color_b = end_color_b >= 0;
	const bool has_end_color_a = end_color_a >= 0;
	const std::string& texture_name = image.empty() ? default_particle_texture_name : image;

	for (int i = 0; i < static_cast<int>(is_active.size()); ++i)
	{
		if (!is_active[i])
		{
			continue;
		}

		int frames_particle_has_been_alive = local_frame_number - start_frame[i];
		if (frames_particle_has_been_alive >= duration_frames)
		{
			is_active[i] = false;
			free_list.push(i);
			active_particle_count--;
			continue;
		}

		particle_x_velocities[i] += gravity_scale_x;
		particle_y_velocities[i] += gravity_scale_y;

		particle_x_velocities[i] *= drag_factor;
		particle_y_velocities[i] *= drag_factor;
		particle_angular_velocities[i] *= angular_drag_factor;

		particle_x_positions[i] += particle_x_velocities[i];
		particle_y_positions[i] += particle_y_velocities[i];
		particle_rotations[i] += particle_angular_velocities[i];

		float lifetime_progress = static_cast<float>(frames_particle_has_been_alive) / static_cast<float>(duration_frames);
		if (has_end_scale)
		{
			particle_scales[i] = glm::mix(particle_initial_scales[i], end_scale, lifetime_progress);
		}

		if (has_end_color_r)
		{
			particle_color_r[i] = glm::mix(particle_start_color_r[i], static_cast<float>(end_color_r), lifetime_progress);
		}
		if (has_end_color_g)
		{
			particle_color_g[i] = glm::mix(particle_start_color_g[i], static_cast<float>(end_color_g), lifetime_progress);
		}
		if (has_end_color_b)
		{
			particle_color_b[i] = glm::mix(particle_start_color_b[i], static_cast<float>(end_color_b), lifetime_progress);
		}
		if (has_end_color_a)
		{
			particle_color_a[i] = glm::mix(particle_start_color_a[i], static_cast<float>(end_color_a), lifetime_progress);
		}

		ScriptBindings::QueueImageDrawEx(
			texture_name,
			particle_x_positions[i],
			particle_y_positions[i],
			particle_rotations[i],
			particle_scales[i],
			particle_scales[i],
			0.5f,
			0.5f,
			particle_color_r[i],
			particle_color_g[i],
			particle_color_b[i],
			particle_color_a[i],
			static_cast<float>(sorting_order));
	}

	local_frame_number++;
}
