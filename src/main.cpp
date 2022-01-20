#include <QApplication>
#include "KcAudioCaptureDlg.h"
#include "cmdline/cmdline.h"
#include <string>
#include <vector>
#include "KcAudioDevice.h"


int main(int argc, char *argv[])
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif

    // 将argv编码由local转化为utf-8
    std::vector<std::string> args(argc);
    std::vector<char*> argv_(argc);
    argv_[0] = argv[0]; // 第一个参数不调整
    for (int i = 1; i < argc; i++) {
        args[i] = QString::fromLocal8Bit(argv[i]).toStdString();
        argv_[i] = &args[i][0];
    }

    cmdline::parser cp;
    cp.add("hide", '\0', "hide app and auto-start recording");
    cp.add("stereo", '\0', "stereo recording");
    cp.add<std::string>("format", 'f', "audio encoding format(can be ogg, opus, flac, wav)", false, "ogg");
    cp.add<int>("quality", 'q', "audio encoding quality level", false, 2, cmdline::range(0, 4));
    cp.add<int>("rate", 'r', "recording audio sample rate", false, 0, cmdline::range(8000, 96000)); // 0表示自动选择与quality选项适应的采样频率
    cp.add<std::string>("path", 'p', "directory that recording audio saved", false, ""); // 缺省为程序所在目录
    cp.parse_check(argc, argv_.data());

    KcAudioDevice ad;
    if (!ad.hasInput())
        return 1;

    QApplication app(argc, argv);
    KcAudioCaptureDlg ac(cp);

    if (!cp.exist("hide"))
        ac.show();
    else if(!ac.start()) {
        return 1;
    }

    return app.exec();
}
