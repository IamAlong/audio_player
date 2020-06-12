#pragma once

#include <memory>

#include "dogAudioImpl.h"

namespace dog{

	namespace audio{

		class DogAudio{
		public:
			DogAudio();
			~DogAudio();
			
			bool start();
			bool pause();
			bool stop();
			bool isFinish();
			void addData(const unsigned char* buffer, size_t bufferSize);
			void setAudioFormat(const DogAudioFormat& audioFormat);
		private:
			std::unique_ptr<DogAudioImpl> m_dogAudioImpl_;
		};

	} // namespace audio

} // namespace dog
