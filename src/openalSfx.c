#include <AL/al.h>
#include <AL/alc.h>
#include <sgsound/openalMemoryStream.h>
#include <assert.h>
#include <sgtools/log.h>
#include <stdbool.h>
#include <string.h>

#define MAX_SFX_STREAMS 8
#define MAX_SFX_CACHE 16
#define VORBIS_READ_SIZE 4096

static unsigned long sUsedCounter = 0;

typedef struct Sfx {
	ALenum Format;
	ALsizei Size;
	long SampleRate;
	short* Data;
} Sfx;

typedef struct {
	char* Name;
	Sfx SfxData;
	unsigned long LastUsed;
} SfxCacheEntry;

static SfxCacheEntry sSfxCache[MAX_SFX_CACHE];
static int sSfxCacheCount = 0;

static ALuint sALSfxSources[MAX_SFX_STREAMS];

static int findEvictionIndex(void) {
	unsigned long oldest = (unsigned long)-1;
	int oldestIndex = 0;
	for (int i = 0; i < sSfxCacheCount; i++) {
		if (sSfxCache[i].LastUsed < oldest) {
			oldest = sSfxCache[i].LastUsed;
			oldestIndex = i;
		}
	}
	return oldestIndex;
}

void InitializeSfxSystem(void) {
	alGenSources(MAX_SFX_STREAMS, sALSfxSources);
	assert(alGetError() == AL_NO_ERROR && "Could not create sound sources");
	for (int i = 0; i < MAX_SFX_STREAMS; ++i) {
		alSource3f(sALSfxSources[i], AL_POSITION, 0, 0, -1);
		alSourcei(sALSfxSources[i], AL_SOURCE_RELATIVE, AL_TRUE);
		alSourcei(sALSfxSources[i], AL_ROLLOFF_FACTOR, 0);
	}
}

static Sfx* findCachedSfx(const char* filename) {
	for (int i = 0; i < sSfxCacheCount; i++) {
		if (strcmp(sSfxCache[i].Name, filename) == 0) {
			sSfxCache[i].LastUsed = sUsedCounter++;
			return &sSfxCache[i].SfxData;
		}
	}
	return NULL;
}

static void addToCache(const char* filename, Sfx* sfx) {
	int index = sSfxCacheCount;
	if (sSfxCacheCount >= MAX_SFX_CACHE) {
		index = findEvictionIndex();
		free(sSfxCache[index].Name);
		free(sSfxCache[index].SfxData.Data);
	} else {
		sSfxCacheCount++;
	}

	SfxCacheEntry* entry = &sSfxCache[index];
	entry->Name = strdup(filename);
	entry->SfxData = *sfx;
}

static Sfx* loadSfx(const char* filename, char* buf, size_t sz) {
	Sfx* cached = findCachedSfx(filename);
	if (cached) return cached;
	bool result = false;
	OggVorbis_File vf;
	if (buf && sz) {
		MemoryStream* stream = malloc(sizeof(MemoryStream));
		stream->data = buf;
		stream->size = sz;
		stream->pos = 0;
		result = ov_open_callbacks(stream, &vf, NULL, 0, callbacks);
		if (result != 0) free(stream);
	} else {
		result = ov_fopen(filename, &vf);
	}
	if (result != 0) {
		sgLogError("Could not open audio: %d\n", result);
		return NULL;
	}
	Sfx newSfx = {0};
	vorbis_info* info;
	info = ov_info(&vf, -1);
	if (info->channels == 1)
		newSfx.Format = AL_FORMAT_MONO16;
	else if (info->channels == 2)
		newSfx.Format = AL_FORMAT_STEREO16;
	else {
		fprintf(stderr, "Unsupported channel count %d in '%s'", info->channels, filename);
		ov_clear(&vf);
		return NULL;
	}
	newSfx.SampleRate = info->rate;
	newSfx.Size = ov_pcm_total(&vf, -1) * info->channels * sizeof(short);
	newSfx.Data = malloc(newSfx.Size);
	long total = 0;
	while (total < newSfx.Size) {
		long bytes = ov_read(&vf, ((char*)newSfx.Data) + total, VORBIS_READ_SIZE, 0, 2, 1, NULL);
		if (bytes <= 0) break;
		total += bytes;
	}
	ov_clear(&vf);
	addToCache(filename, &newSfx);
	return findCachedSfx(filename);
}

static int getFreeSource(void) {
	ALint state;
	for (int i = 0; i < MAX_SFX_STREAMS; i++) {
		alGetSourcei(sALSfxSources[i], AL_SOURCE_STATE, &state);
		if (state != AL_PLAYING) return i;
	}
	sgLogWarn("Buffers full, not playing!");
	return -1;
}

static void playSfx(Sfx* snd, float volume) {
	int src = getFreeSource();
	if (src == -1) return;
	ALuint buffer;
	alGenBuffers(1, &buffer);
	alBufferData(buffer, snd->Format, snd->Data, snd->Size, snd->SampleRate);
	alSourceStop(sALSfxSources[src]);
	alSourceRewind(sALSfxSources[src]);
	alSourcei(sALSfxSources[src], AL_BUFFER, buffer);
	alSourcef(sALSfxSources[src], AL_GAIN, volume);
	alSourcePlay(sALSfxSources[src]);
}

void SfxPlayOneShotF(const char* filename, float volume) {
	Sfx* snd = loadSfx(filename, NULL, 0);
	if (!snd) return;
	playSfx(snd, volume);
}

void SfxPlayOneShot(const char* filename, float volume, char* buf, size_t sz) {
	Sfx* snd = loadSfx(filename, buf, sz);
	if (!snd) return;
	playSfx(snd, volume);
}
