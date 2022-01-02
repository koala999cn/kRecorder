#pragma once
#include "kDsp.h"
#include <vector>


using kPoint2d = std::pair<kReal, kReal>;
using kRange = std::pair<kReal, kReal>;

// 用于绘图的数据接口类
class KvData1d
{
public:

    KvData1d() {}
    virtual ~KvData1d() {}


    // 获取数据点数目
    virtual kIndex count() const = 0;


    // 支持多通道数据，返回通道数目
    virtual kIndex channels() const = 0;


    // 获取channel通道的第idx个数据点的坐标值(x, y)
    virtual kPoint2d value(kIndex idx, kIndex channel = 0) const = 0;


    // 获取x值范围，用于设置横轴坐标
    virtual kRange xrange() const = 0;


    // 获取指定通道的y值范围，用于设置纵轴坐标. 提供暴力求解的缺省实现
    // TODO: 使用极值修正算法
    virtual kRange yrange(kIndex channel) const {
        kReal ymin = value(0, channel).second;
        kReal ymax(ymin);

        for(kIndex i = 1; i < count(); i++) {
            auto y = value(i, channel).second;
            ymin = std::min(ymin, y);
            ymax = std::max(ymax, y);
        }

        return { ymin, ymax };
    }


    // 获取所有通道数据的y值最小、最大值
    virtual kRange yrange() const {
        auto yr = yrange(0);
        for(kIndex i = 1; i < channels(); i++) {
            auto yri = yrange(i);
            yr.first = std::min(yr.first, yri.first);
            yr.second = std::max(yr.second, yri.second);
        }

        return yr;
    }

};
