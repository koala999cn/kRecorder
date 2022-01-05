#pragma once
#include <QDialog>
#include <memory>
#include <vector>

class KcAudio;

namespace Ui {
class KcAudioCaptureDlg;
}

class KcAudioCapture;
class KcAudioRender;

class KcAudioCaptureDlg : public QDialog
{
    Q_OBJECT

public:
    explicit KcAudioCaptureDlg(QWidget *parent = nullptr);
    ~KcAudioCaptureDlg();

private slots:
    void on_btStart_clicked();
    void on_btPlay_clicked();
    void on_btStop_clicked();
    void on_btPause_clicked();
    void on_btOk_clicked();
    void on_btCancel_clicked();

    // 用于追踪回放是否结束的定时器
    void timerEvent( QTimerEvent *event) override;

private:
    void syncDeviceInfo_();

    enum { k_capture, k_play, k_pause, k_ready };
    void syncUiState_(int state);

public:
    std::shared_ptr<KcAudio> audio_; // 录制的音频

private:
    Ui::KcAudioCaptureDlg *ui; // 录制界面
    std::unique_ptr<KcAudioCapture> capture_; // 录制设备
    std::unique_ptr<KcAudioRender> render_; // 播放设备，用于试听
    int timerId_;
};


