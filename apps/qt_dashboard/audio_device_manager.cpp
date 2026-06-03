#include "audio_device_manager.h"
#include <portaudio.h>
#include <QDebug>
#include <cstring>
#include <stdexcept>

// ── Construction / destruction ─────────────────────────────────────────────────

AudioDeviceManager::AudioDeviceManager(QObject* parent)
    : QObject(parent)
{
    input_snapshot_.assign(FRAME_SIZE, 0.f);
    output_snapshot_.assign(FRAME_SIZE, 0.f);
    initPortAudio();
    refreshDevices();
}

AudioDeviceManager::~AudioDeviceManager() {
    stopStream();
    terminatePortAudio();
}

// ── PortAudio lifecycle ────────────────────────────────────────────────────────

void AudioDeviceManager::initPortAudio() {
    PaError err = Pa_Initialize();
    if (err != paNoError) {
        last_error_ = QString("Pa_Initialize failed: %1").arg(Pa_GetErrorText(err));
        qWarning() << last_error_;
        return;
    }
    pa_initialised_ = true;
}

void AudioDeviceManager::terminatePortAudio() {
    if (pa_initialised_) {
        Pa_Terminate();
        pa_initialised_ = false;
    }
}

// ── Device enumeration ─────────────────────────────────────────────────────────

void AudioDeviceManager::refreshDevices() {
    input_devices_.clear();
    output_devices_.clear();

    if (!pa_initialised_) return;

    int count = Pa_GetDeviceCount();
    if (count < 0) {
        last_error_ = QString("Pa_GetDeviceCount error: %1").arg(Pa_GetErrorText(count));
        return;
    }

    for (int i = 0; i < count; ++i) {
        const PaDeviceInfo* info = Pa_GetDeviceInfo(i);
        if (!info) continue;

        DeviceInfo d;
        d.pa_index              = i;
        d.name                  = QString::fromUtf8(info->name);
        d.max_input_channels    = info->maxInputChannels;
        d.max_output_channels   = info->maxOutputChannels;
        d.default_sample_rate   = info->defaultSampleRate;

        if (info->maxInputChannels > 0)  input_devices_.push_back(d);
        if (info->maxOutputChannels > 0) output_devices_.push_back(d);
    }

    // Select defaults
    int def_in  = Pa_GetDefaultInputDevice();
    int def_out = Pa_GetDefaultOutputDevice();
    for (int i = 0; i < static_cast<int>(input_devices_.size()); ++i)
        if (input_devices_[i].pa_index == def_in)  { input_device_idx_  = i; break; }
    for (int i = 0; i < static_cast<int>(output_devices_.size()); ++i)
        if (output_devices_[i].pa_index == def_out) { output_device_idx_ = i; break; }

    emit devicesChanged();
}

QStringList AudioDeviceManager::inputDeviceNames() const {
    QStringList names;
    for (const auto& d : input_devices_) names << d.name;
    return names;
}

QStringList AudioDeviceManager::outputDeviceNames() const {
    QStringList names;
    for (const auto& d : output_devices_) names << d.name;
    return names;
}

void AudioDeviceManager::setInputDevice(int index) {
    if (index >= 0 && index < static_cast<int>(input_devices_.size()))
        input_device_idx_ = index;
}

void AudioDeviceManager::setOutputDevice(int index) {
    if (index >= 0 && index < static_cast<int>(output_devices_.size()))
        output_device_idx_ = index;
}

void AudioDeviceManager::setAudioCallback(AudioCallback cb) {
    audio_callback_ = std::move(cb);
}

// ── Stream control ─────────────────────────────────────────────────────────────

bool AudioDeviceManager::startStream() {
    if (!pa_initialised_ || running_.load()) return false;
    if (input_device_idx_ < 0 || output_device_idx_ < 0) {
        last_error_ = "No input or output device selected";
        return false;
    }

    PaStreamParameters inParams, outParams;
    std::memset(&inParams,  0, sizeof(inParams));
    std::memset(&outParams, 0, sizeof(outParams));

    inParams.device                    = input_devices_[input_device_idx_].pa_index;
    inParams.channelCount              = CHANNELS;
    inParams.sampleFormat              = paFloat32;
    inParams.suggestedLatency          =
        Pa_GetDeviceInfo(inParams.device)->defaultLowInputLatency;

    outParams.device                   = output_devices_[output_device_idx_].pa_index;
    outParams.channelCount             = CHANNELS;
    outParams.sampleFormat             = paFloat32;
    outParams.suggestedLatency         =
        Pa_GetDeviceInfo(outParams.device)->defaultLowOutputLatency;

    PaError err = Pa_OpenStream(
        &stream_,
        &inParams,
        &outParams,
        SAMPLE_RATE,
        static_cast<unsigned long>(FRAME_SIZE),
        paClipOff,
        &AudioDeviceManager::paCallback,
        this);

    if (err != paNoError) {
        last_error_ = QString("Pa_OpenStream failed: %1").arg(Pa_GetErrorText(err));
        emit streamError(last_error_);
        return false;
    }

    err = Pa_StartStream(stream_);
    if (err != paNoError) {
        Pa_CloseStream(stream_);
        stream_ = nullptr;
        last_error_ = QString("Pa_StartStream failed: %1").arg(Pa_GetErrorText(err));
        emit streamError(last_error_);
        return false;
    }

    running_.store(true, std::memory_order_release);

    QString inName  = input_devices_[input_device_idx_].name;
    QString outName = output_devices_[output_device_idx_].name;
    emit streamStarted(inName, outName);
    return true;
}

void AudioDeviceManager::stopStream() {
    if (!running_.load()) return;
    running_.store(false, std::memory_order_release);

    if (stream_) {
        Pa_StopStream(stream_);
        Pa_CloseStream(stream_);
        stream_ = nullptr;
    }
    emit streamStopped();
}

// ── PortAudio real-time callback ───────────────────────────────────────────────

int AudioDeviceManager::paCallback(const void*  input,
                                    void*        output,
                                    unsigned long frameCount,
                                    const PaStreamCallbackTimeInfo* /*timeInfo*/,
                                    unsigned long /*statusFlags*/,
                                    void*        userData)
{
    auto* self = static_cast<AudioDeviceManager*>(userData);
    const float* in  = static_cast<const float*>(input);
    float*       out = static_cast<float*>(output);

    if (!self->running_.load(std::memory_order_acquire)) {
        std::memset(out, 0, frameCount * sizeof(float));
        return paComplete;
    }

    // Invoke user callback (AEC pipeline lives here)
    if (self->audio_callback_) {
        self->audio_callback_(in, out, frameCount);
    } else {
        // Passthrough
        std::memcpy(out, in, frameCount * sizeof(float));
    }

    // Update waveform snapshots (non-blocking try-lock)
    if (self->snapshot_mutex_.try_lock()) {
        std::copy(in,  in  + frameCount, self->input_snapshot_.data());
        std::copy(out, out + frameCount, self->output_snapshot_.data());
        self->snapshot_mutex_.unlock();
    }

    return paContinue;
}

// ── Snapshot accessors ─────────────────────────────────────────────────────────

std::vector<float> AudioDeviceManager::inputSnapshot() const {
    std::lock_guard<std::mutex> lock(snapshot_mutex_);
    return input_snapshot_;
}

std::vector<float> AudioDeviceManager::outputSnapshot() const {
    std::lock_guard<std::mutex> lock(snapshot_mutex_);
    return output_snapshot_;
}

QString AudioDeviceManager::lastError() const {
    return last_error_;
}
