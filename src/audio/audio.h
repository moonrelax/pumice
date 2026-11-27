#ifndef PUMICE_AUDIO_H
#define PUMICE_AUDIO_H

#include <stdint.h>
#include <stdbool.h>

#define AUDIO_SAMPLE_RATE  48000
#define AUDIO_CHANNELS     2
#define AUDIO_MAX_TRACKS   4
#define AUDIO_BUFFER_SIZE  (AUDIO_SAMPLE_RATE / 60 * AUDIO_CHANNELS)

typedef enum {
    AUDIO_FMT_PCM16,
    AUDIO_FMT_ADPCM,
} audio_format;

typedef struct {
    int16_t *buffer;
    uint32_t sample_count;
    uint32_t position;
    bool playing;
    bool looping;
    float volume;
    audio_format format;
} audio_track;

typedef struct {
    audio_track tracks[AUDIO_MAX_TRACKS];
    int16_t mix_buffer[AUDIO_BUFFER_SIZE];
    uint32_t sample_rate;
    bool initialized;
} audio_state;

void audio_init(audio_state *audio);
void audio_destroy(audio_state *audio);

int audio_open_track(audio_state *audio);
void audio_close_track(audio_state *audio, int track);
void audio_set_buffer(audio_state *audio, int track, int16_t *data, uint32_t samples);
void audio_play(audio_state *audio, int track);
void audio_stop(audio_state *audio, int track);
void audio_set_volume(audio_state *audio, int track, float volume);

void audio_mix_frame(audio_state *audio);
const int16_t *audio_get_output(audio_state *audio);

#endif
