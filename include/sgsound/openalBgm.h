#include <stdint.h>
#include <vorbis/vorbisfile.h>
typedef struct Bgm {
	char* Filename;
	int Loops;
	uint64_t LoopStart;
	uint64_t LoopEnd;
	int IsPlaying;
	OggVorbis_File* VorbisFile;
	vorbis_info* VorbisInfo;
	int Format;
	float Volume;
} Bgm;

Bgm* BgmNew(void);
void BgmLoadF(Bgm* bgm);
void BgmLoad(Bgm* bgm, const char* buffer, size_t bufferSize);
void BgmDelete(Bgm* bgm);
