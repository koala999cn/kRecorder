#pragma once

#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#  define K_DECL_EXPORT __declspec(dllexport)
#  define K_DECL_IMPORT __declspec(dllimport)
#else
#  define K_DECL_EXPORT     __attribute__((visibility("default")))
#  define K_DECL_IMPORT     __attribute__((visibility("default")))
#endif

#if defined(KDSP_LIBRARY)
#  define KDSP_EXPORT K_DECL_EXPORT
#else
#  define KDSP_EXPORT K_DECL_IMPORT
#endif


using kReal = double;
using kIndex = long;

enum KeOOR // out of range的取值模式
{
    k_undefined, // f(xmin-dx) = undefined
    k_period, // f(xmin-dx) = f(xmax-dx)
    k_reflect, // f(xmin-dx) = f(xmin+dx)
    k_nearest, // f(xmin-dx) = f(xmin)
    k_extrapolate // 线性外插, k_extrapolate+1表示二次外插
};
