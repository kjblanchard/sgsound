#include <AL/al.h>
#include <AL/alc.h>
#include <assert.h>
#include <sgsound/openalBgm.h>
#include <sgsound/openalStream.h>
#include <sgtools/log.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

typedef enum BufferLoadStatus {
	BufferLoadDefault,
	BufferLoadMusicEnded,
	BufferLoadMusicHitLoopPoint
} BufferLoadStatus;

static long loadBufferData(Stream* stream, BufferLoadStatus* bufferStatus);

Stream* StreamNew(void) {
	Stream* player;
	player = calloc(1, sizeof(*player));
	assert(player != NULL);
	alGenBuffers(BGM_NUM_BUFFERS, player->ALBuffers);
	ALenum result;
	result = alGetError();
	assert(result == AL_NO_ERROR && "Could not create buffers");
	alGenSources(1, &player->Source);
	result = alGetError();
	assert(result == AL_NO_ERROR && "Could not create source");
	alSource3f(player->Source, AL_POSITION, 0, 0, 0);
	result = alGetError();
	assert(result == AL_NO_ERROR && "Could not set source pos");
	alSourcei(player->Source, AL_SOURCE_RELATIVE, AL_TRUE);
	result = alGetError();
	assert(result == AL_NO_ERROR && "Could not set source relative");
	alSourcei(player->Source, AL_ROLLOFF_FACTOR, 0);
	result = alGetError();
	assert(result == AL_NO_ERROR && "Could not set source rolloff");
	size_t dataReadSize = (size_t)(BGM_BUFFER_SAMPLES);
	player->Buffer = malloc(dataReadSize);
	player->BgmData = NULL;
	player->IsPlaying = false;
	player->TotalBytesReadThisLoop = 0;
	return player;
}

static void preloadStreamWithData(Stream* stream) {
	ALsizei i;
	BufferLoadStatus bufferStatus;
	if (!stream->BgmData->VorbisInfo) {
		sgLogWarn("Trying to play a stream with no vorbis info");
		return;
	}
	for (i = 0; i < BGM_NUM_BUFFERS; i++) {
		long bytes_read = loadBufferData(stream, &bufferStatus);
		// Do not queue an empty buffer
		if (bytes_read <= 0) {
			return;
		}
		alBufferData(stream->ALBuffers[i], stream->BgmData->Format, stream->Buffer, (ALsizei)bytes_read, stream->BgmData->VorbisInfo->rate);
	}
	if (alGetError() != AL_NO_ERROR) {
		sgLogError("Error buffering for playback");
		return;
	}
	alSourceQueueBuffers(stream->Source, i, stream->ALBuffers);
	if (alGetError() != AL_NO_ERROR) {
		sgLogError("Error queuing buffers for playback");
	}
}

void StreamLoad(Stream* stream) {
	stream->TotalBytesReadThisLoop = 0;
	assert(stream->BgmData && "No bgm in stream load!");
	alSourceRewind(stream->Source);
	alSourcei(stream->Source, AL_BUFFER, 0);
	preloadStreamWithData(stream);
}

static void RestartStream(Stream* stream) {
	Bgm* bgm = stream->BgmData;
	if (!bgm || !bgm->VorbisInfo) return;
	ov_pcm_seek_lap(bgm->VorbisFile, bgm->LoopStart);
	stream->TotalBytesReadThisLoop = ov_pcm_tell(bgm->VorbisFile) * bgm->VorbisInfo->channels * sizeof(short);
}
static long loadBufferData(Stream* stream, BufferLoadStatus* bufferStatus) {
	// Set the buffer flags to 0, as it is normal
	*bufferStatus = 0;
	// Set the bytes read to 0, since we didn't read any bytes yet
	long total_buffer_bytes_read = 0;
	// Set the max request size to get data from the vorbis file
	int request_size = VORBIS_REQUEST_SIZE;
	// Our goal is to read enough bytes to fill up our buffer samples (8kbs), so
	// while we have read less than that, keep loading. This is due to vorbis
	// reading random amounts, and not the whole size at once.
	while (total_buffer_bytes_read < BGM_BUFFER_SAMPLES) {
		// Update the request size.  Remember our goal is to read the full buffer.
		request_size =
			(total_buffer_bytes_read + request_size <= BGM_BUFFER_SAMPLES)
				? request_size
				: BGM_BUFFER_SAMPLES - total_buffer_bytes_read;
		// Update the request size.  Remember we don't want to go past the loop end point
		request_size =
			(total_buffer_bytes_read + request_size +
				 stream->TotalBytesReadThisLoop <=
			 stream->BgmData->LoopEnd)
				? request_size
				: stream->BgmData->LoopEnd -
					  (total_buffer_bytes_read + stream->TotalBytesReadThisLoop);
		if (request_size == 0) {
			*bufferStatus = BufferLoadMusicHitLoopPoint;
			break;
			// We are at the end of the loop point.
		}
		// Actually read from the file.Notice we offset our memory location(membuf)
		// by the amount of bytes read so that we keep loading more.
		int current_pass_bytes_read =
			ov_read(stream->BgmData->VorbisFile,
					(char*)stream->Buffer + total_buffer_bytes_read, request_size,
					0, sizeof(short), 1, 0);
		// If we have read 0 bytes, we are at the end of the song.
		if (current_pass_bytes_read == 0) {
			// Set the buffer flags to ended
			*bufferStatus = BufferLoadMusicEnded;
			// Stop filling the buffer and exit the while loop
			break;
		}
		// Update the amount of bytes read for this buffer
		total_buffer_bytes_read += current_pass_bytes_read;
	}
	// Add the bytes read to the current bytes read for the entire loop, used for
	// tracking the current loading point.
	stream->TotalBytesReadThisLoop += total_buffer_bytes_read;
	return total_buffer_bytes_read;
}

static void handleProcessedBuffer(Stream* stream) {
	ALuint bufid;
	alSourceUnqueueBuffers(stream->Source, 1, &bufid);
	BufferLoadStatus bufferStatus = BufferLoadDefault;
	long bytes_read = loadBufferData(stream, &bufferStatus);
	// Do NOT queue an empty buffer
	if (bytes_read <= 0) return;
	alBufferData(bufid, stream->BgmData->Format, stream->Buffer,
				 (ALsizei)bytes_read, stream->BgmData->VorbisInfo->rate);
	alSourceQueueBuffers(stream->Source, 1, &bufid);
	if (alGetError() != AL_NO_ERROR) {
		sgLogError("Error buffering data");
		return;
	}
	if (bufferStatus == BufferLoadMusicEnded ||
		bufferStatus == BufferLoadMusicHitLoopPoint) {
		if (stream->BgmData->Loops) {
			RestartStream(stream);
			stream->BgmData->Loops = stream->BgmData->Loops == 255 ? 255 : stream->BgmData->Loops - 1;
		}
	}
	return;
}

void StreamUpdate(Stream* stream) {
	if (!stream->IsPlaying)
		return;
	ALint processed_buffers, state;
	alGetSourcei(stream->Source, AL_SOURCE_STATE, &state);
	alGetSourcei(stream->Source, AL_BUFFERS_PROCESSED, &processed_buffers);
	if (alGetError() != AL_NO_ERROR) {
		sgLogError("Error checking source state");
		return;
	}
	ALint queued;
	if (state == AL_STOPPED) {
		/* If no buffers are queued, playback is finished or starved */
		alGetSourcei(stream->Source, AL_BUFFERS_QUEUED, &queued);
		if (queued == 0)
			return;
	} else if (state == AL_PAUSED) {
		return;
	}
	while (processed_buffers > 0) {
		handleProcessedBuffer(stream);
		--processed_buffers;
	}
	if (state != AL_PLAYING && state != AL_PAUSED) {
		sgLogDebug("We are not playing OR paused, we are %d", state);
		alSourcePlay(stream->Source);
		if (alGetError() != AL_NO_ERROR) {
			sgLogError("Error restarting playback");
			return;
		}
	}
	return;
}

void StreamUpdateVolume(Stream* stream, float volume) {
	alSourcef(stream->Source, AL_GAIN, volume);
}

void StreamPlay(Stream* stream) {
	alSourcePlay(stream->Source);
	stream->IsPlaying = true;
	if (alGetError() != AL_NO_ERROR) {
		fprintf(stderr, "Error starting playback\n");
	}
}

void StreamStop(Stream* stream) {
	alSourceStop(stream->Source);
	RestartStream(stream);
	stream->IsPlaying = false;
}
