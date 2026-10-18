#pragma once

#include "tle.hpp"

#include <QString>

struct Vec3
{
  double x = 0.0;
  double y = 0.0;
  double z = 0.0;

  double magnitude() const;
  Vec3 operator+(const Vec3& other) const;
  Vec3 operator-(const Vec3& other) const;
  Vec3 operator*(double scalar) const;
  double dot(const Vec3& other) const;
  Vec3 cross(const Vec3& other) const;
};

class Sgp4
{
  public:
    bool init(const Tle& tle, QString* error = nullptr);

    bool propagate(
      double tsinceMinutes,
      Vec3& positionKm,
      Vec3& velocityKmS,
      QString* error = nullptr
    ) const;

    double jdEpoch() const;
    double periodMinutes() const;
    bool supports(const Tle& tle) const;

  private:
    bool initNearEarth(
      double epochDays1950,
      double bstar,
      double ecco,
      double argpo,
      double inclo,
      double mo,
      double noKozai,
      double nodeo,
      QString* error
    );

    double tumin_ = 0.0;
    double mu_ = 0.0;
    double radiusearthkm_ = 0.0;
    double xke_ = 0.0;
    double j2_ = 0.0;
    double j3_ = 0.0;
    double j4_ = 0.0;
    double j3oj2_ = 0.0;

    double jdEpoch_ = 0.0;

    double bstar_ = 0.0;
    double ecco_ = 0.0;
    double argpo_ = 0.0;
    double inclo_ = 0.0;
    double mo_ = 0.0;
    double noKozai_ = 0.0;
    double noUnkozai_ = 0.0;
    double nodeo_ = 0.0;

    int isimp_ = 0;

    double gsto_ = 0.0;

    double ainv_ = 0.0;
    double ao_ = 0.0;
    double con41_ = 0.0;
    double cosio_ = 0.0;
    double cosio2_ = 0.0;
    double eccsq_ = 0.0;
    double omeosq_ = 0.0;
    double posq_ = 0.0;
    double rp_ = 0.0;
    double rteosq_ = 0.0;
    double sinio_ = 0.0;

    double aycof_ = 0.0;
    double cc1_ = 0.0;
    double cc4_ = 0.0;
    double cc5_ = 0.0;
    double d2_ = 0.0;
    double d3_ = 0.0;
    double d4_ = 0.0;
    double delmo_ = 0.0;
    double eta_ = 0.0;
    double argpdot_ = 0.0;
    double omgcof_ = 0.0;
    double sinmao_ = 0.0;
    double t2cof_ = 0.0;
    double t3cof_ = 0.0;
    double t4cof_ = 0.0;
    double t5cof_ = 0.0;
    double x1mth2_ = 0.0;
    double x7thm1_ = 0.0;
    double mdot_ = 0.0;
    double nodedot_ = 0.0;
    double xlcof_ = 0.0;
    double xmcof_ = 0.0;
    double nodecf_ = 0.0;
};
