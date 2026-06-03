#include "app_controller.h"
#include "audio_device_manager.h"
#include "../../dsp/aec_pipeline.h"
#include <QVariant>
#include <QDebug>
#include <chrono>
#include <cstring>

#ifdef Q_OS_LINUX
#  include <fstream>
#  include <sstream>
#endif

AppController::AppController(QObject* parent)
    : QObject(parent),
      device_mgr_(std::make_unique<AudioDeviceManager>())
{
    input_snapshot_.assign(AudioDeviceManager::FRAME_SIZE, 0.f);
    output_snapshot_.assign(AudioDeviceManager::FRAME_SIZE, 0.f);

    connect(device_mgr_.get(), &AudioDeviceManager::devicesChanged,
            this, &AppController::devicesChanged);
    connect(device_mgr_.get(), &AudioDeviceManager::streamStarted,
            this, [this](const QString& in, const QString& out) {
                input_device_  = in;
                output_device_ = out;
                emit devicesChanged();
            });
    connect(device_mgr_.get(), &AudioDeviceManager::streamError,
            this, [this](const QString& msg) {
                emit errorOccurred(msg);
                stopSession();
            });

    connect(&poll_timer_, &QTimer::timeout, this, [this]() {
        pollCpuUsage();
        emit frameReady();
    });
    poll_timer_.setInterval(40); // 25 Hz
}

AppController::~AppController() { stopSession(); }

void AppController::startSession() {
    if (running_) return;

    neural_echo::dsp::AECConfig cfg;
    cfg.enable_neural = neural_enabled_;
    pipeline_ = std::make_unique<neural_echo::dsp::AECPipeline>(cfg);

    pipeline_->set_frame_callback(
        [this](const neural_echo::dsp::AudioBuffer&,
               const neural_echo::dsp::FrameMetrics& fm) {
            onFrameMetrics(fm);
        });

    device_mgr_->setAudioCallback(
        [this](const float* in, float* out, std::size_t n) {
            onAudioFrame(in, out, n);
        });

    if (!device_mgr_->startStream()) {
        pipeline_.reset();
        emit errorOccurred(device_mgr_->lastError());
        return;
    }

    running_ = true;
    poll_timer_.start();
    emit runningChanged();
}

void AppController::stopSession() {
    if (!running_) return;
    poll_timer_.stop();
    device_mgr_->stopStream();
    pipeline_.reset();
    running_ = false;
    emit runningChanged();
}

void AppController::setNeuralEnabled(bool v) {
    if (neural_enabled_ == v) return;
    neural_enabled_ = v;
    emit neuralEnabledChanged();
}

void AppController::refreshDevices()                { device_mgr_->refreshDevices(); }
QStringList AppController::availableInputDevices()  const { return device_mgr_->inputDeviceNames(); }
QStringList AppController::availableOutputDevices() const { return device_mgr_->outputDeviceNames(); }
void AppController::selectInputDevice(int i)        { device_mgr_->setInputDevice(i); }
void AppController::selectOutputDevice(int i)       { device_mgr_->setOutputDevice(i); }

void AppController::onAudioFrame(const float* input, float* output, std::size_t frames) {
    if (!pipeline_) { std::memcpy(output, input, frames * sizeof(float)); return; }

    auto ts = static_cast<uint64_t>(
        std::chrono::steady_clock::now().time_since_epoch().count());

    neural_echo::dsp::AudioFrame far_f{input, frames, ts};
    neural_echo::dsp::AudioFrame mic_f{input, frames, ts};

    try {
        auto result  = pipeline_->process(far_f, mic_f);
        std::size_t n = std::min(frames, result.samples.size());
        std::memcpy(output, result.samples.data(), n * sizeof(float));
        if (n < frames) std::memset(output + n, 0, (frames - n) * sizeof(float));

        if (snapshot_mutex_.try_lock()) {
            input_snapshot_.assign(input,  input  + frames);
            output_snapshot_.assign(output, output + frames);
            snapshot_mutex_.unlock();
        }
    } catch (...) {
        std::memcpy(output, input, frames * sizeof(float));
    }
}

void AppController::onFrameMetrics(const neural_echo::dsp::FrameMetrics& m) {
    erle_db_          = static_cast<double>(m.erle_db);
    rtf_              = static_cast<double>(m.rtf);
    frames_processed_ = static_cast<int>(
        pipeline_ ? pipeline_->metrics().session().frames_processed : 0);
    emit metricsUpdated();
}

void AppController::pollCpuUsage() {
#ifdef Q_OS_LINUX
    static unsigned long long prev_idle = 0, prev_total = 0;
    std::ifstream stat("/proc/stat");
    if (!stat.is_open()) return;
    std::string line; std::getline(stat, line);
    std::istringstream ss(line);
    std::string cpu;
    unsigned long long user, nice, system, idle, iowait, irq, softirq;
    ss >> cpu >> user >> nice >> system >> idle >> iowait >> irq >> softirq;
    unsigned long long total = user + nice + system + idle + iowait + irq + softirq;
    unsigned long long di = idle - prev_idle, dt = total - prev_total;
    if (dt > 0) cpu_percent_ = 100.0 * (1.0 - static_cast<double>(di) / dt);
    prev_idle = idle; prev_total = total;
#else
    cpu_percent_ = 0.0;
#endif
}

QVariantList AppController::inputWaveform() const {
    std::lock_guard<std::mutex> lock(snapshot_mutex_);
    QVariantList l;
    l.reserve(static_cast<int>(input_snapshot_.size()));
    for (float v : input_snapshot_) l.append(static_cast<double>(v));
    return l;
}

QVariantList AppController::outputWaveform() const {
    std::lock_guard<std::mutex> lock(snapshot_mutex_);
    QVariantList l;
    l.reserve(static_cast<int>(output_snapshot_.size()));
    for (float v : output_snapshot_) l.append(static_cast<double>(v));
    return l;
}

QVariantMap AppController::sessionMetrics() const {
    if (!pipeline_) return {};
    const auto sm = pipeline_->metrics().session();
    return {
        {"meanErleDb",      static_cast<double>(sm.mean_erle_db)},
        {"minErleDb",       static_cast<double>(sm.min_erle_db)},
        {"maxErleDb",       static_cast<double>(sm.max_erle_db)},
        {"meanRtf",         static_cast<double>(sm.mean_rtf)},
        {"framesProcessed", static_cast<qulonglong>(sm.frames_processed)},
    };
}
