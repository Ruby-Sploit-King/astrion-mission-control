#include "tle.hpp"

#include <QtTest>

class TleTests : public QObject
{
    Q_OBJECT

private slots:
    void parsesIssElements();
    void parsesTextWithNameLine();
    void rejectsSwappedSatelliteNumbers();
    void rejectsMalformedLines();
    void rejectsMissingLines();
    void minutesSinceEpochIsLinear();
};

void TleTests::parsesIssElements()
{
  const QString line1 =
      "1 25544U 98067A   26290.82202591  .00007563  00000+0  14569-3 0  9996";
  const QString line2 =
      "2 25544  51.6326  53.2215 0006765 277.4279  82.5940 15.48913056590762";

  QString error;
  const std::optional<Tle> tle = Tle::parse(line1, line2, &error);

  QVERIFY2(tle.has_value(), qPrintable(error));
  QCOMPARE(tle->satnum, 25544);
  QCOMPARE(tle->epochUtc.date().year(), 2026);
  QCOMPARE(tle->epochUtc.date().dayOfYear(), 290);

  const double expectedHourOfDay = 0.82202591 * 24.0;
  const double actualHourOfDay =
      tle->epochUtc.time().msecsSinceStartOfDay() / 3600000.0;
  QVERIFY(qAbs(actualHourOfDay - expectedHourOfDay) < 0.001);

  QVERIFY(qAbs(tle->argpo * 180.0 / 3.14159265358979323846
               - 277.4279) < 1.0e-9);
  QVERIFY(qAbs(tle->mo * 180.0 / 3.14159265358979323846
               - 82.5940) < 1.0e-9);
  QCOMPARE(tle->revnum, 59076);
}

void TleTests::parsesTextWithNameLine()
{
  const QString text =
      "ISS (ZARYA)\n"
      "1 25544U 98067A   26290.82202591  .00007563  00000+0  14569-3 0  9996\n"
      "2 25544  51.6326  53.2215 0006765 277.4279  82.5940 15.48913056590762\n";

  QString error;
  const std::optional<Tle> tle = Tle::fromText(text, &error);

  QVERIFY2(tle.has_value(), qPrintable(error));
  QCOMPARE(tle->name, QStringLiteral("ISS (ZARYA)"));
  QCOMPARE(tle->satnum, 25544);
}

void TleTests::rejectsSwappedSatelliteNumbers()
{
  const QString line1 =
      "1 25544U 98067A   26290.82202591  .00007563  00000+0  14569-3 0  9996";
  const QString line2 =
      "2 25545  51.6326  53.2215 0006765 277.4279  82.5940 15.48913056590762";

  QString error;
  const std::optional<Tle> tle = Tle::parse(line1, line2, &error);

  QVERIFY(!tle.has_value());
  QVERIFY(!error.isEmpty());
}

void TleTests::rejectsMalformedLines()
{
  QString error;
  const std::optional<Tle> tle = Tle::parse(
    "this is not a tle line",
    "2 25544  51.6326  53.2215 0006765 277.4279  82.5940 15.48913056590762",
    &error
  );

  QVERIFY(!tle.has_value());
  QVERIFY(!error.isEmpty());
}

void TleTests::rejectsMissingLines()
{
  QString error;
  const std::optional<Tle> tle = Tle::fromText(
    "ISS (ZARYA)\nno orbital data here\n",
    &error
  );

  QVERIFY(!tle.has_value());
  QVERIFY(!error.isEmpty());
}

void TleTests::minutesSinceEpochIsLinear()
{
  const QString line1 =
      "1 25544U 98067A   26290.82202591  .00007563  00000+0  14569-3 0  9996";
  const QString line2 =
      "2 25544  51.6326  53.2215 0006765 277.4279  82.5940 15.48913056590762";

  const std::optional<Tle> tle = Tle::parse(line1, line2);

  QVERIFY(tle.has_value());
  QVERIFY(qAbs(tle->minutesSinceEpoch(tle->epochUtc)) < 1.0e-4);

  const QDateTime oneHourLater =
      tle->epochUtc.addSecs(3600);
  QVERIFY(qAbs(tle->minutesSinceEpoch(oneHourLater) - 60.0) < 1.0e-4);

  const QDateTime oneDayEarlier =
      tle->epochUtc.addSecs(-86400);
  QVERIFY(qAbs(tle->minutesSinceEpoch(oneDayEarlier) + 1440.0) < 1.0e-4);
}

QTEST_MAIN(TleTests)

#include "test_tle.moc"
