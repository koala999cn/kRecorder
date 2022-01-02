#pragma once
#include <string>


class KgAudioFile
{
public:
    KgAudioFile();
    KgAudioFile(int channels, int sampleRate, long frames);
    ~KgAudioFile();

    bool open(const std::string& path, bool write = false);
    bool isOpen() const;

	/* Seek within the waveform data chunk of the SNDFILE. sf_seek () uses
	** the same values for whence (SEEK_SET, SEEK_CUR and SEEK_END) as
	** stdio.h function fseek ().
	** An offset of zero with whence set to SEEK_SET will position the
	** read / write pointer to the first data sample.
	** On success sf_seek returns the current position in (multi-channel)
	** samples from the start of the file.
	** Please see the libsndfile documentation for moving the read pointer
	** separately from the write pointer on files open in mode SFM_RDWR.
	** On error all of these functions return -1.
	*/
    long seek(long frames, int where);

    void close();

    auto channels() const { return channels_; }
    auto sampleRate() const { return sampleRate_; }
    auto frames() const { return frames_; }

    // buf.size >= frames*channels
    long readFloat(float* buf, long frames);
    long readDouble(double* buf, long frames);

    long writeFloat(const float* buf, long frames);
    long writeDouble(const double* buf, long frames);

    const char* error() const { return error_.c_str(); }

    static int getSupportTypeCount();
    static const char* getTypeExtension(int nIndex);
    static const char* getTypeDescription(int nIndex);

private:
    void* snd_;

    int channels_;
    int sampleRate_;
    long frames_;

    std::string error_;
};

