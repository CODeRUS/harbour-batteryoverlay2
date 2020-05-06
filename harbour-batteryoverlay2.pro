TARGET = harbour-batteryoverlay2

QT += dbus gui-private
CONFIG += sailfishapp
PKGCONFIG += mlite5

DEFINES += APP_VERSION=\\\"$$VERSION\\\"

SOURCES += \
    src/main.cpp \
    src/viewhelper.cpp \
    src/colorhelper.cpp

HEADERS += \
    src/viewhelper.h \
    src/colorhelper.h

OTHER_FILES += \
    rpm/harbour-batteryoverlay2.spec \
    harbour-batteryoverlay2.desktop \
    harbour-batteryoverlay2.png \
    qml/overlay.qml \
    qml/settings.qml \
    qml/pages/MainPage.qml \
    qml/cover/CoverPage.qml \
    qml/pages/ColorDialog.qml \
    qml/components/ColorSelector.qml \
    qml/components/ColorItem.qml \
    qml/pages/AboutPage.qml
