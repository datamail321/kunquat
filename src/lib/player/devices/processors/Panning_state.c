

/*
 * Author: Tomi Jylhä-Ollila, Finland 2016
 *
 * This file is part of Kunquat.
 *
 * CC0 1.0 Universal, http://creativecommons.org/publicdomain/zero/1.0/
 *
 * To the extent possible under law, Kunquat Affirmers have waived all
 * copyright and related or neighboring rights to Kunquat.
 */


#include <player/devices/processors/Panning_state.h>

#include <debug/assert.h>
#include <init/devices/Device.h>
#include <init/devices/processors/Proc_panning.h>
#include <mathnum/common.h>
#include <memory.h>
#include <player/Linear_controls.h>
#include <player/Work_buffers.h>

#include <stdint.h>
#include <stdlib.h>


static const int CONTROL_WB_PANNING = WORK_BUFFER_IMPL_1;


static void apply_panning(
        const Work_buffers* wbs,
        const float* in_buffers[2],
        float* out_buffers[2],
        int32_t buf_start,
        int32_t buf_stop,
        int32_t audio_rate)
{
    assert(wbs != NULL);
    assert(in_buffers != NULL);
    assert(out_buffers != NULL);
    assert(buf_start >= 0);
    assert(buf_stop >= 0);
    assert(audio_rate > 0);

    float* pannings = Work_buffers_get_buffer_contents_mut(wbs, CONTROL_WB_PANNING);

    // Clamp the input values
    for (int32_t i = buf_start; i < buf_stop; ++i)
    {
        float panning = pannings[i];
        panning = clamp(panning, -1, 1);
        pannings[i] = panning;
    }

    // Apply panning
    // TODO: revisit panning formula
    {
        const float* in_buf = in_buffers[0];
        float* out_buf = out_buffers[0];
        if ((in_buf != NULL) && (out_buf != NULL))
        {
            for (int32_t i = buf_start; i < buf_stop; ++i)
                out_buf[i] = in_buf[i] * (1 - pannings[i]);
        }
    }

    {
        const float* in_buf = in_buffers[1];
        float* out_buf = out_buffers[1];
        if ((in_buf != NULL) && (out_buf != NULL))
        {
            for (int32_t i = buf_start; i < buf_stop; ++i)
                out_buf[i] = in_buf[i] * (1 + pannings[i]);
        }
    }

    return;
}


typedef struct Panning_pstate
{
    Proc_state parent;
    Linear_controls panning;
} Panning_pstate;


static bool Panning_pstate_set_audio_rate(Device_state* dstate, int32_t audio_rate)
{
    assert(dstate != NULL);
    assert(audio_rate > 0);

    Panning_pstate* ppstate = (Panning_pstate*)dstate;
    Linear_controls_set_audio_rate(&ppstate->panning, audio_rate);

    return true;
}


static void Panning_pstate_set_tempo(Device_state* dstate, double tempo)
{
    assert(dstate != NULL);
    assert(isfinite(tempo));
    assert(tempo > 0);

    Panning_pstate* ppstate = (Panning_pstate*)dstate;
    Linear_controls_set_tempo(&ppstate->panning, tempo);

    return;
}


static void Panning_pstate_reset(Device_state* dstate)
{
    assert(dstate != NULL);

    const Proc_panning* panning = (const Proc_panning*)dstate->device->dimpl;

    Panning_pstate* ppstate = (Panning_pstate*)dstate;
    Linear_controls_init(&ppstate->panning);
    Linear_controls_set_value(&ppstate->panning, panning->panning);

    return;
}


static void Panning_pstate_render_mixed(
        Device_state* dstate,
        const Work_buffers* wbs,
        int32_t buf_start,
        int32_t buf_stop,
        double tempo)
{
    assert(dstate != NULL);
    assert(wbs != NULL);
    assert(isfinite(tempo));
    assert(tempo > 0);

    Panning_pstate* ppstate = (Panning_pstate*)dstate;

    // Update controls
    Linear_controls_set_tempo(&ppstate->panning, tempo);

    const Work_buffer* panning_wb = Work_buffers_get_buffer(wbs, CONTROL_WB_PANNING);
    Linear_controls_fill_work_buffer(&ppstate->panning, panning_wb, buf_start, buf_stop);

    // Get input
    const float* in_buffers[] =
    {
        Device_state_get_audio_buffer_contents_mut(dstate, DEVICE_PORT_TYPE_RECEIVE, 0),
        Device_state_get_audio_buffer_contents_mut(dstate, DEVICE_PORT_TYPE_RECEIVE, 1),
    };

    // Get output
    float* out_buffers[] =
    {
        Device_state_get_audio_buffer_contents_mut(dstate, DEVICE_PORT_TYPE_SEND, 0),
        Device_state_get_audio_buffer_contents_mut(dstate, DEVICE_PORT_TYPE_SEND, 1),
    };

    apply_panning(wbs, in_buffers, out_buffers, buf_start, buf_stop, dstate->audio_rate);

    return;
}


bool Panning_pstate_set_panning(
        Device_state* dstate, const Key_indices indices, double value)
{
    assert(dstate != NULL);
    assert(indices != NULL);
    assert(isfinite(value));

    Panning_pstate* ppstate = (Panning_pstate*)dstate;
    Linear_controls_set_value(&ppstate->panning, value);

    return true;
}


Device_state* new_Panning_pstate(
        const Device* device, int32_t audio_rate, int32_t audio_buffer_size)
{
    assert(device != NULL);
    assert(audio_rate > 0);
    assert(audio_buffer_size >= 0);

    Panning_pstate* ppstate = memory_alloc_item(Panning_pstate);
    if ((ppstate == NULL) ||
            !Proc_state_init(&ppstate->parent, device, audio_rate, audio_buffer_size))
    {
        memory_free(ppstate);
        return NULL;
    }

    ppstate->parent.set_audio_rate = Panning_pstate_set_audio_rate;
    ppstate->parent.set_tempo = Panning_pstate_set_tempo;
    ppstate->parent.reset = Panning_pstate_reset;
    ppstate->parent.render_mixed = Panning_pstate_render_mixed;

    Device_state* dstate = (Device_state*)ppstate;
    Linear_controls_init(&ppstate->panning);
    Panning_pstate_set_audio_rate(dstate, audio_rate);

    return dstate;
}


Linear_controls* Panning_pstate_get_cv_controls_panning(
        Device_state* dstate, const Key_indices indices)
{
    assert(dstate != NULL);
    ignore(indices);

    Panning_pstate* ppstate = (Panning_pstate*)dstate;

    return &ppstate->panning;
}


typedef struct Panning_vstate
{
    Voice_state parent;
    Linear_controls panning;
} Panning_vstate;


size_t Panning_vstate_get_size(void)
{
    return sizeof(Panning_vstate);
}


int32_t Panning_vstate_render_voice(
        Voice_state* vstate,
        Proc_state* proc_state,
        const Au_state* au_state,
        const Work_buffers* wbs,
        int32_t buf_start,
        int32_t buf_stop,
        double tempo)
{
    assert(vstate != NULL);
    assert(proc_state != NULL);
    assert(au_state != NULL);
    assert(wbs != NULL);
    assert(buf_start >= 0);
    assert(buf_stop >= 0);
    assert(isfinite(tempo));
    assert(tempo > 0);

    Panning_vstate* pvstate = (Panning_vstate*)vstate;

    // Update controls
    Linear_controls_set_tempo(&pvstate->panning, tempo);

    const Work_buffer* panning_wb = Work_buffers_get_buffer(wbs, CONTROL_WB_PANNING);
    Linear_controls_fill_work_buffer(&pvstate->panning, panning_wb, buf_start, buf_stop);

    // Get input
    const float* in_buffers[] =
    {
        Proc_state_get_voice_buffer_contents_mut(
                proc_state, DEVICE_PORT_TYPE_RECEIVE, 0),
        Proc_state_get_voice_buffer_contents_mut(
                proc_state, DEVICE_PORT_TYPE_RECEIVE, 1),
    };
    if ((in_buffers[0] == NULL) && (in_buffers[1] == NULL))
    {
        vstate->active = false;
        return buf_start;
    }

    // Get output
    float* out_buffers[] =
    {
        Proc_state_get_voice_buffer_contents_mut(proc_state, DEVICE_PORT_TYPE_SEND, 0),
        Proc_state_get_voice_buffer_contents_mut(proc_state, DEVICE_PORT_TYPE_SEND, 1),
    };

    const Device_state* dstate = (const Device_state*)proc_state;
    apply_panning(wbs, in_buffers, out_buffers, buf_start, buf_stop, dstate->audio_rate);

    return buf_stop;
}


void Panning_vstate_init(Voice_state* vstate, const Proc_state* proc_state)
{
    assert(vstate != NULL);
    assert(proc_state != NULL);

    vstate->render_voice = Panning_vstate_render_voice;

    Panning_vstate* pvstate = (Panning_vstate*)vstate;

    const Device_state* dstate = (const Device_state*)proc_state;
    const Proc_panning* panning = (const Proc_panning*)dstate->device->dimpl;
    Linear_controls_init(&pvstate->panning);
    Linear_controls_set_audio_rate(&pvstate->panning, dstate->audio_rate);
    Linear_controls_set_value(&pvstate->panning, panning->panning);

    return;
}


Linear_controls* Panning_vstate_get_cv_controls_panning(
        Voice_state* vstate, const Device_state* dstate, const Key_indices indices)
{
    assert(vstate != NULL);
    assert(dstate != NULL);
    ignore(indices);

    Panning_vstate* pvstate = (Panning_vstate*)vstate;

    return &pvstate->panning;
}

