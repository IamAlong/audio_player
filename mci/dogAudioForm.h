#pragma once

namespace dog{

	namespace audio {

		enum class DogAudioType{
			PCM,
			WAV_MPEG
		};

		struct DogAudioFormat{
			int channel;
			int sampleRate;
			int sampleWidth;
			DogAudioType type;
			DogAudioFormat()
			: channel{ 1 },
			  sampleRate{ 16000 },
			  sampleWidth{ 16 },
			  type{ DogAudioType::PCM } {
			}
		};

	}

}