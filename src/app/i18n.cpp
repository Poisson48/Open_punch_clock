#include "i18n.h"

#include <QLocale>
#include <QVariantMap>

namespace app {

namespace {

struct LocaleEntry {
    const char* code;
    const char* nativeName;
};

constexpr LocaleEntry kLocales[] = {
    {"fr",    "Français"},
    {"en",    "English"},
    {"de",    "Deutsch"},
    {"es",    "Español"},
    {"it",    "Italiano"},
    {"pt",    "Português"},
    {"pt_BR", "Português (Brasil)"},
    {"nl",    "Nederlands"},
    {"pl",    "Polski"},
    {"ru",    "Русский"},
    {"zh_CN", "简体中文"},
    {"ja",    "日本語"},
    {"ko",    "한국어"},
    {"ar",    "العربية"},
    {"tr",    "Türkçe"},
    {"uk",    "Українська"},
};

bool localeSupported(const QString& code)
{
    for (const auto& e : kLocales) {
        if (code == QLatin1String(e.code))
            return true;
    }
    return false;
}

QString baseLang(const QString& code)
{
    const int us = code.indexOf(QLatin1Char('_'));
    return us < 0 ? code : code.left(us);
}

} // namespace

QString I18n::normalizeLocale(const QString& code)
{
    if (code.isEmpty())
        return QStringLiteral("fr");
    QString c = code;
    c.replace(QLatin1Char('-'), QLatin1Char('_'));
    if (localeSupported(c))
        return c;
    const QString lang = baseLang(c);
    if (localeSupported(lang))
        return lang;
    return QStringLiteral("fr");
}

QString I18n::systemLocale()
{
    const QLocale sys = QLocale::system();
    const QString full = sys.name().replace(QLatin1Char('-'), QLatin1Char('_'));
    if (localeSupported(full))
        return full;
    const QString lang = sys.language() == QLocale::C ? QStringLiteral("fr")
                                                      : QLocale(sys.language()).name();
    const QString norm = normalizeLocale(lang);
    return norm;
}

bool I18n::installTranslator(QGuiApplication* app, QTranslator* translator,
                             const QString& locale)
{
    if (!app || !translator)
        return false;

    app->removeTranslator(translator);
    const QString norm = normalizeLocale(locale);
    QLocale::setDefault(QLocale(norm));

    if (norm.startsWith(QLatin1String("fr")))
        return true;

    const QStringList candidates = {
        QStringLiteral("openpunchclock_%1").arg(norm),
        QStringLiteral("openpunchclock_%1").arg(baseLang(norm)),
    };
    for (const QString& name : candidates) {
        if (translator->load(name, QStringLiteral(":/i18n"))) {
            app->installTranslator(translator);
            return true;
        }
    }
    return false;
}

QVariantList I18n::availableLocales()
{
    QVariantList list;
    for (const auto& e : kLocales) {
        QVariantMap m;
        m.insert(QStringLiteral("code"), QString::fromLatin1(e.code));
        m.insert(QStringLiteral("name"), QString::fromUtf8(e.nativeName));
        list.append(m);
    }
    return list;
}

} // namespace app
