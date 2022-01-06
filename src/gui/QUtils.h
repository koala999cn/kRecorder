#pragma once
#include <string>

class QWidget;

class  QUtils
{
public:
	static std::string getAudioSavePath(QWidget* parent = nullptr);
	static std::string getAudioOpenPath(QWidget* parent = nullptr);

private:
	 QUtils() {}
	~ QUtils() {}
};

