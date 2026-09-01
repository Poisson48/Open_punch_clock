#include "../src/core/csv.h"
#include "../src/core/xlsx.h"
#include "../src/core/zip.h"

#include <QtTest>

class DataExchangeTest : public QObject
{
    Q_OBJECT
private slots:
    void csvRoundTrip()
    {
        const std::vector<std::vector<std::string>> rows = {{"a", "b"}, {"c,d", "e"}};
        const auto text = core::csvWrite(rows);
        const auto parsed = core::csvParse(text);
        QCOMPARE(parsed.size(), 2u);
        QCOMPARE(parsed[1][0], std::string("c,d"));
    }

    void xlsxZip()
    {
        const auto data = core::xlsxWrite({{"H1", "H2"}, {"v1", "v2"}});
        QVERIFY(data.size() > 100);
        QVERIFY(data.substr(0, 2) == "PK");
    }
};
QTEST_MAIN(DataExchangeTest)
#include "tst_dataexchange.moc"
