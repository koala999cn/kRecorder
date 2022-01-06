#include "KcAudioCapture.h"
#include "KcAudio.h"


namespace kPrivate
{
    class KcRecorderObserver : public KcAudioDevice::observer_type
    {
    public:

        KcRecorderObserver(KcAudioCapture& recorder) : recorder_(recorder){}

        bool update(void */*outputBuffer*/, void *inputBuffer,
                    unsigned frames, double streamTime) override {
            audio_->addSamples((kReal*)inputBuffer, frames);
            return recorder_.notifyObservers(inputBuffer, frames, streamTime);
        }


        void reset() { audio_.reset(); }


        void reset(const std::shared_ptr<KcAudio>& audio) {
            audio_ = audio;
        }

    private:
        KcAudioCapture& recorder_;
        std::shared_ptr<KcAudio> audio_; // 用来保存已录制的音频数据
    };
}


KcAudioCapture::KcAudioCapture()
{
    device_ = std::make_unique<KcAudioDevice>();
    device_->addObserver(std::make_shared<kPrivate::KcRecorderObserver>(*this));
    openedDevice_ = -1;
}


bool KcAudioCapture::record(std::shared_ptr<KcAudio>& audio, unsigned deviceId, double frameTime)
{
    assert(device_);

    if(deviceId == static_cast<unsigned>(-1))
        deviceId = device_->defaultInput();

    // 检测参数一致性，不一致则重新打开设备
    if( deviceId != openedDevice_ ||
        device_->inputChannels() != audio->channels() ||
        device_->sampleRate() != audio->samplingRate() ||
        (frameTime > 0 && frameTime != device_->frameTime()) ) {

        if(device_->opened()) device_->close(true);
        if(frameTime <= 0) frameTime = 0.05; // 缺省帧长50ms

        KcAudioDevice::KpStreamParameters iParam;
        iParam.deviceId = deviceId;
        iParam.channels = audio->channels();
        unsigned bufferFrames = unsigned(audio->samplingRate() * frameTime + 0.5);

        if(!device_->open(nullptr, &iParam,
                          KcAudioDevice::k_float64,
                          audio->samplingRate(),
                          bufferFrames))
            return false;

        openedDevice_ = deviceId;
    }

    if(running()) stop(true);

    assert(device_->observers() == 1);
    auto obs = dynamic_cast<kPrivate::KcRecorderObserver*>(device_->getObserver(0).get());
    assert(obs != nullptr);
    obs->reset(audio);

    device_->setTime(audio->xrange().first);
    return device_->start(false);
}


bool KcAudioCapture::stop(bool wait)
{ 
    assert(running() || pausing());

    if (running() && !device_->stop(wait))
        return false;

    auto obs = dynamic_cast<kPrivate::KcRecorderObserver*>(device_->getObserver(0).get());
    assert(obs != nullptr);
    obs->reset();

    openedDevice_ = -1;
    return true;
}


bool KcAudioCapture::pause(bool wait)
{
    assert(running());
    return device_->stop(wait);
}


bool KcAudioCapture::goon(bool wait) 
{ 
    assert(pausing());
    return device_->start(wait); 
}


bool KcAudioCapture::running() const
{
    return device_->running();
}


bool KcAudioCapture::pausing() const
{
    return !running() && openedDevice_ != -1;
}