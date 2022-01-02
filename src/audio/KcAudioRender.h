#pragma once
#include <memory>
#include "KcAudioDevice.h"

class KcAudio;

class KcAudioRender : public KtObservable<void*, unsigned, double>
{
public:
    KcAudioRender();

    bool playback(const std::shared_ptr<KcAudio>& audio, unsigned deviceId = -1, double frameTime = 0);

    bool stop(bool wait);

    bool pause(bool wait);

    bool goon(bool wait);

    // 是否正在播放
    bool running() const;

    // 是否暂停播放
    bool pausing() const;


    const char* error() const { return device_->error(); }


private:
    std::unique_ptr<KcAudioDevice> device_; // 播放设备
    unsigned openedDevice_;
};
