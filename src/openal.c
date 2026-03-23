#include <AL/al.h>
#include <AL/alc.h>
#include <sgsound/openal.h>
#include <sgsound/openalBgm.h>
#include <sgsound/openalSfx.h>
#include <sgsound/openalStream.h>
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <vorbis/vorbisfile.h>

#define MAX_STREAMS 2
static unsigned int _currentStreamID = 0;
Stream* _streams[MAX_STREAMS];

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

float _globalBgmVolume = 1.0;

AudioBgmAsset _bgmAssets[MAX_STREAMS];

#ifdef _WIN32
#define strncasecmp(x, y, z) _strnicmp(x, y, z)
#endif

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
	printf("Opened %s", name);
	return true;
}

void InitializeAudioImpl(void) {
	bool result = initializeAL();
	if (!result) {
		fprintf(stderr, "Could not load audio system!");
	}
	for (int i = 0; i < MAX_STREAMS; ++i) {
		_streams[i] = StreamNew();
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
	_currentStreamID = track;
}

/* static void loadBgmInternal(BgmLoadArgs* args, const char* fullPath) { */
static void loadBgmInternal(BgmLoadArgs* args, const char* fullPath, char* data, size_t dataSize) {
	AudioBgmAsset* bgmAsset = &_bgmAssets[args->Track];
	// If theres already a bgm in there, then we should delete it.
	if (bgmAsset->BgmPtr) {
		BgmDelete(bgmAsset->BgmPtr);
		bgmAsset->BgmPtr = NULL;
	}
	bgmAsset->BgmPtr = BgmNew();
	bgmAsset->BgmPtr->Filename = strdup(fullPath);
	bgmAsset->BgmPtr->Loops = args->Loops;
	bgmAsset->BgmPtr->Volume = args->Volume * _globalBgmVolume;
	BgmLoad(bgmAsset->BgmPtr, data, dataSize);
}

static void loadBgmInternalFile(BgmLoadArgs* args, const char* fullPath) {
	AudioBgmAsset* bgmAsset = &_bgmAssets[args->Track];
	// If theres already a bgm in there, then we should delete it.
	if (bgmAsset->BgmPtr) {
		BgmDelete(bgmAsset->BgmPtr);
		bgmAsset->BgmPtr = NULL;
	}
	// End get bytes
	bgmAsset->BgmPtr = BgmNew();
	bgmAsset->BgmPtr->Filename = strdup(fullPath);
	bgmAsset->BgmPtr->Loops = args->Loops;
	bgmAsset->BgmPtr->Volume = args->Volume * _globalBgmVolume;
	BgmLoadF(bgmAsset->BgmPtr);
}

void LoadBgmImpl(const char* filename, char* data, size_t dataSize, float volume, int loops) {
	AudioBgmAsset* bgmAsset = &_bgmAssets[_currentStreamID];
	if (bgmAsset->BgmPtr && bgmAsset->BgmPtr->Filename &&
		strcmp(bgmAsset->BgmPtr->Filename, filename) == 0) {
		return;
	}
	BgmLoadArgs args;
	args.Name = strdup(filename);
	args.Loops = loops;
	args.Track = _currentStreamID;
	args.Volume = volume;
	loadBgmInternal(&args, filename, data, dataSize);
	Stream* stream = _streams[_currentStreamID];
	stream->BgmData = bgmAsset->BgmPtr;
	StreamLoad(stream);
	free(args.Name);
}

void LoadBgmFImpl(const char* filename, float volume, int loops) {
	AudioBgmAsset* bgmAsset = &_bgmAssets[_currentStreamID];
	if (bgmAsset->BgmPtr && bgmAsset->BgmPtr->Filename &&
		strcmp(bgmAsset->BgmPtr->Filename, filename) == 0) {
		return;
	}
	BgmLoadArgs args;
	args.Name = strdup(filename);
	args.Loops = loops;
	args.Track = _currentStreamID;
	args.Volume = volume;
	loadBgmInternalFile(&args, filename);
	Stream* stream = _streams[_currentStreamID];
	stream->BgmData = bgmAsset->BgmPtr;
	StreamLoad(stream);
	free(args.Name);
}

void PlayBgmImpl(void) {
	UpdatePlayingBgmVolumeImpl();
	StreamPlay(_streams[_currentStreamID]);
}

void PauseBgmImpl(void) {}

void StopBgmImpl(void) {
	if (!_streams[_currentStreamID])
		return;
	StreamStop(_streams[_currentStreamID]);
}

void StopBgmFadeoutImpl(void) {}
void UpdatePlayingBgmVolumeImpl(void) {
	for (size_t i = 0; i < MAX_STREAMS; i++) {
		if (!_bgmAssets[i].BgmPtr)
			continue;
		StreamUpdateVolume(_streams[i],
						   _globalBgmVolume * _bgmAssets[i].BgmPtr->Volume);
	}
}
void SetGlobalBgmVolumeImpl(float volume) {
	_globalBgmVolume = volume;
	UpdatePlayingBgmVolumeImpl();
}
void SetGlobalSfxVolumeImpl(float volume) {}
void PlaySfxOneShotFImpl(const char* name, float volume) {
	SfxPlayOneShotF(name, volume);
}
void PlaySfxOneShotImpl(const char* name, float volume, char* buf, size_t sz) {
	SfxPlayOneShot(name, volume, buf, sz);
}
void AudioEventHandlerImpl(void* event) {}
void CloseAudioImpl(void) {
	for (int i = 0; i < MAX_STREAMS; ++i) {
		AudioBgmAsset* asset = &_bgmAssets[i];
		BgmDelete(asset->BgmPtr);
	}
}
void AudioUpdateImpl(void) {
	for (int i = 0; i < MAX_STREAMS; ++i) {
		if (!_bgmAssets[i].BgmPtr)
			continue;
		StreamUpdate(_streams[i]);
	}
}
