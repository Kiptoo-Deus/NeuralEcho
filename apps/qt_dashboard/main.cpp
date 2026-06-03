#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include "app_controller.h"

int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);
    app.setApplicationName("NeuralEcho");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("NeuralEcho Research");

    QQuickStyle::setStyle("Basic");

    qmlRegisterType<AppController>("NeuralEcho", 1, 0, "AppController");

    QQmlApplicationEngine engine;

    // Expose version to QML
    engine.rootContext()->setContextProperty("appVersion", app.applicationVersion());

    const QUrl url(u"qrc:/NeuralEcho/qml/Main.qml"_qs);
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject* obj, const QUrl& objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);

    engine.load(url);
    return app.exec();
}
