

#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include <jack/jack.h>

#include <Player.h>
#include <Song.h>
#include "Driver_jack.h"

#include <frame_t.h>


/**
 * Mixing callback function for JACK.
 *
 * \param nframes   Number of frames to be mixed.
 * \param arg       A Player object. TODO: This will be changed into a list
 *                  of Player objects later.
 *
 * \return   Zero on success, non-zero on error.
 */
static int Driver_jack_process(jack_nframes_t nframes, void* arg);


/**
 * Freewheel callback function.
 *
 * \param starting   Freewheel state. Non-zero means freewheel, zero means
 *                   realtime.
 * \param arg        A Player object. TODO: This will be changed into a list
 *                   of Player objects later.
 */
//static void Driver_jack_freewheel(int starting, void* arg);


/**
 * Buffer size change callback function.
 *
 * \param nframes   New buffer size.
 * \param arg       A Player object. TODO: This will be changed into a list
 *                  of Player objects later.
 *
 * \return   Zero on success, non-zero on error.
 */
//static int Driver_jack_bufsize(jack_nframes_t nframes, void* arg);


/**
 * Sample rate change callback function.
 *
 * \param nframes   New sample rate.
 * \param arg       A Player object. TODO: This will be changed into a list
 *                  of Player objects later.
 *
 * \return   Zero on success, non-zero on error.
 */
//static int Driver_jack_sample_rate(jack_nframes_t nframes, void* arg);


/**
 * XRun callback function.
 *
 * \param arg   TODO: TBD
 *
 * \return   Zero on success, non-zero on error.
 */
//static int Driver_jack_xrun(void* arg);


/**
 * The JACK client handle.
 */
static jack_client_t* handle = NULL;


/**
 * Ports used for output. TODO: size
 */
static jack_port_t* ports[2] = { NULL };


/**
 * Active status of the driver.
 */
static bool active = false;


/**
 * Mixing rate.
 */
static uint32_t mix_freq = 48000;


bool Driver_jack_init(Playlist* playlist, uint32_t* freq)
{
	assert(playlist != NULL);
	assert(freq != NULL);
	jack_status_t status = 0;
	if (handle == NULL)
	{
		handle = jack_client_open("Kunquat", JackNullOption, &status);
		if (handle == NULL)
		{
			return false;
		}
		if (jack_set_process_callback(handle, Driver_jack_process, playlist) != 0)
		{
			handle = NULL;
			return false;
		}
	}
	if (ports[0] == NULL)
	{
		ports[0] = jack_port_register(handle,
				"out_l",
				JACK_DEFAULT_AUDIO_TYPE,
				JackPortIsOutput | JackPortIsTerminal, 0);
		if (ports[0] == NULL)
		{
			return false;
		}
	}
	if (ports[1] == NULL)
	{
		ports[1] = jack_port_register(handle,
				"out_r",
				JACK_DEFAULT_AUDIO_TYPE,
				JackPortIsOutput | JackPortIsTerminal, 0);
		if (ports[1] == NULL)
		{
			return false;
		}
	}
	if (!active)
	{
		if (jack_activate(handle) != 0)
		{
			return false;
		}
		active = true;
	}
	const char** available_ports = jack_get_ports(handle,
			NULL,
			NULL,
			JackPortIsPhysical | JackPortIsInput);
	if (available_ports == NULL)
	{
		return false;
	}
	if (jack_connect(handle,
			jack_port_name(ports[0]),
			available_ports[0]) != 0)
	{
		free(available_ports);
		return false;
	}
	if (jack_connect(handle,
			jack_port_name(ports[1]),
			available_ports[1]) != 0)
	{
		free(available_ports);
		return false;
	}
	free(available_ports);
	*freq = mix_freq = jack_get_sample_rate(handle);
	return true;
}


static int Driver_jack_process(jack_nframes_t nframes, void* arg)
{
	assert(arg != NULL);
	if (!active)
	{
		return 0;
	}
	Player* player = (Player*)arg;
	if (!player->play->mode)
	{
		return 0;
	}
	assert(player->play->mode > STOP);
	assert(player->play->mode < PLAY_LAST);
	uint32_t mixed = Player_mix(player, nframes);
	jack_default_audio_sample_t* out_l = jack_port_get_buffer(ports[0], nframes);
	jack_default_audio_sample_t* out_r = jack_port_get_buffer(ports[1], nframes);
	int buf_count = Song_get_buf_count(player->song);
	frame_t** bufs = Song_get_bufs(player->song);
	frame_t* buf_l = bufs[0];
	frame_t* buf_r = bufs[0];
	if (buf_count > 1)
	{
		buf_r = bufs[1];
	}
	for (uint32_t i = 0; i < mixed; ++i)
	{
		*out_l++ = *buf_l++;
		*out_r++ = *buf_r++;
	}
	return 0;
}

