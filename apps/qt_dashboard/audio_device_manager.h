#pragma once
#include <QObject>
#include <QStringList>
#include <QString>
#include <functional>
#include <memory>
#include <vector>
#include <atomic>
#include <thread>
#include <mutex>

// Forward declarations to avoid PortAudio header in public interface
typedef void PaStream;
struct PaStreamCallbackTimeInfo;

namespace neural_echo::dsp {
    struct AECPipeline;
}

/// Manages PortAudio device enumeration and real-time audio I/O.
///
/// Runs a dedicated capture/playback loop that feeds PCM frames into the
/// AECPipeline on a high-priority audio thread.
class AudioDeviceManager : public QObject {
    Q_OBJECT

public:
    static constexpr int    SAMPLE_RATE  = 16000;
    static constexpr int    FRAME_SIZE   = 160;    ///< 10 ms @ 16 kHz
    static constexpr int    CHANNELS     = 1;

    /// Callback signature: invoked on the audio thread for each processed frame.
    /// (input_samples, output_samples, frame_size)
    using AudioCallback = std::function<void(const float*, float*, std::size_t)>;

    explicit AudioDeviceManager(QObject* parent = nullptr);
    ~AudioDeviceManager() override;

    // Non-copyable
    AudioDeviceManager(const AudioDeviceManager&)            = delete;
    AudioDeviceManager& operator=(const AudioDeviceManager&) = delete;

    // ── Device enumeration ────────────────────────────────────────────────────
    /// Re-scan available audio devices. Call on startup and device hot-plug.
    Q_INVOKABLE void refreshDevices();

    Q_INVOKABLE QStringList inputDeviceNames()  const;
    Q_INVOKABLE QStringList outputDeviceNames() const;

    /// Index into inputDeviceNames() / outputDeviceNames()
    Q_INVOKABLE void setInputDevice(int index);
    Q_INVOKABLE void setOutputDevice(int index);

    int  selectedInputDevice()  const { return input_device_idx_;  }
    int  selectedOutputDevice() const { return output_device_idx_; }

    // ── Stream control ────────────────────────────────────────────────────────
    /// Open streams and begin capturing / playing audio.
    bool startStream();
    /// Stop and close all streams.
    void stopStream();
    bool isRunning() const { return running_.load(std::memory_order_acquire); }

    /// Register callback invoked from the audio thread on every frame.
    void setAudioCallback(AudioCallback cb);

    /// Returns the last Pa error string, or empty if no error.
    QString lastError() const;

    // ── Waveform snapshot for UI ──────────────────────────────────────────────
    /// Thread-safe copy of the most recent input frame (for waveform display).
    std::vector<float> inputSnapshot()  const;
    /// Thread-safe copy of the most recent output frame.
    std::vector<float> outputSnapshot() const;

signals:
    void devicesChanged();
    void streamStarted(const QString& inputDevice, const QString& outputDevice);
    void streamStopped();
    void streamError(const QString& message);
    void frameProcessed();   ///< Emitted from audio thread — use Qt::QueuedConnection

private:
    // PortAudio C callback (must be static)
    static int paCallback(const void* input, void* output,
                          unsigned long frameCount,
                          const PaStreamCallbackTimeInfo* timeInfo,
                          unsigned long statusFlags,
                          void* userData);

    void initPortAudio();
    void terminatePortAudio();

    struct DeviceInfo {
        int     pa_index = -1;
        QString name;
        int     max_input_channels  = 0;
        int     max_output_channels = 0;
        double  default_sample_rate = 44100.0;
    };

    std::vector<DeviceInfo> input_devices_;
    std::vector<DeviceInfo> output_devices_;

    int   input_device_idx_  = -1;
    int   output_device_idx_ = -1;

    PaStream*         stream_         = nullptr;
    std::atomic<bool> running_        {false};
    AudioCallback     audio_callback_;

    mutable std::mutex        snapshot_mutex_;
    std::vector<float>        input_snapshot_;
    std::vector<float>        output_snapshot_;

    QString last_error_;
    bool    pa_initialised_ = false;
};
