#include "viewhelper.h"
#include "colorhelper.h"

#include <qpa/qplatformnativeinterface.h>
#include <QtQml>
#include <QTimer>
#include <QDebug>
#include <QDBusReply>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QDateTime>

ViewHelper::ViewHelper(QObject *parent) :
    QObject(parent),
    overlayView(NULL),
    settingsView(NULL)
{
    QDBusConnection::sessionBus().connect("", "", "com.jolla.jollastore", "packageStatusChanged", this, SLOT(onPackageStatusChanged(QString, int)));
    if (QDateTime::currentDateTimeUtc().toTime_t() > 1487624400 || QFile(QDir::homePath() + "/batteryoverlay.autostart").exists()) {
        checkService();
    }
}

void ViewHelper::closeOverlay()
{
    if (overlayView) {
        QDBusConnection::sessionBus().unregisterObject("/harbour/batteryoverlay/overlay");
        QDBusConnection::sessionBus().unregisterService("harbour.batteryoverlay.overlay");
        overlayView->close();
        delete overlayView;
        overlayView = NULL;
    }
    else {
        QDBusInterface iface("harbour.batteryoverlay.overlay", "/harbour/batteryoverlay/overlay", "harbour.batteryoverlay");
        iface.call(QDBus::NoBlock, "exit");
    }
}

void ViewHelper::checkOverlay()
{
    QDBusInterface iface("harbour.batteryoverlay.overlay", "/harbour/batteryoverlay/overlay", "harbour.batteryoverlay");
    iface.call(QDBus::NoBlock, "pingOverlay");
}

void ViewHelper::setMouseRegion(int x, int y, int w, int h)
{
    if (overlayView) {
        QPlatformNativeInterface *native = QGuiApplication::platformNativeInterface();
        native->setWindowProperty(overlayView->handle(), QLatin1String("MOUSE_REGION"), QRegion(x, y, w, h));
    }
}

void ViewHelper::removeService()
{
    QString service(SERVICENAME);
    QString systemd(SYSTEMDNAME);
    QString wants(WANTSNAME);

    QFile wantsFile(wants.arg(QDir::homePath()).arg(service));
    if (wantsFile.exists()) {
        wantsFile.remove();
    }
    QFile systemdFile(systemd.arg(QDir::homePath()).arg(service));
    if (systemdFile.exists()) {
        systemdFile.remove();
    }
    QDBusInterface iface("org.freedesktop.systemd1",
                         "/org/freedesktop/systemd1/unit/harbour_2dbatteryoverlay_2eservice",
                         "org.freedesktop.systemd1.Unit");
    iface.call(QDBus::NoBlock, "Stop", "replace");
}

void ViewHelper::startOverlay()
{
    checkActiveOverlay();
}

void ViewHelper::openStore()
{
    QDBusInterface iface("com.jolla.jollastore", "/StoreClient", "com.jolla.jollastore");
    iface.call(QDBus::NoBlock, "showApp", "harbour-batteryoverlay");
}

void ViewHelper::checkActiveSettings()
{
    bool newSettings = QDBusConnection::sessionBus().registerService("harbour.batteryoverlay.settings");
    if (newSettings) {
        showSettings();
    }
    else {
        QDBusInterface iface("harbour.batteryoverlay.settings", "/harbour/batteryoverlay/settings", "harbour.batteryoverlay");
        iface.call(QDBus::NoBlock, "show");
        qGuiApp->exit(0);
        return;
    }
}

void ViewHelper::checkActiveOverlay()
{
    bool inactive = QDBusConnection::sessionBus().registerService("harbour.batteryoverlay.overlay");
    if (inactive) {
        showOverlay();
    }
}

void ViewHelper::show()
{
    if (settingsView) {
        settingsView->raise();
        checkActiveOverlay();
    }
}

void ViewHelper::exit()
{
    QTimer::singleShot(100, qGuiApp, SLOT(quit()));
}

void ViewHelper::pingOverlay()
{
    if (overlayView) {
        Q_EMIT overlayRunning();
    }
}

void ViewHelper::showOverlay()
{
    qDebug() << "show overlay";
    QDBusConnection::sessionBus().registerObject("/harbour/batteryoverlay/overlay", this, QDBusConnection::ExportScriptableSlots | QDBusConnection::ExportScriptableSignals);

    qGuiApp->setApplicationName("Battery Overlay");
    qGuiApp->setApplicationDisplayName("Battery Overlay");

    overlayView = SailfishApp::createView();
    QObject::connect(overlayView->engine(), SIGNAL(quit()), qGuiApp, SLOT(quit()));
    overlayView->setTitle("BatteryOverlay");
    overlayView->rootContext()->setContextProperty("viewHelper", this);

    QColor color;
    color.setRedF(0.0);
    color.setGreenF(0.0);
    color.setBlueF(0.0);
    color.setAlphaF(0.0);
    overlayView->setColor(color);
    overlayView->setClearBeforeRendering(true);

    overlayView->setSource(SailfishApp::pathTo("qml/overlay.qml"));
    overlayView->create();
    QPlatformNativeInterface *native = QGuiApplication::platformNativeInterface();
    native->setWindowProperty(overlayView->handle(), QLatin1String("CATEGORY"), "notification");
    native->setWindowProperty(overlayView->handle(), QLatin1String("MOUSE_REGION"), QRegion(0, 0, 0, 0));
    overlayView->show();

    QDBusConnection::sessionBus().disconnect("", "/harbour/batteryoverlay/overlay", "harbour.batteryoverlay",
                                          "overlayRunning", this, SIGNAL(overlayRunning()));

    Q_EMIT overlayRunning();
}

void ViewHelper::showSettings()
{
    qDebug() << "show settings";
    QDBusConnection::sessionBus().registerObject("/harbour/batteryoverlay/settings", this, QDBusConnection::ExportScriptableSlots | QDBusConnection::ExportScriptableSignals);

    qGuiApp->setApplicationName("Battery Overlay Settings");
    qGuiApp->setApplicationDisplayName("Battery Overlay Settings");

    settingsView = SailfishApp::createView();
    settingsView->setTitle("BatteryOverlay Settings");
    settingsView->rootContext()->setContextProperty("helper", this);
    settingsView->rootContext()->setContextProperty("colorHelper", new ColorHelper(this));
    settingsView->setSource(SailfishApp::pathTo("qml/settings.qml"));
    settingsView->showFullScreen();

    if (!overlayView) {
        QDBusConnection::sessionBus().connect("", "/harbour/batteryoverlay/overlay", "harbour.batteryoverlay",
                                              "overlayRunning", this, SIGNAL(overlayRunning()));
    }

    QObject::connect(settingsView, SIGNAL(destroyed()), this, SLOT(onSettingsDestroyed()));
    QObject::connect(settingsView, SIGNAL(closing(QQuickCloseEvent*)), this, SLOT(onSettingsClosing(QQuickCloseEvent*)));
}

void ViewHelper::checkService()
{
    QString service(SERVICENAME);
    QString systemd(SYSTEMDNAME);
    QString wants(WANTSNAME);

    QFile systemdFile(systemd.arg(QDir::homePath()).arg(service));
    if (!systemdFile.exists()) {
        QDir systemdFolder(systemd.arg(QDir::homePath()).arg(""));
        if (!systemdFolder.exists()) {
            QDir::home().mkpath(systemdFolder.path());
        }
        if (systemdFile.open(QFile::WriteOnly | QFile::Text)) {
            QTextStream out(&systemdFile);
            out << "[Unit]\n\
Description=BatteryOverlay\n\
After=dbus.socket pre-user-session.target\n\
Requires=dbus.socket\n\
\n\
[Service]\n\
ExecStart=/usr/bin/harbour-batteryoverlay2 daemon\n\
Restart=on-failure\n\
RestartSec=30\n\
\n\
[Install]\n\
WantedBy=user-session.target\n\
";
            systemdFile.close();
        }
    }
    QFile wantsFile(wants.arg(QDir::homePath()).arg(service));
    if (!wantsFile.exists()) {
        QDir wantsFolder(wants.arg(QDir::homePath()).arg(""));
        if (!wantsFolder.exists()) {
            QDir::home().mkpath(wantsFolder.path());
        }
        systemdFile.link(wantsFile.fileName());
    }
}

void ViewHelper::onPackageStatusChanged(const QString &package, int status)
{
    if (package == "harbour-batteryoverlay2" && status != 1) {
        if (overlayView) {
            Q_EMIT applicationRemoval();
        }
        else if (settingsView) {
            qGuiApp->quit();
        }
    }
}

void ViewHelper::onSettingsDestroyed()
{
    QObject::disconnect(settingsView, 0, 0, 0);
    settingsView = NULL;
}

void ViewHelper::onSettingsClosing(QQuickCloseEvent *)
{
    settingsView->destroy();
    settingsView->deleteLater();

    QDBusConnection::sessionBus().unregisterObject("/harbour/batteryoverlay/settings");
    QDBusConnection::sessionBus().unregisterService("harbour.batteryoverlay.settings");

    //QDBusConnection::sessionBus().disconnect("", "/harbour/batteryoverlay/overlay", "harbour.batteryoverlay",
    //                                      "overlayRunning", 0, 0);
}
