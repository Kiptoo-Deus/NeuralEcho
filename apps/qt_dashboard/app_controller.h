#pragma once
#include <QObject>
#include <QVariantMap>
#include <QVariantList>
#include <QTimer>
#include <QString>
#include <memory>
#include <mutex>
#include <vector>

class AudioDeviceManager;
namespace neural_echo::dsp {
    class AECPipeline;
    struct FrameMetrics;
}

class AppController : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool    running        READ running        NOTIFY runningChanged)
    Q_PROPERTY(bool    neuralEnabled  READ neuralEnabled  WRITE setNeuralEnabled
                                                          NOTIFY neuralEnabledChanged)
    Q_PROPERTY(QString inputDevice    READ inputDevice    NOTIFY devicesChanged)
    Q_PROPERTY(QString outputDevice   READ outputDevice   NOTIFY devicesChanged)
    Q_PROPERTY(double  erleDb         READ erleDb         NOTIFY metricsUpdated)
    Q_PROPERTY(double  rtf            READ rtf            NOTIFY metricsUpdated)
    Q_PROPERTY(double  cpuPercent     READ cpuPercent     NOTIFY metricsUpdated)
    Q_PROPERTY(int     framesProcessed READ framesProcessed NOTIFY metricsUpdated)

public:
    explicit AppController(QObject* parent = nullptr);
    ~AppController() override;

    bool    running()          const { return running_; }
    bool    neuralEnabled()    const { return neural_enabled_; }
    QString inputDevice()      const { return input_device_; }
    QString outputDevice()     const { return output_device_; }
    double  erleDb()           const { return erle_db_; }
    double  rtf()              const { return rtf_; }
    double  cpuPercent()       const { return cpu_percent_; }
    int     framesProcessed()  const { return frames_processed_; }

    void setNeuralEnabled(bool v);

public slots:
    Q_INVOKABLE void         startSession();
    Q_INVOKABLE void         stopSession();
    Q_INVOKABLE void         refreshDevices();
    Q_INVOKABLE QStringList  availableInputDevices()  const;
    Q_INVOKABLE QStringList  availableOutputDevices() const;
    Q_INVOKABLE void         selectInputDevice(int index);
    Q_INVOKABLE void         selectOutputDevice(int index);

    Q_INVOKABLE QVariantList inputWaveform()   const;
    Q_INVOKABLE QVariantList outputWaveform()  const;
    Q_INVOKABLE QVariantMap  sessionMetrics()  const;

signals:
    void runningChanged();
    void neuralEnabledChanged();
    void devicesChanged();
    void metricsUpdated();
    void frameReady();
    void errorOccurred(const QString& message);

private:
    void onAudioFrame(const float* input, float* output, std::size_t frames);
    void onFrameMetrics(const neural_echo::dsp::FrameMetrics& m);
    void pollCpuUsage();

    std::unique_ptr<AudioDeviceManager>          device_mgr_;
    std::unique_ptr<neural_echo::dsp::AECPipeline> pipeline_;

    QTimer poll_timer_;

    bool    running_         = false;
    bool    neural_enabled_  = true;
    QString input_device_;
    QString output_device_;

    double  erle_db_         = 0.0;
    double  rtf_             = 0.0;
    double  cpu_percent_     = 0.0;
    int     frames_processed_= 0;

    // Thread-safe waveform snapshot
    mutable std::mutex   snapshot_mutex_;
    std::vector<float>   input_snapshot_;
    std::vector<float>   output_snapshot_;
};
