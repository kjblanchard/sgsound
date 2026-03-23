#include <sgsound/openalMemoryStream.h>
#include <ogg/ogg.h>
#include <stdio.h>
#include <string.h>
#include <vorbis/vorbisfile.h>

//save this was original
/* size_t mem_read(void* ptr, size_t size, size_t nmemb, void* datasource) { */
/* 	MemoryStream* mem = (MemoryStream*)datasource; */
/* 	size_t bytes = size * nmemb; */
/* 	if (mem->pos + bytes > mem->size) */
/* 		bytes = mem->size - mem->pos; */
/* 	memcpy(ptr, mem->data + mem->pos, bytes); */
/* 	mem->pos += bytes; */
/* 	return bytes / size; */
/* } */

size_t mem_read(void* ptr, size_t size, size_t nmemb, void* datasource) {
	MemoryStream* mem = (MemoryStream*)datasource;
	size_t bytesRequested = size * nmemb;
	size_t bytesRemaining = mem->size - mem->pos;
	size_t bytesToCopy = bytesRequested;
	if (bytesToCopy > bytesRemaining) bytesToCopy = bytesRemaining;
	memcpy(ptr, mem->data + mem->pos, bytesToCopy);
	mem->pos += bytesToCopy;
	return bytesToCopy / size;
}

int mem_seek(void* datasource, ogg_int64_t offset, int whence) {
	MemoryStream* mem = (MemoryStream*)datasource;
	size_t newpos;
	switch (whence) {
		case SEEK_SET:
			newpos = offset;
			break;
		case SEEK_CUR:
			newpos = mem->pos + offset;
			break;
		case SEEK_END:
			newpos = mem->size + offset;
			break;
		default:
			return -1;
	}
	if (newpos > mem->size)
		return -1;
	mem->pos = newpos;
	return 0;
}

long mem_tell(void* datasource) {
	MemoryStream* mem = (MemoryStream*)datasource;
	return mem->pos;
}

int mem_close(void* datasource) {
	MemoryStream* mem = (MemoryStream*)datasource;
	free(mem);
	return 0;
}

ov_callbacks callbacks = {
	mem_read,
	mem_seek,
	mem_close,
	mem_tell};
// End callbacks
