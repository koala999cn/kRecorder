#include "KcAudioCaptureDlg.h"
#include "ui_audio_capture_dlg.h"
#include <assert.h>
#include <memory>
#include <QFileDialog>
#include <QMessageBox>
#include <QDesktopServices>
#include <QTimer>
#include "KcAudioDevice.h"
#include "KcAudioRender.h"
#include "KcAudioCapture.h"
#include "KgAudioFile.h"
#include "KcAudio.h"
#include "KtuMath.h"
#include "KtSampling.h"
#include "gui/QtAudioUtils.h"
#include "KgAudioFile.h"


namespace kPrivate
{
    static const double k_frameTime = 0.1;

    // 该oberver用于监听每帧数据，并动态更新ui组件
    class KcWaveObserver : public KcAudioCapture::observer_type
    {
    public:
        KcWaveObserver(Ui::KcAudioCaptureDlg *ui) : ui_(ui) {}

        bool update(void *data, unsigned frames, double streamTime) override {

            // 更新音量组件
            bool mono = ui_->rbMono->isChecked();
            if(mono) { // 单声道
                auto volumn = KtuMath<kReal>::minmax((kReal*)data, frames);
                ui_->wgVolumn->setVolumn(std::max(std::abs(volumn.first), std::abs(volumn.second)));
            }
            else { // 立体声
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
            ui_->lbTime->setText(QString(u8"%1:%2:%3").
                                    arg(int(streamTime/60)%60, 2, 10, QChar('0')).
                                    arg(int(streamTime)%60, 2, 10, QChar('0')).
                                    arg(int(streamTime*1000)%1000, 3, 10, QChar('0'))
                                    );
            ui_->lbTime->update();

            // 更新波形组件
            auto bars = ui_->wgWave->getBarCount();
            std::vector<std::pair<float, float>> ranges(bars);
            KtSampling<kReal> samp;
            samp.resetn(0, frames - 1, bars, 0);
            int channels = mono ? 1 : 2;
            for (int i = 0; i < bars; i++) {
                long x0 = std::ceil(samp.indexToX(i));
                long x1 = std::floor(samp.indexToX(i + 1));
                assert(x0 < frames);
                if (x1 >= frames) x1 = frames - 1;
                const kReal* buf = (kReal*)data;
                buf += channels * x0;
                auto r = KtuMath<kReal>::minmax(buf, channels * (x1 - x0 + 1));
                ranges[i].first = r.first, ranges[i].second = r.second;
            }
            ui_->wgWave->setBarRanges(ranges);
            ui_->wgWave->update();

            return true;
        }

    private:
        Ui::KcAudioCaptureDlg *ui_;
    };
}


KcAudioCaptureDlg::KcAudioCaptureDlg(const cmdline::parser& cp, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::KcAudioCaptureDlg),
    timerId_(-1)
{
    ui->setupUi(this);
    
    // 创建用来更新音量和录音/放音时间控件的观察者
    auto obs = std::make_shared<kPrivate::KcWaveObserver>(ui);

    capture_ = std::make_unique<KcAudioCapture>();
    capture_->pushBack(obs);

    render_ = std::make_unique<KcAudioRender>();
    render_->pushBack(obs); // 与录音共同一个观察者

    // 初始化设备列表cbDeviceList
    KcAudioDevice ad;
    auto defaultIn = ad.defaultInput();
    for (unsigned i = 0; i < ad.count(); i++) {
        auto info = ad.info(i);
        if (info.inputChannels > 0) {
            QVariant deviceId(i);
            ui->cbDeviceList->addItem(QString::fromLocal8Bit(info.name), deviceId);
            if (defaultIn == i)
                defaultIn = ui->cbDeviceList->count() - 1;
        }
    }

    assert(ui->cbDeviceList->count() > 0);
    ui->cbDeviceList->setCurrentIndex(defaultIn);
    syncDeviceInfo_(); // 根据输入设备同步初始化采样频率

    // 连接信号处理槽
    connect(ui->cbDeviceList, QOverload<int>::of(&QComboBox::currentIndexChanged),
        [=](int index) {
            assert(index == ui->cbDeviceList->currentIndex());
            syncDeviceInfo_();
        });

    // 设置采样频率
    auto rate = cp.get<int>("rate");
    auto idx = ui->cbRate->findText(QString::number(rate));
    if (idx < 0) idx = 0;
    ui->cbRate->setCurrentIndex(idx);


    // 初始化录音保存路径
    auto path = QString::fromStdString(cp.get<std::string>("path"));
    if (path.isEmpty() || !QDir(path).exists())
        path = QCoreApplication::applicationDirPath();
    ui->lbSaveDir->setText(path);

    // 初始化录音品质
    auto quality = cp.get<int>("quality");
    if (quality < 0) quality = 0;
    if (quality >= ui->cbQuality->count()) quality = ui->cbQuality->count() - 1;
    ui->cbQuality->setCurrentIndex(quality);

    // 初始化录音格式
    auto format = cp.get<std::string>("format");
    if (format == "opus")
        ui->cbFormat->setCurrentIndex(1);
    else if (format == "flac")
        ui->cbFormat->setCurrentIndex(2);
    else if (format == "wav")
        ui->cbFormat->setCurrentIndex(3);
    else
        ui->cbFormat->setCurrentIndex(0);

    // 单声道/立体声？
    if (cp.exist("stereo"))
        ui->rbStereo->setChecked(true);
    else
        ui->rbMono->setChecked(true);


    // 界面美化：设置图标
    setWindowIcon(QApplication::style()->standardIcon((enum QStyle::StandardPixmap)0));

    ui->btStart->setIcon(QIcon(u8":/kRecorder/record"));
    ui->btStop->setIcon(QIcon(u8":/kRecorder/stop"));
    ui->btSelDir->setIcon(QApplication::style()->standardIcon(QStyle::SP_DirIcon));
    ui->btOpenDir->setIcon(QApplication::style()->standardIcon(QStyle::SP_DialogOpenButton));

    syncUiState_(kState::ready);
}


KcAudioCaptureDlg::~KcAudioCaptureDlg()
{
    if(render_->running()) render_->stop(true);
    if(capture_->running()) capture_->stop(true);

    delete ui;
}


bool KcAudioCaptureDlg::start()
{
    assert(!capture_->running());

    ui->btStart->setDisabled(true); // 防止重复点击
    ui->btStart->repaint();

    auto deviceId = ui->cbDeviceList->currentData().toInt(); 
    auto channels = ui->rbStereo->isChecked() ? 2 : 1;
    int quality = ui->cbQuality->currentIndex();
    unsigned sampleRate = 0;
    if(ui->cbRate->currentIndex() > 0) // 首个选项表示自动选择频率
        sampleRate = ui->cbRate->currentText().toInt();
    if (sampleRate == 0) {
        sampleRate = getSampleRate_(quality); // 根据录音品质选择采样频率

        if (getExtension_() == "opus")
            sampleRate = fitOpusRate(sampleRate);
    }

    // 构造录音保存路径
    auto fileTime = QDateTime::currentDateTime();
    auto fileName = ui->lbSaveDir->text() + "/" + fileTime.toString("yyyy_MM_dd_hh_mm_ss");
    fileName += "." + getExtension_();
    file_ = std::make_shared<KgAudioFile>(channels, sampleRate, 0);
    if (!file_->open(fileName.toLocal8Bit().constData(), quality + 1)) {
        if (isVisible())
            QMessageBox::information(this, u8"错误", QString::fromLocal8Bit(file_->errorText()));
        ui->btStart->setDisabled(false);
        return false;
    }

    if (!capture_->record(file_, deviceId, kPrivate::k_frameTime)) {
        ui->btStart->setDisabled(false);
        if(isVisible())
            QMessageBox::information(this, u8"错误", QString::fromLocal8Bit(capture_->errorText())); // TODO: fromLocal8Bit???
        ui->btStart->setDisabled(false);
        return false;
    }

    filePath_ = fileName.toLocal8Bit().constData();
    syncUiState_(kState::capture);
    return true;
}


bool KcAudioCaptureDlg::stop()
{
    ui->btStop->setDisabled(true);

    assert(capture_ && (capture_->running() || capture_->pausing()));
    if (!capture_->stop(true)) {
        ui->btStop->setDisabled(false);
        if (isVisible())
            QMessageBox::information(this, u8"错误", QString::fromLocal8Bit(capture_->errorText())); // TODO: fromLocal8Bit???
        return false;
    }

    file_->close();
    syncUiState_(kState::ready);
    return true;
}


void KcAudioCaptureDlg::syncDeviceInfo_()
{
    KcAudioDevice ad;
    auto deviceId = ui->cbDeviceList->currentData().toInt();
    auto info = ad.info(deviceId);
    assert(QString::fromLocal8Bit(info.name) == ui->cbDeviceList->currentText());

    auto rate = ui->cbRate->currentText();
    ui->cbRate->clear();
    ui->cbRate->addItem(u8"自动选择");
    for(unsigned i = 0; i < info.sampleRates.size(); i++)
        ui->cbRate->addItem(QString::number(info.sampleRates[i]));
    ui->cbRate->setCurrentText(rate);

    ui->rbMono->setEnabled(info.inputChannels > 0);
    ui->rbStereo->setEnabled(info.inputChannels > 1);
    if(ui->rbStereo->isChecked() && info.inputChannels > 1)
        ui->rbStereo->setChecked(true);
    else
        ui->rbMono->setChecked(true);

    update();
}


void KcAudioCaptureDlg::syncUiState_(kState state)
{
    ui->cbDeviceList->setEnabled(state == kState::ready);
    ui->cbRate->setEnabled(state == kState::ready);
    ui->rbMono->setEnabled(state == kState::ready);
    ui->rbStereo->setEnabled(state == kState::ready);
    ui->cbFormat->setEnabled(state == kState::ready);
    ui->cbQuality->setEnabled(state == kState::ready);
    //ui->btSelDir->setEnabled(state == kState::ready);
    //ui->btOpenDir->setEnabled(state == kState::ready);


    ui->btPlay->setEnabled(state != kState::capture && !filePath_.empty());
    ui->btStart->setEnabled(state == kState::ready);
    ui->btStop->setEnabled(state == kState::capture || state == kState::pause);
    ui->btPause->setEnabled(state == kState::capture || state == kState::pause);

    if(state == kState::play) {
        ui->btPlay->setText(tr(u8"停止"));
        ui->btPlay->setIcon(QIcon(u8":/kRecorder/stop_play"));
    }
    else {
        ui->btPlay->setText(tr(u8"播放"));
        ui->btPlay->setIcon(QIcon(u8":/kRecorder/play"));
    }

    if(state == kState::pause) {
        ui->btPause->setText(tr(u8"继续"));
        ui->btPause->setIcon(QIcon(u8":/kRecorder/goon"));
    }
    else {
        ui->btPause->setText(tr(u8"暂停"));
        ui->btPause->setIcon(QIcon(u8":/kRecorder/pause"));
    }

    repaint(); // 立即生效
}


QString KcAudioCaptureDlg::getExtension_() const
{
    auto idxFormat = ui->cbFormat->currentIndex();
    switch (idxFormat)
    {
    case 0:
        return "ogg";
    case 1:
        return "opus";
    case 2:
        return "flac";
    case 3:
        return "wav";
    default:
        break;
    }

    return "ogg";
}


unsigned KcAudioCaptureDlg::getSampleRate_(int quality) const
{
    KcAudioDevice ad;
    auto deviceId = ui->cbDeviceList->currentData().toInt();
    auto info = ad.info(deviceId);
    std::sort(info.sampleRates.begin(), info.sampleRates.end());
    
    // 剔去8k-96k之外的频率
    while (!info.sampleRates.empty() && info.sampleRates.front() < 8000)
        info.sampleRates.erase(info.sampleRates.begin());
    while (!info.sampleRates.empty() && info.sampleRates.back() > 96000)
        info.sampleRates.pop_back();

    if (info.sampleRates.empty())
        return ad.preferredSampleRate(deviceId);

    assert(quality >= 0);
    float factor = static_cast<float>(quality) / (ui->cbQuality->count() - 1);
    int index = std::floor(factor * (info.sampleRates.size() - 1));
    if (index >= info.sampleRates.size())
        index = info.sampleRates.size() - 1;

    return info.sampleRates[index];
}


// 适配opus采样频率
unsigned KcAudioCaptureDlg::fitOpusRate(unsigned rate) const
{
    KcAudioDevice ad;
    auto deviceId = ui->cbDeviceList->currentData().toInt();
    auto info = ad.info(deviceId);
    std::sort(info.sampleRates.begin(), info.sampleRates.end());
    while (!info.sampleRates.empty() && info.sampleRates.back() > 48000)
        info.sampleRates.pop_back();
    if (info.sampleRates.empty())
        return rate;

    if (rate > info.sampleRates.back())
        rate = info.sampleRates.back();

    
    // opus编码支持的采样频率
    std::vector<unsigned> opus_rates{ 8000, 12000, 16000, 24000, 48000 };

    // 按照与rate的接近程度对opus_rates排序
    std::sort(opus_rates.begin(), opus_rates.end(), [rate](unsigned a, unsigned b) {
        return std::abs(int(a - rate)) < std::abs(int(b - rate)); });

    // 依次搜索录音设备是否支持该频率
    for (auto r : opus_rates) {
        if (std::find(info.sampleRates.begin(), info.sampleRates.end(), r) != info.sampleRates.end())
            return r;
    }

    // 若设备都不支持，返回原来的频率
    return rate;
}


// 开始录音
void KcAudioCaptureDlg::on_btStart_clicked()
{
    start();
}


// 停止录音
void KcAudioCaptureDlg::on_btStop_clicked()
{
    stop();
}


// 暂停/继续录音
void KcAudioCaptureDlg::on_btPause_clicked()
{
    ui->btPause->setDisabled(true);

    if(capture_->running()) { // 暂停
        if (capture_->pause(true)) {
            syncUiState_(kState::pause);
            return;
        }
    }
    else { // 继续
        if (capture_->goon()) {
            syncUiState_(kState::capture);
            return;
        }
    }

    ui->btPause->setDisabled(false);
    QMessageBox::information(this, u8"错误", QString::fromLocal8Bit(capture_->errorText())); // TODO: fromLocal8Bit???
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

            syncUiState_(capture_->pausing() ? kState::pause : kState::ready);
            return;
        }
    }
    else { // 回放
        auto file = std::make_shared<KgAudioFile>();
        if (!file->open(filePath_, KgAudioFile::k_read)) {
            QMessageBox::information(this, u8"错误", QString::fromLocal8Bit(file->errorText()));
            return;
        }

        if(render_->playback(file, -1, kPrivate::k_frameTime)) {
            syncUiState_(kState::play);
            assert(timerId_ == -1);
            timerId_ = startTimer(kPrivate::k_frameTime * 1000);
            return;
        }
    }

    QMessageBox::information(this, u8"错误", QString::fromLocal8Bit(render_->errorText())); // TODO: fromLocal8Bit???
}


void KcAudioCaptureDlg::on_btSelDir_clicked()
{
    auto pathName = QFileDialog::getExistingDirectory(this, tr("选择目录"),
        ui->lbSaveDir->text(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if(!pathName.isEmpty())
        ui->lbSaveDir->setText(pathName);
}


void KcAudioCaptureDlg::on_btOpenDir_clicked()
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(ui->lbSaveDir->text()));
}


void KcAudioCaptureDlg::timerEvent( QTimerEvent* e)
{
    if(e->timerId() == timerId_) {
        if(!render_->running()) {
            render_->stop(true);
            killTimer(timerId_);
            timerId_ = -1;
            syncUiState_(capture_->pausing() ? kState::pause : kState::ready);
        }
    }
}
