#pragma once
#include <QDialog>
#include <memory>
#include <vector>
#include "cmdline/cmdline.h"


namespace Ui {
class KcAudioCaptureDlg;
}

class KgAudioFile;
class KcAudioCapture;
class KcAudioRender;


class KcAudioCaptureDlg : public QDialog
{
    Q_OBJECT

public:
    explicit KcAudioCaptureDlg(const cmdline::parser& cp, QWidget *parent = nullptr);
    ~KcAudioCaptureDlg();

    bool start();
    bool stop();

private slots:
    void on_btStart_clicked();
    void on_btPlay_clicked();
    void on_btStop_clicked();
    void on_btPause_clicked();

    void on_btSelDir_clicked();
    void on_btOpenDir_clicked();

private:
    void syncDeviceInfo_();

    enum class kState { capture, play, pause, ready };
    void syncUiState_(kState state);

    // 用于追踪回放是否结束的定时器
    void timerEvent(QTimerEvent* event) override;

    QString getExtension_() const; // 获取录音格式的文件名后缀
    unsigned getSampleRate_(int quality) const; // 根据录音品质选择采样频率
    unsigned fitOpusRate(unsigned rate) const; // 优先返回当前录制设备支持的适配opus的采样频率

private:
    Ui::KcAudioCaptureDlg *ui; // 录制界面
    std::unique_ptr<KcAudioCapture> capture_; // 录制设备
    std::unique_ptr<KcAudioRender> render_; // 播放设备，用于试听
    std::shared_ptr<KgAudioFile> file_; // 录音保存的文件对象
    std::string filePath_; // 最近录音保存文件的路径
    int timerId_; // 定时器事件id
};


