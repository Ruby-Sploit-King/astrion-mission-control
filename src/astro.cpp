#include "astro.hpp"

#include <cmath>

namespace
{
  constexpr double pi = 3.14159265358979323846;
  constexpr double twopi = 2.0 * pi;
  constexpr double deg2rad = pi / 180.0;

  constexpr double wgs84A = 6378.137;
  constexpr double wgs84F = 1.0 / 298.257223563;
  constexpr double shadowRadiusKm = 6378.135;

  double fmodPos(double value, double modulus)
  {
    double result = std::fmod(value, modulus);

    if (result < 0.0)
    {
      result += modulus;
    }

    return result;
  }
}

double Astro::julianDate(const QDateTime& whenUtc)
{
  return 2440587.5
      + static_cast<double>(whenUtc.toMSecsSinceEpoch()) / 86400000.0;
}

double Astro::gstime(double jdut1)
{
  const double tut1 = (jdut1 - 2451545.0) / 36525.0;
  double temp =
      -6.2e-6 * tut1 * tut1 * tut1
      + 0.093104 * tut1 * tut1
      + (876600.0 * 3600.0 + 8640184.812866) * tut1
      + 67310.54841;

  return fmodPos(temp * deg2rad / 240.0, twopi);
}

Vec3 Astro::temeToEcef(const Vec3& positionTeme, double gmstRadians)
{
  const double cosG = std::cos(gmstRadians);
  const double sinG = std::sin(gmstRadians);

  return {
    cosG * positionTeme.x + sinG * positionTeme.y,
    -sinG * positionTeme.x + cosG * positionTeme.y,
    positionTeme.z
  };
}

Astro::Geodetic Astro::ecefToGeodetic(const Vec3& positionEcef)
{
  const double e2 = wgs84F * (2.0 - wgs84F);
  const double longitude = std::atan2(
    positionEcef.y,
    positionEcef.x
  );
  const double p = std::hypot(
    positionEcef.x,
    positionEcef.y
  );

  double latitude = std::atan2(
    positionEcef.z,
    p * (1.0 - e2)
  );
  double altitude = 0.0;

  for (int i = 0; i < 6; ++i)
  {
    const double sinLat = std::sin(latitude);
    const double n = wgs84A
        / std::sqrt(1.0 - e2 * sinLat * sinLat);

    if (std::fabs(std::cos(latitude)) > 1.0e-12)
    {
      altitude = p / std::cos(latitude) - n;
    }
    else
    {
      altitude = std::fabs(positionEcef.z) - n * (1.0 - e2);
    }

    latitude = std::atan2(
      positionEcef.z,
      p * (1.0 - e2 * n / (n + altitude))
    );
  }

  Geodetic out;
  out.latitudeDeg = latitude / deg2rad;
  out.longitudeDeg = fmodPos(longitude / deg2rad + 540.0, 360.0) - 180.0;
  out.altitudeKm = altitude;
  return out;
}

Vec3 Astro::sunDirectionTeme(double jdut1)
{
  const double n = jdut1 - 2451545.0;

  const double meanLongitude =
      fmodPos((280.460 + 0.9856474 * n) * deg2rad, twopi);
  const double meanAnomaly =
      fmodPos((357.528 + 0.9856003 * n) * deg2rad, twopi);

  const double eclipticLongitude = meanLongitude
      + (1.915 * std::sin(meanAnomaly)
         + 0.020 * std::sin(2.0 * meanAnomaly)) * deg2rad;

  const double obliquity =
      (23.439 - 0.0000004 * n) * deg2rad;

  const double sinEcl = std::sin(eclipticLongitude);
  const double cosEcl = std::cos(eclipticLongitude);
  const double sinOb = std::sin(obliquity);

  const double declination = std::asin(sinOb * sinEcl);
  const double rightAscension = std::atan2(
    std::cos(obliquity) * sinEcl,
    cosEcl
  );

  const double cosDec = std::cos(declination);

  return {
    std::cos(rightAscension) * cosDec,
    std::sin(rightAscension) * cosDec,
    std::sin(declination)
  };
}

bool Astro::isSunlit(
  const Vec3& positionTeme,
  const Vec3& sunDirection
)
{
  const double alongSun = positionTeme.dot(sunDirection);

  if (alongSun >= 0.0)
  {
    return true;
  }

  const double radiusSquared = positionTeme.dot(positionTeme)
      - alongSun * alongSun;

  return radiusSquared >= shadowRadiusKm * shadowRadiusKm;
}
