#include "KgAudioFile.h"
#include "libsndfile/sndfile.h"
#include <memory.h>


KgAudioFile::KgAudioFile()
{
    snd_ = 0;
    channels_ = 1;
    sampleRate_ = 44100;
    frames_ = 0;
}

KgAudioFile::KgAudioFile(int channels, int sampleRate, long frames)
{
    snd_ = 0;
    channels_ = channels;
    sampleRate_ = sampleRate;
    frames_ = frames;
}

KgAudioFile::~KgAudioFile()
{
    close();
}


#include <fcntl.h>
bool KgAudioFile::open(const std::string& path, bool write)
{
    SF_INFO si;
    ::memset(&si, 0, sizeof(si));
    si.channels = channels_;
    si.samplerate = sampleRate_;
    si.frames = frames_;
	si.format = SF_FORMAT_WAV | SF_FORMAT_FLOAT;
    snd_ = ::sf_open(path.c_str(), write ? SFM_WRITE : SFM_READ, &si);

    // TODO: libsnd对unicode路径支持不好，可以使用sf_open_fd替代
    // 但需要统一环境编译库文件
    //int fd = ::open(path.c_str(), O_RDONLY);
    //snd_ = ::sf_open_fd(fd, write ? SFM_WRITE : SFM_READ, &si, true);

    if(snd_ == nullptr) {
        error_ = ::sf_strerror(0);
		return false;
	}

    channels_ = si.channels;
    sampleRate_ = si.samplerate;
    frames_ = (long)si.frames;

    return true;
}

bool KgAudioFile::isOpen() const
{
    return snd_ != nullptr;
}

long KgAudioFile::seek(long frames, int where)
{
    return (long)::sf_seek((SNDFILE*)snd_, frames, where);
}

void KgAudioFile::close()
{
    if(snd_) ::sf_close((SNDFILE*)snd_);
    snd_ = nullptr;
}

long KgAudioFile::readFloat(float* buf, long frames)
{
    return (long)::sf_readf_float((SNDFILE*)snd_, buf, frames);
}

long KgAudioFile::readDouble(double* buf, long frames)
{
    return (long)::sf_readf_double((SNDFILE*)snd_, buf, frames);
}

long KgAudioFile::writeFloat(const float* buf, long frames)
{
    return (long)::sf_writef_float((SNDFILE*)snd_, buf, frames);
}

long KgAudioFile::writeDouble(const double* buf, long frames)
{
    return (long)::sf_writef_double((SNDFILE*)snd_, buf, frames);
}

namespace kPrivate
{
    struct KpTypeDesc
    {
        const char* ext;
        const char* desc;
    };

    static KpTypeDesc k_supportedTypeDesc[] =
    {
        { "wav", "Microsoft Wave" },
        { "aiff|aifc", "AIFF/AIFF-C" },
        { "au|snd", "Sun/DEC/NeXT Audio" },
        { "raw", "Headerless 32-bit IEEE float" },
        { "paf", "Paris Audio File" },
        { "iff", "Commodore Amiga IFF/8SVX" },
        { "wav", "NIST SPHERE" },
        { "sf", "Berkeley/IRCAM/CARL Sound File" },
        { "voc", "Creative Voice File" },
        { "w64", "Sound forge W64" },
        { "iff|svx", "Commodore Amiga" },
        { "mat4", "GNU Octave 2.0" },
        { "mat5", "GNU Octave 2.1" },
        { "pvf", "Portable Voice Format" },
        { "xi", "Fasttracker 2" },
        { "htk", "HMM Tool Kit" },
        { "caf", "Apple Core Audio Format" },
        { "sd2", "Sound Designer II" },
        { "flac", "Free Lossless Audio Codec" }
    };
}

int KgAudioFile::getSupportTypeCount()
{
	return sizeof(kPrivate::k_supportedTypeDesc)/sizeof(kPrivate::KpTypeDesc);
}

const char* KgAudioFile::getTypeExtension(int index)
{
    return kPrivate::k_supportedTypeDesc[index].ext;
}

const char* KgAudioFile::getTypeDescription(int index)
{
    return kPrivate::k_supportedTypeDesc[index].desc;
}
