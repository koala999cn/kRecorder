<<<<<<< HEAD
#include "QWaveBar.h"
#include <QPainter>


QWaveBar::QWaveBar(QWidget *parent)
    : QWidget{parent}
    , barColor_(QColor(255, 63, 63)) // (255, 92, 92)
{
   barWidth_ = 5, barSpace_ = 4;
}


int QWaveBar::getBarCount() const {
    return 0.5 + width() / (barWidth_ + barSpace_);
}


void QWaveBar::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(Qt::transparent);
    painter.setBrush(barColor_);

    // draw the bars
    for (unsigned i = 0; i < barRanges_.size(); i++) {
        int x0 = i * (barWidth_ + barSpace_);

        // map y: -1 to h, 1 to 0, 0.5 to h/2
        auto& r = barRanges_[i];
        int y0 = (1 - r.second) * height() * 0.5;
        int yh = (r.second - r.first) * height() * 0.5;

        painter.drawRoundedRect(QRect(x0, y0, barWidth_, yh), 1, 2);
    }
}
=======
#include "QWaveBar.h"
#include <QPainter>


QWaveBar::QWaveBar(QWidget *parent)
    : QWidget{parent}
    , barColor_(QColor(255, 63, 63)) // (255, 92, 92)
{
   barWidth_ = 5, barSpace_ = 4;
}


int QWaveBar::getBarCount() const {
    return 0.5 + width() / (barWidth_ + barSpace_);
}


void QWaveBar::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(Qt::transparent);
    painter.setBrush(barColor_);

    // draw the bars
    for (unsigned i = 0; i < barRanges_.size(); i++) {
        int x0 = i * (barWidth_ + barSpace_);

        // map y: -1 to h, 1 to 0, 0.5 to h/2
        auto& r = barRanges_[i];
        int y0 = (1 - r.second) * height() * 0.5;
        int yh = (r.second - r.first) * height() * 0.5;

        painter.drawRoundedRect(QRect(x0, y0, barWidth_, yh), 1, 2);
    }
}
>>>>>>> cda5f07e398855ee1ee2981f09e438c3a9f5813e
