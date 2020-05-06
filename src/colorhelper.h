#ifndef COLORHELPER_H
#define COLORHELPER_H

#include <QObject>
#include <QColor>

class ColorHelper : public QObject
{
    Q_OBJECT
public:
    explicit ColorHelper(QObject *parent = 0);
    Q_INVOKABLE QString colorString(const QColor &color);

};

#endif // COLORHELPER_H
