#include "KcAudioRender.h"
#include "KcAudio.h"
#include <string.h>


namespace kPrivate
{
    class KcRenderObserver : public KcAudioDevice::observer_type
    {
    public:
        KcRenderObserver(KcAudioRender& render) : render_(render) {}

        bool update(void* outputBuffer, void* /*inputBuffer*/,
                    unsigned frames, double streamTime) override {

            double* buf = (double*)outputBuffer; // TODO:
            auto pos = audio_->sampling().xToHighIndex(streamTime);
            if(pos > audio_->count())
                pos = audio_->count();
            kIndex total = std::min<kIndex>(frames, audio_->count() - pos);

            audio_->getSamples(pos, buf, total);
            ::memset(buf + audio_->sizeOfSamples(total), 0, audio_->sizeOfSamples(frames - total));

            pos += total;

            return render_.notifyObservers(outputBuffer, frames, streamTime) &&
                    pos < audio_->count();
        }

        void reset() { audio_.reset(); }

        void reset(const std::shared_ptr<KcAudio>& audio) {
            audio_ = audio;
        }


    private:
        KcAudioRender& render_;
        std::shared_ptr<KcAudio> audio_;
    };
}


KcAudioRender::KcAudioRender()
{
    device_ = std::make_unique<KcAudioDevice>();
    device_->addObserver(std::make_shared<kPrivate::KcRenderObserver>(*this));
    openedDevice_ = -1;
}


bool KcAudioRender::playback(const std::shared_ptr<KcAudio>& audio, unsigned deviceId, double frameTime)
{
    assert(device_);

    if(deviceId == static_cast<unsigned>(-1))
        deviceId = device_->defaultOutput();

    // 检测参数一致性，不一致则重新打开设备
    if( deviceId != openedDevice_ ||
        device_->outputChannels() != audio->channels() ||
        device_->sampleRate() != audio->samplingRate() ||
        (frameTime > 0 && frameTime != device_->frameTime()) ) {

        if(device_->opened()) device_->close(true);
        if(frameTime <= 0) frameTime = 0.05; // 缺省帧长50ms

        KcAudioDevice::KpStreamParameters oParam;
        oParam.deviceId = deviceId;
        oParam.channels = audio->channels();
        unsigned bufferFrames = unsigned(audio->samplingRate() * frameTime + 0.5);

        if(!device_->open(&oParam, nullptr,
                          KcAudioDevice::k_float64,
                          audio->samplingRate(),
                          bufferFrames))
            return false;

        openedDevice_ = deviceId;
    }

    if(running()) stop(true);

    assert(device_->observers() == 1);
    auto obs = dynamic_cast<kPrivate::KcRenderObserver*>(device_->getObserver(0).get());
    assert(obs != nullptr);
    obs->reset(audio);

    device_->setTime(audio->xrange().first);
    return device_->start(false);
}


bool KcAudioRender::stop(bool wait)
{
    assert(running());

    auto obs = dynamic_cast<kPrivate::KcRenderObserver*>(device_->getObserver(0).get());
    assert(obs != nullptr);
    obs->reset();

    if (device_->stop(wait)) {
        openedDevice_ = -1;
        return true;
    }

    return false;
}


bool KcAudioRender::pause(bool wait)
{
    assert(running());
    return device_->stop(wait);
}


bool KcAudioRender::goon(bool wait)
{
    assert(pausing());
    return device_->start(wait);
}


bool KcAudioRender::running() const
{
    return device_->running();
}


bool KcAudioRender::pausing() const
{
    return !running() && openedDevice_ != -1;
}