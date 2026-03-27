#include <AL/al.h>
#include <AL/alc.h>
#include <assert.h>
#include <sgsound/openal.h>
#include <sgsound/openalBgm.h>
#include <sgsound/openalSfx.h>
#include <sgsound/openalStream.h>
#include <sgtools/tools.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <vorbis/vorbisfile.h>

#define MAX_STREAMS 2
static unsigned int sCurrentStreamID = 0;
static Stream* sStreams[MAX_STREAMS];
static float sGlobalBgmVolume = 1.0;

typedef struct BgmLoadArgs {
	char* Name;
	int Loops;
	int Track;
	float Volume;
} BgmLoadArgs;

typedef struct AudioBgmAsset {
	Bgm* BgmPtr;
	bool IsFading;
} AudioBgmAsset;

static AudioBgmAsset sBgmAssets[MAX_STREAMS];

static bool initializeAL(void) {
	const ALCchar* name;
	ALCdevice* device;
	ALCcontext* ctx;
	device = NULL;
	if (!device)
		device = alcOpenDevice(NULL);
	if (!device) {
		fprintf(stderr, "Could not open a device!\n");
		return false;
	}
	ctx = alcCreateContext(device, NULL);
	if (ctx == NULL || alcMakeContextCurrent(ctx) == ALC_FALSE) {
		if (ctx != NULL)
			alcDestroyContext(ctx);
		alcCloseDevice(device);
		fprintf(stderr, "Could not set a context!\n");
		return false;
	}
	name = NULL;
	if (alcIsExtensionPresent(device, "ALC_ENUMERATE_ALL_EXT"))
		name = alcGetString(device, ALC_ALL_DEVICES_SPECIFIER);
	if (!name || alcGetError(device) != AL_NO_ERROR)
		name = alcGetString(device, ALC_DEVICE_SPECIFIER);
	sgLogDebug("Opened %s", name);
	return true;
}

void InitializeAudioImpl(void) {
	bool result = initializeAL();
	if (!result) {
		sgLogCritical("Could not load audio system!");
	}
	for (int i = 0; i < MAX_STREAMS; ++i) {
		sStreams[i] = StreamNew();
	}
	InitializeSfxSystem();
}
void SetBgmTrackImpl(int track) {
	if (track < 0 || track >= MAX_STREAMS) {
		printf(
			"Track passed in is not available, track unchanged, please use "
			"between 0 and %d",
			MAX_STREAMS);
		return;
	}
	sCurrentStreamID = track;
}

static void loadBgmInternal(BgmLoadArgs* args, const char* fullPath, char* data, size_t dataSize) {
	AudioBgmAsset* bgmAsset = &sBgmAssets[args->Track];
	// If theres already a bgm in there, then we should delete it.
	if (bgmAsset->BgmPtr) {
		BgmDelete(bgmAsset->BgmPtr);
		bgmAsset->BgmPtr = NULL;
	}
	bgmAsset->BgmPtr = BgmNew();
	bgmAsset->BgmPtr->Filename = strdup(fullPath);
	bgmAsset->BgmPtr->Loops = args->Loops;
	bgmAsset->BgmPtr->Volume = args->Volume * sGlobalBgmVolume;
	BgmLoad(bgmAsset->BgmPtr, data, dataSize);
}

static void loadBgmInternalFile(BgmLoadArgs* args, const char* fullPath) {
	AudioBgmAsset* bgmAsset = &sBgmAssets[args->Track];
	// If theres already a bgm in there, then we should delete it.
	if (bgmAsset->BgmPtr) {
		BgmDelete(bgmAsset->BgmPtr);
		bgmAsset->BgmPtr = NULL;
	}
	bgmAsset->BgmPtr = BgmNew();
	bgmAsset->BgmPtr->Filename = strdup(fullPath);
	bgmAsset->BgmPtr->Loops = args->Loops;
	bgmAsset->BgmPtr->Volume = args->Volume * sGlobalBgmVolume;
	BgmLoadF(bgmAsset->BgmPtr);
}

void LoadBgmImpl(const char* filename, char* data, size_t dataSize, float volume, int loops) {
	AudioBgmAsset* bgmAsset = &sBgmAssets[sCurrentStreamID];
	if (bgmAsset->BgmPtr && bgmAsset->BgmPtr->Filename &&
		strcmp(bgmAsset->BgmPtr->Filename, filename) == 0) {
		return;
	}
	BgmLoadArgs args;
	args.Name = strdup(filename);
	args.Loops = loops;
	args.Track = sCurrentStreamID;
	args.Volume = volume;
	loadBgmInternal(&args, filename, data, dataSize);
	Stream* stream = sStreams[sCurrentStreamID];
	stream->BgmData = bgmAsset->BgmPtr;
	StreamLoad(stream);
	free(args.Name);
}

void LoadBgmFImpl(const char* filename, float volume, int loops) {
	AudioBgmAsset* bgmAsset = &sBgmAssets[sCurrentStreamID];
	if (bgmAsset->BgmPtr && bgmAsset->BgmPtr->Filename &&
		strcmp(bgmAsset->BgmPtr->Filename, filename) == 0) {
		return;
	}
	BgmLoadArgs args;
	args.Name = strdup(filename);
	args.Loops = loops;
	args.Track = sCurrentStreamID;
	args.Volume = volume;
	loadBgmInternalFile(&args, filename);
	Stream* stream = sStreams[sCurrentStreamID];
	stream->BgmData = bgmAsset->BgmPtr;
	StreamLoad(stream);
	free(args.Name);
}

void PlayBgmImpl(void) {
	UpdatePlayingBgmVolumeImpl();
	StreamPlay(sStreams[sCurrentStreamID]);
}

void PauseBgmImpl(void) {}

void StopBgmImpl(void) {
	if (!sStreams[sCurrentStreamID])
		return;
	StreamStop(sStreams[sCurrentStreamID]);
}

void UpdatePlayingBgmVolumeImpl(void) {
	for (size_t i = 0; i < MAX_STREAMS; i++) {
		if (!sBgmAssets[i].BgmPtr)
			continue;
		StreamUpdateVolume(sStreams[i],
						   sGlobalBgmVolume * sBgmAssets[i].BgmPtr->Volume);
	}
}

void SetGlobalBgmVolumeImpl(float volume) {
	sGlobalBgmVolume = volume;
	UpdatePlayingBgmVolumeImpl();
}

void SetGlobalSfxVolumeImpl(float volume) {}
void PlaySfxOneShotFImpl(const char* name, float volume) {
	SfxPlayOneShotF(name, volume);
}
void PlaySfxOneShotImpl(const char* name, float volume, char* buf, size_t sz) {
	SfxPlayOneShot(name, volume, buf, sz);
}

void CloseAudioImpl(void) {
	for (int i = 0; i < MAX_STREAMS; ++i) {
		AudioBgmAsset* asset = &sBgmAssets[i];
		BgmDelete(asset->BgmPtr);
	}
}

void AudioUpdateImpl(void) {
	for (int i = 0; i < MAX_STREAMS; ++i) {
		if (!sBgmAssets[i].BgmPtr)
			continue;
		StreamUpdate(sStreams[i]);
	}
}
