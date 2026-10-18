#pragma once

#include "sgp4.hpp"

#include <QDateTime>

namespace Astro
{
  struct Geodetic
  {
    double latitudeDeg = 0.0;
    double longitudeDeg = 0.0;
    double altitudeKm = 0.0;
  };

  double julianDate(const QDateTime& whenUtc);

  double gstime(double jdut1);

  Vec3 temeToEcef(const Vec3& positionTeme, double gmstRadians);

  Geodetic ecefToGeodetic(const Vec3& positionEcef);

  Vec3 sunDirectionTeme(double jdut1);

  bool isSunlit(const Vec3& positionTeme, const Vec3& sunDirection);
}
