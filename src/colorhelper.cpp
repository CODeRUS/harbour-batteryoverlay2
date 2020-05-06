#include "colorhelper.h"

ColorHelper::ColorHelper(QObject *parent): QObject(parent)
{

}

QString ColorHelper::colorString(const QColor &color)
{
    return color.name(QColor::HexArgb);
}
