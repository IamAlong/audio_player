#include <string>
#include <queue>
#include <xaudio2.h>
#include <Windows.h>

#include <iostream>
#include <mutex>

#include "dogAudioForm.h"

#pragma comment(lib, "xaudio2.lib")

namespace dog {

	namespace audio {

		class DogAudioImpl{
		private:
			class Callback : public IXAudio2VoiceCallback {
			public:
				HANDLE hBufferEndEvent;
				DogAudioImpl* dogAudioImpl;
				Callback()
					: hBufferEndEvent(CreateEvent(NULL, FALSE, FALSE, NULL)) {
				}
				~Callback() {
					CloseHandle(hBufferEndEvent);
				}
				// ���ݿ鲥����ص�
				void __stdcall OnBufferEnd(void * pBufferContext) override {
					XAUDIO2_VOICE_STATE state;
					dogAudioImpl->m_sourceVoice_->GetState(&state);
					dogAudioImpl->m_bufferCount_ = state.BuffersQueued;
					if (pBufferContext) {
						if (!dogAudioImpl->m_bufferCount_) {
							dogAudioImpl->m_finish_ = true;
						}
						std::lock_guard<std::mutex> lk(dogAudioImpl->m_mutex_);
						dogAudioImpl->popBufferFromQue();
						while (dogAudioImpl->m_bufferCount_ < 64 && !dogAudioImpl->m_waitingPlayBuffer_.empty()) {
							dogAudioImpl->m_playingBuffer_.push(dogAudioImpl->m_waitingPlayBuffer_.front());
							dogAudioImpl->m_sourceVoice_->SubmitSourceBuffer(&(dogAudioImpl->m_playingBuffer_.back()), nullptr);
							dogAudioImpl->m_waitingPlayBuffer_.pop();
							++dogAudioImpl->m_bufferCount_;
						}
						SetEvent(hBufferEndEvent);
					}
				}

				void __stdcall OnStreamEnd() override {
				}
				// ����Ҫ�Ļص�
				void __stdcall OnVoiceProcessingPassEnd() override {
				}
				void __stdcall OnVoiceProcessingPassStart(UINT32 SamplesRequired) {
				}
				// ȫ������
				void __stdcall OnBufferStart(void * pBufferContext) override {
				}
				void __stdcall OnLoopEnd(void * pBufferContext) override {
				}
				void __stdcall OnVoiceError(void * pBufferContext, HRESULT Error) override {
				}
			};

			bool init();
			bool resetBuffer();
			void popBufferFromQue();
			XAUDIO2_BUFFER makeXAudio2Buffer(const BYTE* pBuffer, size_t buffSize);
			WAVEFORMATEX makeWaveFormatEx(const DogAudioFormat& audioFormat);

		public:
			DogAudioImpl();
			~DogAudioImpl();

			bool startPlay();
			bool pausePlay();
			bool stopPlay();
			bool finish();
			size_t addDataToSourceVoice(const BYTE* pBuffer, size_t buffSize);

			// �����ͽ�ֹͣδ�������ݵĲ���
			void setAudioFormat(const DogAudioFormat& audioFormat);

		private:
			IXAudio2* m_xAudio_;
			IXAudio2MasteringVoice* m_masteringVoice_;
			IXAudio2SourceVoice* m_sourceVoice_;

			int m_bufferCount_;
			int m_processCount_;
			bool m_isSpeaking_;
			bool m_finish_;
			std::queue<XAUDIO2_BUFFER> m_playingBuffer_;
			std::queue<XAUDIO2_BUFFER> m_waitingPlayBuffer_;

			DogAudioFormat m_audioFormat_;
			Callback m_callback_;
			std::mutex m_mutex_;
		};

	} // namespace audio

} // namespace dog