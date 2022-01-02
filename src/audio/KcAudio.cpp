#include "KcAudio.h"
#include "KgAudioFile.h"


std::string KcAudio::load(const std::string& path)
{
    KgAudioFile sf;

    if(!sf.open(path)) return sf.error();

    auto dx = static_cast<kReal>(1) / sf.sampleRate();
    reset(dx, sf.channels(), sf.frames());

    //TODO: 处理kReal为非double的情况
    //assert(std::is_same<kReal, double>());

    sf.readDouble(data_.data(), sf.frames());

    return "";
}


std::string KcAudio::save(const std::string& path)
{
	KgAudioFile sf(channels(), samplingRate(), count());

	if (!sf.open(path, true)) 
        return sf.error();

    if (sf.channels() != channels() ||
        sf.sampleRate() != samplingRate()) 
        return "audio parameters mismatch";

    assert(data_.size() == count() * channels());
    if (sf.writeDouble(data_.data(), count()) != count())
        return sf.error();

	sf.close();
	return "";
}