#pragma once
#include "KcAudioDevice.h"
#include <memory>


class KcAudio;

// �Զ��ҽ����ڴ洢¼�����ݵ�observer
class KcAudioCapture : public KtObservable<void*, unsigned, double>
{
public:
    KcAudioCapture();

    // when deviceId = -1, use the defautl input device to capture audio
    // frameTime��ʾÿ֡��ʱ������λΪ�룬��<=0����ʹ���ϴ�¼�����ã���ʹ��ȱʡֵ
    bool record(std::shared_ptr<KcAudio>& audio, unsigned deviceId = -1, double frameTime = 0);

    // ֹͣ¼��
    bool stop(bool wait);

    // ��ͣ¼��
    bool pause(bool wait);

    // ����¼��
    bool goon(bool wait);

    // �Ƿ�����¼�ƣ�
    bool running() const;

    // �Ƿ���ͣ¼�ƣ�
    bool pausing() const;


    const char* error() const { return device_->error(); }

private:
    std::unique_ptr<KcAudioDevice> device_; // ¼���豸
    unsigned openedDevice_;
};
