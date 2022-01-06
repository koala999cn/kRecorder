#include "QUtils.h"
#include <QString>
#include <QFileDialog>
#include <QStringLiteral>
#include "../audio/KgAudioFile.h"


namespace kPrivate
{
    static QString getAudioFileFilter() 
    {
        static QString filter;
        if (!filter.isNull()) return filter;

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
            if (i != KgAudioFile::getSupportTypeCount() - 1)
                filter.append(";;");
        }

        return filter;
    }
}


std::string QUtils::getAudioSavePath(QWidget* parent)
{
    auto filter = kPrivate::getAudioFileFilter();
    auto path = QFileDialog::getSaveFileName(parent, QStringLiteral("±£´æÒôÆµ"), "", filter);
    return path.toLocal8Bit().data();
}


std::string QUtils::getAudioOpenPath(QWidget* parent)
{
    auto filter = kPrivate::getAudioFileFilter();
    auto path = QFileDialog::getOpenFileName(parent, QStringLiteral("´ò¿ªÒôÆµ"), "", filter);
    return path.toLocal8Bit().data();
}