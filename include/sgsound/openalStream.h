#pragma once
#include <stdint.h>
#define BGM_NUM_BUFFERS 4
#define MAX_SFX_SOUNDS 10
#define BGM_BUFFER_SAMPLES 8192	  // 8kb
// #define BGM_BUFFER_SAMPLES 16384	  // 8kb
#define VORBIS_REQUEST_SIZE 4096  // Max size to request from vorbis to load.
#ifdef __cplusplus
extern "C" {
#endif
typedef struct Bgm Bgm;

typedef struct Stream {
	unsigned int ALBuffers[BGM_NUM_BUFFERS];
	unsigned int Source;
	short *Buffer;
	unsigned int IsPlaying;
	uint64_t TotalBytesReadThisLoop;
	Bgm *BgmData;
} Stream;

Stream *StreamNew(void);
void StreamLoad(Stream *stream);
void StreamUpdateVolume(Stream *stream, float volume);
void StreamUpdate(Stream *stream);
void StreamPlay(Stream *stream);
void StreamStop(Stream *stream);

#ifdef __cplusplus
}
#endif
