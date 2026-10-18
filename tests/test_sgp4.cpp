#include "astro.hpp"
#include "sgp4.hpp"
#include "tle.hpp"

#include <QtTest>

namespace goldenIss
{
#include "golden/golden_iss.h"
}

namespace goldenVer
{
#include "golden/golden_ver.h"
}

namespace
{
  constexpr double positionToleranceKm = 1.0e-5;
  constexpr double velocityToleranceKmS = 1.0e-8;

  template <typename GoldenCaseT>
  void runGoldenCase(const GoldenCaseT& golden)
  {
    QString error;
    const std::optional<Tle> tle = Tle::parse(
      golden.line1,
      golden.line2,
      &error
    );

    QVERIFY2(tle.has_value(), qPrintable(error));

    Sgp4 propagator;
    const bool initialized = propagator.init(*tle, &error);

    int epochIndex = -1;

    for (int i = 0; i < 7; ++i)
    {
      if (golden.tsinceMinutes[i] == 0.0)
      {
        epochIndex = i;
      }
    }

    QVERIFY(epochIndex >= 0);

    if (!initialized)
    {
      QVERIFY2(
        golden.expectedError[epochIndex] != 0,
        qPrintable(
          QString("Init failed for %1: %2")
            .arg(golden.name)
            .arg(error)
        )
      );
      return;
    }

    for (int i = 0; i < 7; ++i)
    {
      Vec3 position;
      Vec3 velocity;
      error.clear();
      const bool ok = propagator.propagate(
        golden.tsinceMinutes[i],
        position,
        velocity,
        &error
      );

      if (golden.expectedError[i] != 0)
      {
        QVERIFY2(
          !ok,
          qPrintable(
            QString("Expected error %1 for %2 at t=%3")
              .arg(golden.expectedError[i])
              .arg(golden.name)
              .arg(golden.tsinceMinutes[i])
          )
        );
        continue;
      }

      QVERIFY2(
        ok,
        qPrintable(
          QString("Propagation failed for %1 at t=%2: %3")
            .arg(golden.name)
            .arg(golden.tsinceMinutes[i])
            .arg(error)
        )
      );

      const double* expected = golden.expected[i];
      const double actual[6] = {
        position.x, position.y, position.z,
        velocity.x, velocity.y, velocity.z
      };

      for (int component = 0; component < 6; ++component)
      {
        const double tolerance = component < 3
            ? positionToleranceKm
            : velocityToleranceKmS;

        QVERIFY2(
          qAbs(actual[component] - expected[component]) <= tolerance,
          qPrintable(
            QString(
              "%1 t=%2 component=%3 expected=%4 actual=%5"
            )
              .arg(golden.name)
              .arg(golden.tsinceMinutes[i])
              .arg(component)
              .arg(expected[component], 0, 'g', 17)
              .arg(actual[component], 0, 'g', 17)
          )
        );
      }
    }
  }
}

class Sgp4Tests : public QObject
{
    Q_OBJECT

private slots:
    void issMatchesReference();
    void verificationCasesMatch();
    void issFieldsParseCorrectly();
    void julianDateAtJ2000();
    void gstimeAtJ2000();
    void geodeticRoundTrip();
    void geodeticKnownPoints();
    void sunDirectionAtJ2000();
    void shadowGeometry();
};

void Sgp4Tests::issMatchesReference()
{
  runGoldenCase(goldenIss::kGoldenCases[0]);
}

void Sgp4Tests::verificationCasesMatch()
{
  for (const goldenVer::GoldenCase& golden : goldenVer::kGoldenCases)
  {
    runGoldenCase(golden);
  }
}

void Sgp4Tests::issFieldsParseCorrectly()
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
  QVERIFY(qAbs(tle->ecco - 0.0006765) < 1.0e-12);
  QVERIFY(qAbs(tle->inclo * 180.0 / 3.14159265358979323846
               - 51.6326) < 1.0e-9);
  QVERIFY(qAbs(tle->noKozai * 1440.0 / (2.0 * 3.14159265358979323846)
               - 15.48913056) < 1.0e-9);
  QVERIFY(qAbs(tle->bstar - 1.4569e-4) < 1.0e-12);

  Sgp4 propagator;
  QVERIFY(propagator.init(*tle, &error));
  QVERIFY(propagator.periodMinutes() > 92.0);
  QVERIFY(propagator.periodMinutes() < 93.5);
}

void Sgp4Tests::julianDateAtJ2000()
{
  const QDateTime j2000(
    QDate(2000, 1, 1),
    QTime(12, 0, 0),
    Qt::UTC
  );

  QVERIFY(qAbs(Astro::julianDate(j2000) - 2451545.0) < 1.0e-9);
}

void Sgp4Tests::gstimeAtJ2000()
{
  const double expectedDeg = 280.460618375;
  const double actualDeg = Astro::gstime(2451545.0) * 180.0
      / 3.14159265358979323846;

  QVERIFY(qAbs(actualDeg - expectedDeg) < 1.0e-6);
}

void Sgp4Tests::geodeticRoundTrip()
{
  const double samples[][3] = {
    {51.6, -0.1, 420.0},
    {-33.9, 151.2, 15.0},
    {35.6, 139.7, 40.0},
    {0.0, 0.0, 400.0},
    {89.0, -120.0, 380.0}
  };

  const double a = 6378.137;
  const double f = 1.0 / 298.257223563;
  const double e2 = f * (2.0 - f);

  for (const auto& sample : samples)
  {
    const double lat = sample[0] * 3.14159265358979323846 / 180.0;
    const double lon = sample[1] * 3.14159265358979323846 / 180.0;
    const double alt = sample[2];
    const double sinLat = std::sin(lat);
    const double n = a / std::sqrt(1.0 - e2 * sinLat * sinLat);

    const Vec3 ecef = {
      (n + alt) * std::cos(lat) * std::cos(lon),
      (n + alt) * std::cos(lat) * std::sin(lon),
      (n * (1.0 - e2) + alt) * sinLat
    };

    const Astro::Geodetic geo = Astro::ecefToGeodetic(ecef);

    QVERIFY(qAbs(geo.latitudeDeg - sample[0]) < 1.0e-7);
    QVERIFY(qAbs(geo.longitudeDeg - sample[1]) < 1.0e-7);
    QVERIFY(qAbs(geo.altitudeKm - sample[2]) < 1.0e-6);
  }
}

void Sgp4Tests::geodeticKnownPoints()
{
  const Astro::Geodetic equator = Astro::ecefToGeodetic({6378.137, 0, 0});
  QVERIFY(qAbs(equator.latitudeDeg) < 1.0e-9);
  QVERIFY(qAbs(equator.longitudeDeg) < 1.0e-9);
  QVERIFY(qAbs(equator.altitudeKm) < 1.0e-6);

  const Astro::Geodetic pole = Astro::ecefToGeodetic({0, 0, 6356.752314245});
  QVERIFY(qAbs(pole.latitudeDeg - 90.0) < 1.0e-6);

  const Astro::Geodetic antimeridian =
      Astro::ecefToGeodetic({0, 6378.137, 0});
  QVERIFY(qAbs(antimeridian.longitudeDeg - 90.0) < 1.0e-9);
}

void Sgp4Tests::sunDirectionAtJ2000()
{
  const Vec3 sun = Astro::sunDirectionTeme(2451545.0);

  const double expected[3] = {
    0.18010164163354486,
    -0.9024813884014727,
    -0.39126812069200373
  };

  QVERIFY(qAbs(sun.x - expected[0]) < 1.0e-7);
  QVERIFY(qAbs(sun.y - expected[1]) < 1.0e-7);
  QVERIFY(qAbs(sun.z - expected[2]) < 1.0e-7);
  QVERIFY(qAbs(sun.magnitude() - 1.0) < 1.0e-12);
}

void Sgp4Tests::shadowGeometry()
{
  const Vec3 sun = {1.0, 0.0, 0.0};

  QVERIFY(Astro::isSunlit({7000.0, 0.0, 0.0}, sun));
  QVERIFY(Astro::isSunlit({0.0, 7000.0, 0.0}, sun));
  QVERIFY(!Astro::isSunlit({-7000.0, 0.0, 0.0}, sun));
  QVERIFY(!Astro::isSunlit({-5000.0, 100.0, 0.0}, sun));
  QVERIFY(!Astro::isSunlit({-5000.0, 2000.0, 0.0}, sun));
  QVERIFY(Astro::isSunlit({-5000.0, 20000.0, 0.0}, sun));
}

QTEST_MAIN(Sgp4Tests)

#include "test_sgp4.moc"
