#pragma once

#include <QGuiApplication>
#include <QString>
#include <QStringList>
#include <QTranslator>
#include <QVariantList>

namespace app {

class I18n
{
public:
    static QString normalizeLocale(const QString& code);
    static QString systemLocale();
    static bool installTranslator(QGuiApplication* app, QTranslator* translator,
                                  const QString& locale);
    static QVariantList availableLocales();
};

} // namespace app
