#include "KcAudioCaptureDlg.h"
#include "ui_audio_capture_dlg.h"
#include "audio/KcAudioDevice.h"
#include "audio/KcAudioRender.h"
#include "audio/KcAudioCapture.h"
#include "audio/KgAudioFile.h"
#include "audio/KcAudio.h"
#include "dsp/KtuMath.h"
#include "dsp/KtSampling.h"
#include <assert.h>
#include <memory>
#include <QFileDialog>
#include <QMessageBox>


namespace kPrivate
{
    // 该oberver用于监听每帧数据，并动态更新ui组件
    class KcWaveObserver : public KcAudioCapture::observer_type
    {
    public:
        KcWaveObserver(Ui::KcAudioCaptureDlg *ui, const std::shared_ptr<KcAudio>& audio)
            : ui_(ui), audio_(audio) {}

        bool update(void *data, unsigned frames, double streamTime) override {

            // 更新音量组件
            if(audio_->channels() == 1) { // 单声道
                auto volumn = KtuMath<kReal>::minmax((kReal*)data, frames*audio_->channels());
                ui_->wgVolumn->setVolumn(std::max(std::abs(volumn.first), std::abs(volumn.second)));
            }
            else { // 立体声
                assert(audio_->channels() == 2);
                kReal volLeft(0), volRight(0);
                const kReal* buf = (kReal*)data;
                for(unsigned i = 0; i < frames; i++, buf += 2) {
                    volLeft = std::max(volLeft, std::abs(buf[0]));
                    volRight = std::max(volRight, std::abs(buf[1]));
                    ui_->wgVolumn->setVolumn(volLeft, volRight);
                }
            }


            ui_->wgVolumn->update();

            // 更新计时组件
            ui_->lbTime->setText(QString("%1:%2:%3").
                                    arg(int(streamTime/60)%60, 2, 10, QChar('0')).
                                    arg(int(streamTime)%60, 2, 10, QChar('0')).
                                    arg(int(streamTime*1000)%1000, 3, 10, QChar('0'))
                                    );

            // 更新波形组件
            auto bars = ui_->wgWave->getBarCount();
            std::vector<std::pair<float, float>> ranges(bars);
            KtSampling<kReal> samp;
            samp.resetn(0, frames-1, bars, 0);
            for(int i = 0; i < bars; i++) {
                long x0 = std::ceil(samp.indexToX(i));
                long x1 = std::floor(samp.indexToX(i+1));
                assert(x0 < frames);
                if(x1 >= frames) x1 = frames - 1;
                const kReal* buf = (kReal*)data;
                buf += audio_->channels() * x0;
                auto r = KtuMath<kReal>::minmax(buf, audio_->channels() * (x1 - x0 + 1));
                assert(r.first >= -1 && r.first <= 1 && r.second >= -1 && r.second <= 1);
                ranges[i].first = r.first, ranges[i].second = r.second;
            }
            ui_->wgWave->setBarRanges(ranges);
            ui_->wgWave->update();

            return true;
        }


    private:
        Ui::KcAudioCaptureDlg *ui_;
        std::shared_ptr<KcAudio> audio_;
    };
}


KcAudioCaptureDlg::KcAudioCaptureDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::KcAudioCaptureDlg),
    timerId_(-1)
{
    ui->setupUi(this);
    audio_ = std::make_shared<KcAudio>();
    auto obs = std::make_shared<kPrivate::KcWaveObserver>(ui, audio_);

    capture_ = std::make_unique<KcAudioCapture>();
    capture_->addObserver(obs);

    render_ = std::make_unique<KcAudioRender>();
    render_->addObserver(obs);


    // 初始化设备列表
    KcAudioDevice ad;
    for(unsigned i = 0; i < ad.count(); i++) {
        auto info = ad.info(i);
        if(info.inputChannels > 0) {
            QVariant deviceId(i);
            ui->cbDeviceList->addItem(QString::fromLocal8Bit(info.name), deviceId);
        }
    }

    assert(ui->cbDeviceList->count() > 0);
    ui->cbDeviceList->setCurrentIndex(0);
    syncDeviceInfo_();


    // 连接信号处理槽
    connect(ui->cbDeviceList, QOverload<int>::of(&QComboBox::currentIndexChanged),
        [=](int index){
        assert(index == ui->cbDeviceList->currentIndex());
        syncDeviceInfo_();
    });

    // 界面美化：设置图标
    setWindowIcon(QApplication::style()->standardIcon((enum QStyle::StandardPixmap)0));
    setWindowTitle(tr("录音"));

    ui->btOk->setIcon(QApplication::style()->standardIcon(QStyle::SP_DialogSaveButton));
    ui->btCancel->setIcon(QApplication::style()->standardIcon(QStyle::SP_DialogCancelButton));
    ui->btStart->setIcon(QIcon(":/icon/record"));
    ui->btStop->setIcon(QIcon(":/icon/stop"));

    syncUiState_(k_ready);
}


KcAudioCaptureDlg::~KcAudioCaptureDlg()
{
    if(render_->running()) render_->stop(true);
    if(capture_->running()) capture_->stop(true);

    delete ui;
}


void KcAudioCaptureDlg::syncDeviceInfo_()
{
    KcAudioDevice ad;
    auto deviceId = ui->cbDeviceList->currentData().toInt();
    auto info = ad.info(deviceId);
    assert(QString::fromLocal8Bit(info.name) == ui->cbDeviceList->currentText());

    ui->cbRate->clear();
    for(unsigned i = 0; i < info.sampleRates.size(); i++)
        ui->cbRate->addItem(QString::number(info.sampleRates[i]));

    ui->rbMono->setChecked(true);
    ui->rbMono->setEnabled(info.inputChannels > 0);
    ui->rbStereo->setEnabled(info.inputChannels > 1);
}


void KcAudioCaptureDlg::syncUiState_(int state)
{
    ui->cbDeviceList->setEnabled(state == k_ready);
    ui->cbRate->setEnabled(state == k_ready);
    ui->rbMono->setEnabled(state == k_ready);
    ui->rbStereo->setEnabled(state == k_ready);

    ui->btOk->setEnabled(state == k_ready);
    ui->btCancel->setEnabled(state == k_ready);
    ui->btPlay->setEnabled(state != k_capture && audio_ && audio_->count() > 0);
    ui->btStart->setEnabled(state == k_ready);
    ui->btStop->setEnabled(state == k_capture || state == k_pause);
    ui->btPause->setEnabled(state == k_capture || state == k_pause);


    if(state == k_play) {
        ui->btPlay->setText("停止");
        ui->btPlay->setIcon(QIcon(":/icon/stop_play"));
    }
    else {
        ui->btPlay->setText("播放");
        ui->btPlay->setIcon(QIcon(":/icon/play"));
    }

    if(state == k_pause) {
        ui->btPause->setText("继续");
        ui->btPause->setIcon(QIcon(":/icon/goon"));
    }
    else {
        ui->btPause->setText("暂停");
        ui->btPause->setIcon(QIcon(":/icon/pause"));
    }
}


// 开始录音
void KcAudioCaptureDlg::on_btStart_clicked()
{
    ui->btStart->setDisabled(true); // 防止重复点击

    auto deviceId = ui->cbDeviceList->currentData().toInt();
    auto sampleRate = ui->cbRate->currentText().toInt();
    auto channels = 0;
    if(ui->rbMono->isChecked())
        channels = 1;
    else if(ui->rbStereo->isChecked())
        channels = 2;

    audio_->reset(1.0/sampleRate, channels);
    if(!capture_->record(audio_, deviceId)) {
        ui->btStart->setDisabled(false);
        messageBox_(capture_->error(), "错误");
        return;
    }

    syncUiState_(k_capture);
}


// 停止录音
void KcAudioCaptureDlg::on_btStop_clicked()
{
    assert(capture_ && capture_->running());
    if (capture_->stop(true)) 
        syncUiState_(k_ready);
}


// 暂停/继续录音
void KcAudioCaptureDlg::on_btPause_clicked()
{
    if(capture_->running()) { // 暂停
        if (capture_->pause(true)) 
            syncUiState_(k_pause);
    }
    else { // 继续
        if (capture_->goon(true)) 
            syncUiState_(k_capture);
    }
}


// 回放/停止
void KcAudioCaptureDlg::on_btPlay_clicked()
{
    if(render_->running()) { // 停止
        if (render_->stop(true)) {
            if (timerId_ != -1) {
                killTimer(timerId_);
                timerId_ = -1;
            }

            syncUiState_(capture_->pausing() ? k_pause : k_ready);
        }
    }
    else { // 回放
        if(render_->playback(audio_)) {
            syncUiState_(k_play);
            assert(timerId_ == -1);
            timerId_ = startTimer(100); // 100ms定时器
        }
    }
}


// 保存音频
void KcAudioCaptureDlg::on_btOk_clicked()
{
    // 构造filter
    QString filter;
    for (int i = 0; i < KgAudioFile::getSupportTypeCount(); i++) {
        QString x(KgAudioFile::getTypeDescription(i));
        x.append('(');
        auto exts = QString(KgAudioFile::getTypeExtension(i)).split('|');
        for (auto s : exts) {
            x.append("*.");
            x.append(s);
            x.append(' ');
        }

        x = x.trimmed();
        x.append(')');

        filter.append(x); 
        if(i != KgAudioFile::getSupportTypeCount() - 1)
            filter.append(";;");
    }

    auto path = QFileDialog::getSaveFileName(this, tr("保存录音"), "", filter);
    auto r = audio_->save(path.toLocal8Bit().data());
    if (!r.empty()) {
        messageBox_(r, "错误");
    }
}


void KcAudioCaptureDlg::on_btCancel_clicked()
{
    reject();
}


void KcAudioCaptureDlg::timerEvent( QTimerEvent *event)
{
    if(event->timerId() == timerId_) {
        if(!render_->running()) {
            killTimer(timerId_);
            timerId_ = -1;
            syncUiState_(capture_->pausing() ? k_pause : k_ready);
        }
    }
}


void KcAudioCaptureDlg::messageBox_(std::string msg, std::string title)
{
    QMessageBox box(this);
    box.setWindowTitle(QString::fromStdString(title));
    box.setText(QString::fromLocal8Bit(msg));
    box.setIcon(QMessageBox::Information);
    box.setStandardButtons(QMessageBox::Ok);
    box.exec();
}