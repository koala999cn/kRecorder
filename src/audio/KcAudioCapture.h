#pragma once
#include "KcAudioDevice.h"
#include <memory>


class KcAudio;

// 自动挂接用于存储录音数据的observer
class KcAudioCapture : public KtObservable<void*, unsigned, double>
{
public:
    KcAudioCapture();

    // when deviceId = -1, use the defautl input device to capture audio
    // frameTime表示每帧的时长，单位为秒，若<=0，则使用上次录音设置，或使用缺省值
    bool record(std::shared_ptr<KcAudio>& audio, unsigned deviceId = -1, double frameTime = 0);

    // 停止录制
    bool stop(bool wait);

    // 暂停录制
    bool pause(bool wait);

    // 继续录制
    bool goon(bool wait);

    // 是否正在录制？
    bool running() const;

    // 是否暂停录制？
    bool pausing() const;


    const char* error() const { return device_->error(); }

private:
    std::unique_ptr<KcAudioDevice> device_; // 录音设备
    unsigned openedDevice_;
};
