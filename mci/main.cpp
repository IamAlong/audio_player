#include <iostream>
#include <string>
#include <fstream>
#include <ctime>
#include <algorithm>

#include "dogAudio.h"
#include "dogAudioForm.h"

using namespace std;
using namespace dog::audio;

int main() {
	ifstream fin("test.pcm", ios::binary);
	if (!fin.good()) {
		cout << "fin not good" << endl;
		return 1;
	}
	DogAudio dogAudio;
	char buffer[2048] = { 0 };
	while (!fin.eof()) {
		fin.read(buffer, 2048);
		int gount = fin.gcount();
		dogAudio.addData(reinterpret_cast<const unsigned char*>(buffer), fin.gcount());
		memset(buffer, 0, 2048);
	}
	
	getchar();
	dogAudio.stop();
	return 0;
}