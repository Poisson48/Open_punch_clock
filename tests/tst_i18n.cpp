#include "../src/app/i18n.h"

#include <QtTest>

class I18nTest : public QObject
{
    Q_OBJECT
private slots:
    void normalizeLocale()
    {
        QCOMPARE(app::I18n::normalizeLocale(QStringLiteral("en-US")), QStringLiteral("en"));
        QCOMPARE(app::I18n::normalizeLocale(QStringLiteral("pt-BR")), QStringLiteral("pt_BR"));
        QCOMPARE(app::I18n::normalizeLocale(QStringLiteral("zh-CN")), QStringLiteral("zh_CN"));
        QCOMPARE(app::I18n::normalizeLocale(QStringLiteral("xx")), QStringLiteral("fr"));
    }

    void availableLocales()
    {
        const QVariantList locales = app::I18n::availableLocales();
        QCOMPARE(locales.size(), 16);
        bool hasFr = false;
        bool hasEn = false;
        for (const QVariant& v : locales) {
            const QString code = v.toMap().value(QStringLiteral("code")).toString();
            if (code == QLatin1String("fr"))
                hasFr = true;
            if (code == QLatin1String("en"))
                hasEn = true;
        }
        QVERIFY(hasFr);
        QVERIFY(hasEn);
    }
};

QTEST_MAIN(I18nTest)
#include "tst_i18n.moc"
