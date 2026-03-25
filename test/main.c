#include <sgsound/openal.h>
#include <sgsound/openalBgm.h>
#include <sgforge/directory.h>
#include <sgforge/unpack.h>
#include <sgtools/log.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

const int cMaxSFXPlays = 100;
const char* cDatafile = "test.sg";
const char* cBgmFilename = "test.ogg";
const char* cSfxFilename = "testsfx.ogg";
const bool useStream = true;

static int numPlayedSfx = 0;
static int ticks = 0;
static void testBgmStream(const char* f, Directory* d) {
	char* soundData;
	size_t soundDataSize;
	int result = GetDataFromDirectory(f, &soundData, &soundDataSize, d);
	if (!result) {
		sgLogError("could not get data");
	}
	LoadBgmImpl("helo", soundData, soundDataSize, 1.0, -1);
}

static void playSfxStream(Directory* d) {
	char* soundData;
	size_t soundDataSize;
	int result = GetDataFromDirectory(cSfxFilename, &soundData, &soundDataSize, d);
	if (!result) {
		sgLogError("could not get data");
	}
	PlaySfxOneShotImpl(cSfxFilename, 1.0f, soundData, soundDataSize);
}

int main() {
	Directory* directory = sgDeserializeDirectoryFromFile(cDatafile);
	if (!directory) {
		sgLogCritical("borked\n");
	}
	InitializeAudioImpl();
	if (useStream) {
		testBgmStream(cBgmFilename, directory);
	}
	SetBgmTrackImpl(0);
	if (!useStream) LoadBgmFImpl(cBgmFilename, 1.0, -1);
	PlayBgmImpl();
	while (numPlayedSfx < cMaxSFXPlays) {
		AudioUpdateImpl();
		// 10ms
		usleep(10000);
		++ticks;
		if (ticks >= 60) {
			ticks = 0;
			++numPlayedSfx;
			if (useStream)
				playSfxStream(directory);
			else
				PlaySfxOneShotFImpl(cSfxFilename, 1.0f);
		}
	}
	sgFreeDirectory(directory);
	sgShutdownLogSystem();
	CloseAudioImpl();
	return true;
}
