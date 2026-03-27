#pragma once
#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

void SetBgmTrackImpl(int track);
// void LoadBgmImpl(const char *filename, float volume, int loops);
void LoadBgmImpl(const char* filename, char* data, size_t dataSize, float volume, int loops);
void LoadBgmFImpl(const char *filename, float volume, int loops);
void PlayBgmImpl(void);
void PauseBgmImpl(void);
void StopBgmImpl(void);
void UpdatePlayingBgmVolumeImpl(void);
void SetGlobalBgmVolumeImpl(float volume);
void SetGlobalSfxVolumeImpl(float volume);
void PlaySfxOneShotFImpl(const char *name, float volume);
void PlaySfxOneShotImpl(const char *name, float volume, char* buf, size_t sz);
void CloseAudioImpl(void);
void AudioUpdateImpl(void);
void InitializeAudioImpl(void);

#ifdef __cplusplus
}
#endif
