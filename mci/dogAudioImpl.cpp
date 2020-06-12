#include "dogAudioImpl.h"

#include <thread>
#include <iostream>

using namespace dog::audio;

namespace {
	const int typeArray[2] = { WAVE_FORMAT_PCM, WAVE_FORMAT_MPEG };
}

bool DogAudioImpl::init() {
#ifndef _XBOX
	CoInitializeEx(nullptr, COINIT_MULTITHREADED);
#endif
	// 构建xAudio引擎
	if (FAILED(XAudio2Create(&m_xAudio_, 0, XAUDIO2_DEFAULT_PROCESSOR)) || !m_xAudio_) {
		return false;
	}
	// 构建主音
	if (FAILED(m_xAudio_->CreateMasteringVoice(&m_masteringVoice_)) || !m_masteringVoice_) {
		return false;
	}
	// 构建源音
	WAVEFORMATEX format = makeWaveFormatEx(m_audioFormat_);
	if (FAILED(m_xAudio_->CreateSourceVoice(&m_sourceVoice_, &format, 0,
		XAUDIO2_DEFAULT_FREQ_RATIO, &m_callback_)) || !m_sourceVoice_) {
		return false;
	}
	return true;
}

bool DogAudioImpl::resetBuffer() {
	try {
		std::lock_guard<std::mutex> ls(m_mutex_);
		while (!m_playingBuffer_.empty()) {
			if (m_playingBuffer_.front().pAudioData) {
				delete[] m_playingBuffer_.front().pAudioData;
				m_playingBuffer_.front().pAudioData = nullptr;
			}
			if (m_playingBuffer_.front().pContext) {
				delete m_playingBuffer_.front().pContext;
				m_playingBuffer_.front().pContext = nullptr;
			}
			m_playingBuffer_.pop();
		}
		while (!m_waitingPlayBuffer_.empty()) {
			if (m_waitingPlayBuffer_.front().pAudioData) {
				delete[] m_waitingPlayBuffer_.front().pAudioData;
				m_waitingPlayBuffer_.front().pAudioData = nullptr;
			}
			if (m_waitingPlayBuffer_.front().pContext) {
				delete m_waitingPlayBuffer_.front().pContext;
				m_waitingPlayBuffer_.front().pContext = nullptr;
			}
			m_waitingPlayBuffer_.pop();
		}
	}
	catch (...) {
		return false;
	}
	return true;
}

XAUDIO2_BUFFER DogAudioImpl::makeXAudio2Buffer(const BYTE* pBuffer, size_t buffSize) {
	XAUDIO2_BUFFER xAudio2Buffer;
	xAudio2Buffer.Flags = 0;
	xAudio2Buffer.AudioBytes = buffSize;
	xAudio2Buffer.pAudioData = pBuffer;
	xAudio2Buffer.PlayBegin = 0;
	xAudio2Buffer.PlayLength = 0;
	xAudio2Buffer.LoopBegin = 0;
	xAudio2Buffer.LoopLength = 0;
	xAudio2Buffer.LoopCount = 0;
	xAudio2Buffer.pContext = new int[1]; // 表示数据块，供回调用
	return xAudio2Buffer;
}

WAVEFORMATEX DogAudioImpl::makeWaveFormatEx(const DogAudioFormat& audioFormat) {
	WAVEFORMATEX format;
	format.wFormatTag = typeArray[static_cast<int>(audioFormat.type)];
	format.wBitsPerSample = audioFormat.sampleWidth;
	format.nChannels = audioFormat.channel;
	format.nSamplesPerSec = audioFormat.sampleRate;
	format.nBlockAlign = audioFormat.sampleWidth * audioFormat.channel / 8;
	format.nAvgBytesPerSec = format.nBlockAlign * audioFormat.sampleRate;
	format.cbSize = 0;
	return format;
}

DogAudioImpl::DogAudioImpl()
	: m_isSpeaking_{ false },
	  m_bufferCount_{ 0 },
	  m_processCount_{ 0 },
	  m_mutex_{},
	  m_finish_ { false },
	  m_playingBuffer_{ std::queue<XAUDIO2_BUFFER>() },
	  m_waitingPlayBuffer_{ std::queue<XAUDIO2_BUFFER>() },
	  m_audioFormat_{ DogAudioFormat() } {
	m_callback_.dogAudioImpl = this;
}

DogAudioImpl::~DogAudioImpl() {
	if (m_isSpeaking_) {
		stopPlay();
	}
	if (m_sourceVoice_) {
		m_sourceVoice_->DestroyVoice();
		m_sourceVoice_ = nullptr;
	}
	if (m_masteringVoice_) {
		(m_masteringVoice_)->DestroyVoice();
		m_masteringVoice_ = nullptr;
	}
	if (m_xAudio_) {
		(m_xAudio_)->StopEngine();
		m_xAudio_ = nullptr;
	}
	resetBuffer();
}

bool DogAudioImpl::startPlay() {
	if (!init()) {
		if (m_xAudio_ == nullptr) {
			return false;
		}
		m_xAudio_ = nullptr;
		m_masteringVoice_ = nullptr;
		m_sourceVoice_ = nullptr;
	}
	if (!m_sourceVoice_) {
		return false;
	}
	if (!m_isSpeaking_) {
		if (FAILED(m_sourceVoice_->Start(0, XAUDIO2_COMMIT_NOW))) {
			return false;
		}
	}
	m_isSpeaking_ = true;
	return true;
}

bool DogAudioImpl::pausePlay() {
	if (!m_sourceVoice_) {
		return false;
	}
	if (m_isSpeaking_) {
		if (FAILED(m_sourceVoice_->Stop(0, XAUDIO2_COMMIT_NOW))) {
			return false;
		}
		m_isSpeaking_ = false;
	}
	return true;
}

bool DogAudioImpl::stopPlay() {
	if (!m_sourceVoice_) {
		return false;
	}
	if (m_isSpeaking_) {
		XAUDIO2_VOICE_STATE state;
		m_sourceVoice_->GetState(&state);
		while (state.BuffersQueued) {
			if (FAILED(m_sourceVoice_->Stop(0, XAUDIO2_COMMIT_NOW))) {
				return false;
			}
			if (FAILED(m_sourceVoice_->FlushSourceBuffers())) {
				return false;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
			m_sourceVoice_->GetState(&state);
		}
		if (!resetBuffer()) {
			return false;
		}
		m_isSpeaking_ = false;
	}
	return true;
}

size_t DogAudioImpl::addDataToSourceVoice(const BYTE* pBuffer, size_t buffSize) {
	if (m_finish_) {
		m_finish_ = false;
	}
	if (!m_isSpeaking_) {
		if (!startPlay()) {
			return 0;
		}
	}

	BYTE* buffer = new BYTE[buffSize];
	memcpy(buffer, pBuffer, buffSize);
	XAUDIO2_BUFFER audioBuffer = makeXAudio2Buffer(buffer, buffSize);
	*(int*)(audioBuffer.pContext) = m_processCount_;
	m_callback_.OnBufferEnd(nullptr);
	std::lock_guard<std::mutex> ls(m_mutex_);
	if (!m_waitingPlayBuffer_.empty()) {
		while (m_bufferCount_ < 64) {
			m_playingBuffer_.push(m_waitingPlayBuffer_.front());
			m_sourceVoice_->SubmitSourceBuffer(&m_playingBuffer_.back(), nullptr);
			m_waitingPlayBuffer_.pop();
			++m_bufferCount_;
		}
	}
	if (m_bufferCount_ >= 64) {
		m_waitingPlayBuffer_.push(audioBuffer);
	}
	else {
		m_playingBuffer_.push(audioBuffer);
		m_sourceVoice_->SubmitSourceBuffer(&m_playingBuffer_.back(), nullptr);
	}
	++m_processCount_;
	return ++m_bufferCount_;
}

void DogAudioImpl::popBufferFromQue() {
	if (!m_playingBuffer_.empty()) {
		if (m_playingBuffer_.front().pAudioData) {
			delete[] m_playingBuffer_.front().pAudioData;
			m_playingBuffer_.front().pAudioData = nullptr;
		}
		if (!m_playingBuffer_.front().pContext) {
			delete m_playingBuffer_.front().pContext;
			m_playingBuffer_.front().pContext = nullptr;
		}
		m_playingBuffer_.front().AudioBytes = 0;
		m_playingBuffer_.pop();
	}
}

void DogAudioImpl::setAudioFormat(const DogAudioFormat& audioFormat) {
	m_audioFormat_ = std::move(audioFormat);
	stopPlay();
	if (m_sourceVoice_) {
		m_sourceVoice_->DestroyVoice();
		m_sourceVoice_ = nullptr;
	}
	WAVEFORMATEX format = makeWaveFormatEx(m_audioFormat_);
	m_xAudio_->CreateSourceVoice(&m_sourceVoice_, &format, 0, XAUDIO2_DEFAULT_FREQ_RATIO, &m_callback_);
}

bool DogAudioImpl::finish() {
	return m_finish_;
}