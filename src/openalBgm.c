#include <AL/al.h>
#include <assert.h>
#include <ogg/ogg.h>
#include <sgsound/openalBgm.h>
#include <sgsound/openalMemoryStream.h>
#include <sgtools/log.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

Bgm* BgmNew(void) {
	Bgm* bgm = malloc(sizeof(*bgm));
	bgm->VorbisFile = malloc(sizeof(*bgm->VorbisFile));
	bgm->VorbisInfo = NULL;
	bgm->Filename = NULL;
	bgm->LoopStart = bgm->LoopEnd = bgm->Loops = 0;
	bgm->IsPlaying = false;
	bgm->Volume = 1.0f;
	return bgm;
}

static void getLoopPointsFromVorbisComments(Bgm* bgm, double* loopBegin, double* loopEnd) {
	assert(bgm && "No bgm!");
	vorbis_comment* vc = ov_comment(bgm->VorbisFile, -1);
	if (!vc) {
		sgLogError("Error retrieving vorbis comments for bgm %s , setting to 0", bgm->Filename);
		*loopBegin = *loopEnd = 0;
		return;
	}
	const char* start = "LOOPSTART=";
	const char* end = "LOOPEND=";
	for (int i = 0; i < vc->comments; ++i) {
		char* comment = vc->user_comments[i];

		if (strncasecmp(comment, start, strlen(start)) == 0) {
			float startTime = atof(comment + strlen(start));
			*loopBegin = startTime;
		} else if (strncasecmp(comment, end, strlen(end)) == 0) {
			float endTime = atof(comment + strlen(end));
			*loopEnd = endTime;
		}
	}
}

static void setBgmLoopPoints(Bgm* bgm) {
	assert(bgm && "No bgm!");
	double loopBegin = 0, loopEnd = 0;
	if (bgm->Loops != 0) {
		getLoopPointsFromVorbisComments(bgm, &loopBegin, &loopEnd);
	}
	bgm->LoopStart = loopBegin > 0 ? (int64_t)(loopBegin * bgm->VorbisInfo->rate)
								   : ov_pcm_tell(bgm->VorbisFile);
	// Loop end needs to be measured against our buffers loading,
	//	so they will be multiplied by channels and sizeof,Due to us checking
	// this on every step.
	loopEnd = loopBegin >= loopEnd ? 0 : loopEnd;
	if (loopEnd > 0) {
		int64_t samplesOffset = (int64_t)(loopEnd * bgm->VorbisInfo->rate);
		bgm->LoopEnd = samplesOffset * bgm->VorbisInfo->channels * sizeof(short);
	} else {
		bgm->LoopEnd = ov_pcm_total(bgm->VorbisFile, -1) * bgm->VorbisInfo->channels * sizeof(short);
	}
}

void BgmLoad(Bgm* bgm, const char* buffer, size_t bufferSize) {
	MemoryStream* stream = malloc(sizeof(MemoryStream));
	stream->data = buffer;
	stream->size = bufferSize;
	stream->pos = 0;
	int result = ov_open_callbacks(stream, bgm->VorbisFile, NULL, 0, callbacks);
	if (result != 0) {
		fprintf(stderr, "Could not open audio from memory: %d\n", result);
		free(stream);
		return;
	}
	bgm->VorbisInfo = ov_info(bgm->VorbisFile, -1);
	if (bgm->VorbisInfo->channels == 1) {
		bgm->Format = AL_FORMAT_MONO16;

	} else {
		bgm->Format = AL_FORMAT_STEREO16;
	}
	if (!bgm->Format) {
		sgLogError("Unsupported channel count: %d\n", bgm->VorbisInfo->channels);
		ov_clear(bgm->VorbisFile);
		free(stream);
		return;
	}
	setBgmLoopPoints(bgm);
}

void BgmLoadF(Bgm* bgm) {
	int result = ov_fopen(bgm->Filename, bgm->VorbisFile);
	if (result != 0) {
		fprintf(stderr, "Could not open audio in %s: %d", bgm->Filename, result);
		return;
	}
	bgm->VorbisInfo = ov_info(bgm->VorbisFile, -1);
	if (bgm->VorbisInfo->channels == 1) {
		bgm->Format = AL_FORMAT_MONO16;
	} else {
		bgm->Format = AL_FORMAT_STEREO16;
	}
	if (!bgm->Format) {
		fprintf(stderr, "Unsupported channel count: %d", bgm->VorbisInfo->channels);
		ov_clear(bgm->VorbisFile);
		return;
	}
	setBgmLoopPoints(bgm);
}

void BgmDelete(Bgm* bgm) {
	if (!bgm)
		return;
	if (bgm->VorbisInfo) {
		ov_clear(bgm->VorbisFile);
	}
	free(bgm->VorbisFile);
	free(bgm->Filename);
	free(bgm);
}
