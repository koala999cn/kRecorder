# kRecorder

#### 介绍
一个录音程序。基于Qt实现GUI界面，基于RtAudio实现录音，基于libsndfile实现音频存储。

#### 编译
由于Qt使用MD标记编译，所以本程序和使用的库也统一使用MD标记编译。

RtAudio在静态库编译情况下，会自动将MD标记改写为MT标记，可通过手动设定RTAUDIO_STATIC_MSVCRT为false避免。

#### 使用
程序运行界面如下：


