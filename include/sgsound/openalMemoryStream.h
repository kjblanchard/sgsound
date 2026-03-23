#include <stdlib.h>
#include <vorbis/vorbisfile.h>

typedef struct {
	const char* data;
	size_t size;
	size_t pos;
} MemoryStream;

extern ov_callbacks callbacks;
