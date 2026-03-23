#pragma once
#include <stdlib.h>

void SfxPlayOneShotF(const char* filename, float volume);
void SfxPlayOneShot(const char* filename, float volume, char* buf, size_t sz);
void InitializeSfxSystem(void);
