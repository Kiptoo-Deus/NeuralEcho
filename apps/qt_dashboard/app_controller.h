#pragma once
#include <QObject>
#include <QVariantMap>
#include <QTimer>
#include <memory>

namespace neural_echo::dsp {
    class AECPipeline;
    struct FrameMetrics;
}

/// Bridge between the C++ AEC pipeline and the QML UI.
/// Exposed to QML as the singleton AppController.
class AppController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool     running     READ running     NOTIFY runningChanged)
    Q_PROPERTY(bool     neuralEnabled READ neuralEnabled WRITE setNeuralEnabled
               NOTIFY neuralEnabledChanged)
    Q_PROPERTY(QString  deviceName  READ deviceName  NOTIFY deviceNameChanged)
    Q_PROPERTY(double   erleDb      READ erleDb      NOTIFY metricsUpdated)
    Q_PROPERTY(double   rtf         READ rtf         NOTIFY metricsUpdated)
    Q_PROPERTY(double   cpuPercent  READ cpuPercent  NOTIFY metricsUpdated)

public:
    explicit AppController(QObject* parent = nullptr);
    ~AppController() override;

    bool    running()       const { return running_; }
    bool    neuralEnabled() const { return neural_enabled_; }
    QString deviceName()    const { return device_name_; }
    double  erleDb()        const { return erle_db_; }
    double  rtf()           const { return rtf_; }
    double  cpuPercent()    const { return cpu_percent_; }

    void setNeuralEnabled(bool v);

public slots:
    /// Start real-time processing session.
    Q_INVOKABLE void startSession();
    /// Stop real-time processing session.
    Q_INVOKABLE void stopSession();
    /// Returns a snapshot of the latest waveform samples as a QVariantList.
    Q_INVOKABLE QVariantList inputWaveform()  const;
    Q_INVOKABLE QVariantList outputWaveform() const;
    /// Returns current session aggregate metrics as a QVariantMap.
    Q_INVOKABLE QVariantMap  sessionMetrics() const;

signals:
    void runningChanged();
    void neuralEnabledChanged();
    void deviceNameChanged();
    void metricsUpdated();
    void frameReady();    ///< Emitted at 25 Hz to trigger waveform redraws.

private:
    void onFrameProcessed(const neural_echo::dsp::FrameMetrics& m);
    void pollMetrics();

    std::unique_ptr<neural_echo::dsp::AECPipeline> pipeline_;
    QTimer  poll_timer_;

    bool    running_        = false;
    bool    neural_enabled_ = true;
    QString device_name_    = "No device";
    double  erle_db_        = 0.0;
    double  rtf_            = 0.0;
    double  cpu_percent_    = 0.0;

    // Waveform snapshot buffers (guarded by mutex in real impl)
    std::vector<float> input_snapshot_;
    std::vector<float> output_snapshot_;
};
