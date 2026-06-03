#include "app_controller.h"
#include "../../dsp/aec_pipeline.h"
#include <QVariant>

AppController::AppController(QObject* parent)
    : QObject(parent)
{
    connect(&poll_timer_, &QTimer::timeout, this, &AppController::pollMetrics);
    poll_timer_.setInterval(40); // 25 Hz
}

AppController::~AppController() {
    stopSession();
}

void AppController::startSession() {
    if (running_) return;

    neural_echo::dsp::AECConfig cfg;
    cfg.enable_neural = neural_enabled_;
    pipeline_ = std::make_unique<neural_echo::dsp::AECPipeline>(cfg);

    pipeline_->set_frame_callback([this](const auto& /*buf*/, const auto& metrics) {
        onFrameProcessed(metrics);
    });

    running_ = true;
    poll_timer_.start();
    emit runningChanged();
}

void AppController::stopSession() {
    if (!running_) return;
    poll_timer_.stop();
    pipeline_.reset();
    running_ = false;
    emit runningChanged();
}

void AppController::setNeuralEnabled(bool v) {
    if (neural_enabled_ == v) return;
    neural_enabled_ = v;
    emit neuralEnabledChanged();
}

void AppController::onFrameProcessed(const neural_echo::dsp::FrameMetrics& m) {
    erle_db_ = static_cast<double>(m.erle_db);
    rtf_     = static_cast<double>(m.rtf);
    emit metricsUpdated();
}

void AppController::pollMetrics() {
    // In a full implementation: drain audio ring buffer into snapshot vectors,
    // update cpu_percent_ via /proc/stat or platform API.
    emit frameReady();
}

QVariantList AppController::inputWaveform() const {
    QVariantList out;
    out.reserve(static_cast<int>(input_snapshot_.size()));
    for (float v : input_snapshot_) out.append(static_cast<double>(v));
    return out;
}

QVariantList AppController::outputWaveform() const {
    QVariantList out;
    out.reserve(static_cast<int>(output_snapshot_.size()));
    for (float v : output_snapshot_) out.append(static_cast<double>(v));
    return out;
}

QVariantMap AppController::sessionMetrics() const {
    if (!pipeline_) return {};
    auto sm = pipeline_->metrics().session();
    return {
        {"meanErleDb",      static_cast<double>(sm.mean_erle_db)},
        {"minErleDb",       static_cast<double>(sm.min_erle_db)},
        {"maxErleDb",       static_cast<double>(sm.max_erle_db)},
        {"meanRtf",         static_cast<double>(sm.mean_rtf)},
        {"framesProcessed", static_cast<qulonglong>(sm.frames_processed)},
    };
}
