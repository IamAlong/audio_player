#include "dogAudio.h"

#include <iostream>

using namespace dog::audio;

DogAudio::DogAudio()
	: m_dogAudioImpl_{ new DogAudioImpl } {
}

DogAudio::~DogAudio() {
}

bool DogAudio::start() {
	return m_dogAudioImpl_->startPlay();
}

bool DogAudio::pause() {
	return m_dogAudioImpl_->pausePlay();
}

bool DogAudio::stop() {
	return m_dogAudioImpl_->stopPlay();
}

bool DogAudio::isFinish() {
	return m_dogAudioImpl_->finish();
}

void DogAudio::addData(const unsigned char* buffer, size_t bufferSize) {
	m_dogAudioImpl_->addDataToSourceVoice(buffer, bufferSize);
}

void DogAudio::setAudioFormat(const DogAudioFormat& audioFormat) {
	m_dogAudioImpl_->setAudioFormat(audioFormat);
}