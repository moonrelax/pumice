#include "audio.h"
#include "../core/log.h"
#include <stdlib.h>
#include <string.h>

void audio_init(audio_state *audio) {
    memset(audio, 0, sizeof(*audio));
    audio->sample_rate = AUDIO_SAMPLE_RATE;
    audio->initialized = true;
    for (int i = 0; i < AUDIO_MAX_TRACKS; i++)
        audio->tracks[i].volume = 1.0f;
    LOG_INFO("audio: initialized %u Hz stereo", audio->sample_rate);
}

void audio_destroy(audio_state *audio) {
    for (int i = 0; i < AUDIO_MAX_TRACKS; i++) {
        free(audio->tracks[i].buffer);
        audio->tracks[i].buffer = NULL;
    }
    audio->initialized = false;
}

int audio_open_track(audio_state *audio) {
    for (int i = 0; i < AUDIO_MAX_TRACKS; i++) {
        if (!audio->tracks[i].buffer && !audio->tracks[i].playing) {
            audio->tracks[i].volume = 1.0f;
            audio->tracks[i].position = 0;
            return i;
        }
    }
    return -1;
}

void audio_close_track(audio_state *audio, int track) {
    if (track < 0 || track >= AUDIO_MAX_TRACKS) return;
    free(audio->tracks[track].buffer);
    memset(&audio->tracks[track], 0, sizeof(audio_track));
}

void audio_set_buffer(audio_state *audio, int track, int16_t *data, uint32_t samples) {
    if (track < 0 || track >= AUDIO_MAX_TRACKS) return;
    free(audio->tracks[track].buffer);
    audio->tracks[track].buffer = malloc(samples * AUDIO_CHANNELS * sizeof(int16_t));
    memcpy(audio->tracks[track].buffer, data, samples * AUDIO_CHANNELS * sizeof(int16_t));
    audio->tracks[track].sample_count = samples;
    audio->tracks[track].position = 0;
}

void audio_play(audio_state *audio, int track) {
    if (track < 0 || track >= AUDIO_MAX_TRACKS) return;
    audio->tracks[track].playing = true;
}

void audio_stop(audio_state *audio, int track) {
    if (track < 0 || track >= AUDIO_MAX_TRACKS) return;
    audio->tracks[track].playing = false;
    audio->tracks[track].position = 0;
}

void audio_set_volume(audio_state *audio, int track, float volume) {
    if (track < 0 || track >= AUDIO_MAX_TRACKS) return;
    if (volume < 0.0f) volume = 0.0f;
    if (volume > 1.0f) volume = 1.0f;
    audio->tracks[track].volume = volume;
}

void audio_mix_frame(audio_state *audio) {
    uint32_t frame_samples = audio->sample_rate / 60;
    memset(audio->mix_buffer, 0, frame_samples * AUDIO_CHANNELS * sizeof(int16_t));

    for (int t = 0; t < AUDIO_MAX_TRACKS; t++) {
        audio_track *trk = &audio->tracks[t];
        if (!trk->playing || !trk->buffer) continue;

        for (uint32_t i = 0; i < frame_samples && trk->position < trk->sample_count; i++) {
            for (int ch = 0; ch < AUDIO_CHANNELS; ch++) {
                int32_t sample = audio->mix_buffer[i * AUDIO_CHANNELS + ch];
                int32_t add = (int32_t)(trk->buffer[trk->position * AUDIO_CHANNELS + ch] * trk->volume);
                sample += add;
                if (sample > 32767) sample = 32767;
                if (sample < -32768) sample = -32768;
                audio->mix_buffer[i * AUDIO_CHANNELS + ch] = (int16_t)sample;
            }
            trk->position++;
        }

        if (trk->position >= trk->sample_count) {
            if (trk->looping) trk->position = 0;
            else trk->playing = false;
        }
    }
}

const int16_t *audio_get_output(audio_state *audio) {
    return audio->mix_buffer;
}
