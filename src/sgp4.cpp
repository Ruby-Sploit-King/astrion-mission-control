#include "sgp4.hpp"

#include <cmath>

namespace
{
  constexpr double pi = 3.14159265358979323846;
  constexpr double twopi = 2.0 * pi;
  constexpr double deg2rad = pi / 180.0;
  constexpr double x2o3 = 2.0 / 3.0;
  constexpr double temp4Threshold = 1.5e-12;

  double fmodPos(double value, double modulus)
  {
    double result = std::fmod(value, modulus);

    if (result < 0.0)
    {
      result += modulus;
    }

    return result;
  }

  void getGravConstantsWgs72(
    double& tumin,
    double& mu,
    double& radiusearthkm,
    double& xke,
    double& j2,
    double& j3,
    double& j4,
    double& j3oj2
  )
  {
    mu = 398600.8;
    radiusearthkm = 6378.135;
    xke = 60.0 / std::sqrt(
      radiusearthkm * radiusearthkm * radiusearthkm / mu
    );
    tumin = 1.0 / xke;
    j2 = 0.001082616;
    j3 = -0.00000253881;
    j4 = -0.00000165597;
    j3oj2 = j3 / j2;
  }

  double gstime(double jdut1)
  {
    const double tut1 = (jdut1 - 2451545.0) / 36525.0;
    double temp =
        -6.2e-6 * tut1 * tut1 * tut1
        + 0.093104 * tut1 * tut1
        + (876600.0 * 3600.0 + 8640184.812866) * tut1
        + 67310.54841;

    temp = fmodPos(temp * deg2rad / 240.0, twopi);
    return temp;
  }

  struct InitlResult
  {
    double no = 0.0;
    double ainv = 0.0;
    double ao = 0.0;
    double con41 = 0.0;
    double con42 = 0.0;
    double cosio = 0.0;
    double cosio2 = 0.0;
    double eccsq = 0.0;
    double omeosq = 0.0;
    double posq = 0.0;
    double rp = 0.0;
    double rteosq = 0.0;
    double sinio = 0.0;
    double gsto = 0.0;
  };

  InitlResult initl(
    double xke,
    double j2,
    double ecco,
    double epochDays1950,
    double inclo,
    double noKozai
  )
  {
    InitlResult out;

    const double eccsq = ecco * ecco;
    const double omeosq = 1.0 - eccsq;
    const double rteosq = std::sqrt(omeosq);
    const double cosio = std::cos(inclo);
    const double cosio2 = cosio * cosio;

    const double ak = std::pow(xke / noKozai, x2o3);
    const double d1 =
        0.75 * j2 * (3.0 * cosio2 - 1.0) / (rteosq * omeosq);
    double del = d1 / (ak * ak);
    const double adel = ak * (
        1.0
        - del * del
        - del * (1.0 / 3.0 + 134.0 * del * del / 81.0)
    );
    del = d1 / (adel * adel);

    out.no = noKozai / (1.0 + del);
    out.ao = std::pow(xke / out.no, x2o3);

    const double sinio = std::sin(inclo);
    const double po = out.ao * omeosq;
    const double con42 = 1.0 - 5.0 * cosio2;

    out.ainv = 1.0 / out.ao;
    out.posq = po * po;
    out.rp = out.ao * (1.0 - ecco);
    out.con41 = -con42 - cosio2 - cosio2;
    out.cosio = cosio;
    out.cosio2 = cosio2;
    out.eccsq = eccsq;
    out.omeosq = omeosq;
    out.rteosq = rteosq;
    out.sinio = sinio;
    out.con42 = con42;
    out.gsto = gstime(epochDays1950 + 2433281.5);

    return out;
  }
}

double Vec3::magnitude() const
{
  return std::sqrt(x * x + y * y + z * z);
}

Vec3 Vec3::operator+(const Vec3& other) const
{
  return {x + other.x, y + other.y, z + other.z};
}

Vec3 Vec3::operator-(const Vec3& other) const
{
  return {x - other.x, y - other.y, z - other.z};
}

Vec3 Vec3::operator*(double scalar) const
{
  return {x * scalar, y * scalar, z * scalar};
}

double Vec3::dot(const Vec3& other) const
{
  return x * other.x + y * other.y + z * other.z;
}

Vec3 Vec3::cross(const Vec3& other) const
{
  return {
    y * other.z - z * other.y,
    z * other.x - x * other.z,
    x * other.y - y * other.x
  };
}

bool Sgp4::supports(const Tle& tle) const
{
  return twopi / tle.noKozai < 225.0;
}

bool Sgp4::init(const Tle& tle, QString* error)
{
  if (!supports(tle))
  {
    if (error)
    {
      *error = "Deep-space orbits are not supported by this propagator.";
    }

    return false;
  }

  getGravConstantsWgs72(
    tumin_, mu_, radiusearthkm_, xke_,
    j2_, j3_, j4_, j3oj2_
  );

  jdEpoch_ = tle.jdEpoch;
  bstar_ = tle.bstar;
  ecco_ = tle.ecco;
  argpo_ = tle.argpo;
  inclo_ = tle.inclo;
  mo_ = tle.mo;
  noKozai_ = tle.noKozai;
  nodeo_ = tle.nodeo;

  return initNearEarth(
    tle.jdEpoch - 2433281.5,
    bstar_, ecco_, argpo_, inclo_, mo_, noKozai_, nodeo_,
    error
  );
}

bool Sgp4::initNearEarth(
  double epochDays1950,
  double bstar,
  double ecco,
  double argpo,
  double inclo,
  double mo,
  double noKozai,
  double nodeo,
  QString* error
)
{
  isimp_ = 0;

  const double ss = 78.0 / radiusearthkm_ + 1.0;
  const double qzms2ttemp = (120.0 - 78.0) / radiusearthkm_;
  const double qzms2t =
      qzms2ttemp * qzms2ttemp * qzms2ttemp * qzms2ttemp;

  const InitlResult init = initl(
    xke_, j2_, ecco, epochDays1950, inclo, noKozai
  );

  noUnkozai_ = init.no;
  ainv_ = init.ainv;
  ao_ = init.ao;
  con41_ = init.con41;
  cosio_ = init.cosio;
  cosio2_ = init.cosio2;
  eccsq_ = init.eccsq;
  omeosq_ = init.omeosq;
  posq_ = init.posq;
  rp_ = init.rp;
  rteosq_ = init.rteosq;
  sinio_ = init.sinio;
  gsto_ = init.gsto;

  if (!(omeosq_ >= 0.0 || noUnkozai_ >= 0.0))
  {
    if (error)
    {
      *error = "Epoch orbital elements are invalid.";
    }

    return false;
  }

  isimp_ = 0;

  if (rp_ < 220.0 / radiusearthkm_ + 1.0)
  {
    isimp_ = 1;
  }

  double sfour = ss;
  double qzms24 = qzms2t;
  const double perige = (rp_ - 1.0) * radiusearthkm_;

  if (perige < 156.0)
  {
    sfour = perige - 78.0;

    if (perige < 98.0)
    {
      sfour = 20.0;
    }

    const double qzms24temp = (120.0 - sfour) / radiusearthkm_;
    qzms24 =
        qzms24temp * qzms24temp * qzms24temp * qzms24temp;
    sfour = sfour / radiusearthkm_ + 1.0;
  }

  const double pinvsq = 1.0 / posq_;
  const double tsi = 1.0 / (ao_ - sfour);
  eta_ = ao_ * ecco * tsi;
  const double etasq = eta_ * eta_;
  const double eeta = ecco * eta_;
  const double psisq = std::fabs(1.0 - etasq);
  const double coef = qzms24 * std::pow(tsi, 4.0);
  const double coef1 = coef / std::pow(psisq, 3.5);

  const double cc2 = coef1 * noUnkozai_ * (
      ao_ * (1.0 + 1.5 * etasq + eeta * (4.0 + etasq))
      + 0.375 * j2_ * tsi / psisq * con41_
          * (8.0 + 3.0 * etasq * (8.0 + etasq))
  );

  cc1_ = bstar * cc2;

  double cc3 = 0.0;

  if (ecco > 1.0e-4)
  {
    cc3 = -2.0 * coef * tsi * j3oj2_ * noUnkozai_
        * sinio_ / ecco;
  }

  x1mth2_ = 1.0 - cosio2_;

  cc4_ = 2.0 * noUnkozai_ * coef1 * ao_ * omeosq_ * (
      eta_ * (2.0 + 0.5 * etasq)
      + ecco * (0.5 + 2.0 * etasq)
      - j2_ * tsi / (ao_ * psisq) * (
          -3.0 * con41_ * (
              1.0 - 2.0 * eeta + etasq * (1.5 - 0.5 * eeta)
          )
          + 0.75 * x1mth2_
              * (2.0 * etasq - eeta * (1.0 + etasq))
              * std::cos(2.0 * argpo)
      )
  );

  cc5_ = 2.0 * coef1 * ao_ * omeosq_ * (
      1.0 + 2.75 * (etasq + eeta) + eeta * etasq
  );

  const double cosio4 = cosio2_ * cosio2_;
  const double temp1 = 1.5 * j2_ * pinvsq * noUnkozai_;
  const double temp2 = 0.5 * temp1 * j2_ * pinvsq;
  const double temp3 =
      -0.46875 * j4_ * pinvsq * pinvsq * noUnkozai_;

  mdot_ = noUnkozai_
      + 0.5 * temp1 * rteosq_ * con41_
      + 0.0625 * temp2 * rteosq_
          * (13.0 - 78.0 * cosio2_ + 137.0 * cosio4);

  argpdot_ = -0.5 * temp1 * init.con42
      + 0.0625 * temp2
          * (7.0 - 114.0 * cosio2_ + 395.0 * cosio4)
      + temp3 * (3.0 - 36.0 * cosio2_ + 49.0 * cosio4);

  const double xhdot1 = -temp1 * cosio_;

  nodedot_ = xhdot1
      + (0.5 * temp2 * (4.0 - 19.0 * cosio2_)
         + 2.0 * temp3 * (3.0 - 7.0 * cosio2_)) * cosio_;

  omgcof_ = bstar * cc3 * std::cos(argpo);

  xmcof_ = 0.0;

  if (ecco > 1.0e-4)
  {
    xmcof_ = -x2o3 * coef * bstar / eeta;
  }

  nodecf_ = 3.5 * omeosq_ * xhdot1 * cc1_;
  t2cof_ = 1.5 * cc1_;

  if (std::fabs(cosio_ + 1.0) > temp4Threshold)
  {
    xlcof_ = -0.25 * j3oj2_ * sinio_
        * (3.0 + 5.0 * cosio_) / (1.0 + cosio_);
  }
  else
  {
    xlcof_ = -0.25 * j3oj2_ * sinio_
        * (3.0 + 5.0 * cosio_) / temp4Threshold;
  }

  aycof_ = -0.5 * j3oj2_ * sinio_;

  const double delmotemp = 1.0 + eta_ * std::cos(mo);
  delmo_ = delmotemp * delmotemp * delmotemp;
  sinmao_ = std::sin(mo);
  x7thm1_ = 7.0 * cosio2_ - 1.0;

  d2_ = 0.0;
  d3_ = 0.0;
  d4_ = 0.0;
  t3cof_ = 0.0;
  t4cof_ = 0.0;
  t5cof_ = 0.0;

  if (isimp_ != 1)
  {
    const double cc1sq = cc1_ * cc1_;
    d2_ = 4.0 * ao_ * tsi * cc1sq;

    const double temp = d2_ * tsi * cc1_ / 3.0;
    d3_ = (17.0 * ao_ + sfour) * temp;
    d4_ = 0.5 * temp * ao_ * tsi
        * (221.0 * ao_ + 31.0 * sfour) * cc1_;

    t3cof_ = d2_ + 2.0 * cc1sq;
    t4cof_ = 0.25
        * (3.0 * d3_ + cc1_ * (12.0 * d2_ + 10.0 * cc1sq));
    t5cof_ = 0.2 * (
        3.0 * d4_
        + 12.0 * cc1_ * d3_
        + 6.0 * d2_ * d2_
        + 15.0 * cc1sq * (2.0 * d2_ + cc1sq)
    );
  }

  QString ignore;
  Vec3 position;
  Vec3 velocity;

  if (!propagate(0.0, position, velocity, &ignore))
  {
    if (error)
    {
      *error = "Satellite orbit is invalid at the TLE epoch.";
    }

    return false;
  }

  return true;
}

bool Sgp4::propagate(
  double tsince,
  Vec3& position,
  Vec3& velocity,
  QString* error
) const
{
  const double vkmpersec = radiusearthkm_ * xke_ / 60.0;

  const double xmdf = mo_ + mdot_ * tsince;
  const double argpdf = argpo_ + argpdot_ * tsince;
  const double nodedf = nodeo_ + nodedot_ * tsince;
  double argpm = argpdf;
  double mm = xmdf;
  const double t2 = tsince * tsince;
  double nodem = nodedf + nodecf_ * t2;
  double tempa = 1.0 - cc1_ * tsince;
  double tempe = bstar_ * cc4_ * tsince;
  double templ = t2cof_ * t2;

  if (isimp_ != 1)
  {
    const double delomg = omgcof_ * tsince;
    const double delmtemp = 1.0 + eta_ * std::cos(xmdf);
    const double delm = xmcof_ * (
        delmtemp * delmtemp * delmtemp - delmo_
    );
    const double temp = delomg + delm;
    mm = xmdf + temp;
    argpm = argpdf - temp;
    const double t3 = t2 * tsince;
    const double t4 = t3 * tsince;
    tempa = tempa - d2_ * t2 - d3_ * t3 - d4_ * t4;
    tempe = tempe + bstar_ * cc5_ * (std::sin(mm) - sinmao_);
    templ = templ + t3cof_ * t3
        + t4 * (t4cof_ + tsince * t5cof_);
  }

  double nm = noUnkozai_;
  double em = ecco_;
  const double inclm = inclo_;

  if (nm <= 0.0)
  {
    if (error)
    {
      *error = "Mean motion is less than zero.";
    }

    return false;
  }

  double am = std::pow(xke_ / nm, x2o3) * tempa * tempa;
  nm = xke_ / std::pow(am, 1.5);
  em = em - tempe;

  if (em >= 1.0 || em < -0.001)
  {
    if (error)
    {
      *error = "Mean eccentricity is outside 0.0 <= e < 1.0.";
    }

    return false;
  }

  if (em < 1.0e-6)
  {
    em = 1.0e-6;
  }

  mm = mm + noUnkozai_ * templ;
  double xlm = mm + argpm + nodem;
  const double emsq = em * em;
  const double temp = 1.0 - emsq;

  nodem = fmodPos(nodem, twopi);
  argpm = fmodPos(argpm, twopi);
  xlm = fmodPos(xlm, twopi);
  mm = fmodPos(xlm - argpm - nodem, twopi);

  const double sinim = std::sin(inclm);
  const double cosim = std::cos(inclm);

  const double ep = em;
  const double xincp = inclm;
  const double argpp = argpm;
  const double nodep = nodem;
  const double mp = mm;
  const double sinip = sinim;
  const double cosip = cosim;

  const double axnl = ep * std::cos(argpp);
  double temp1d = 1.0 / (am * (1.0 - ep * ep));
  const double aynl = ep * std::sin(argpp) + temp1d * aycof_;
  const double xl = mp + argpp + nodep + temp1d * xlcof_ * axnl;

  const double u = fmodPos(xl - nodep, twopi);
  double eo1 = u;
  double tem5 = 9999.9;
  int ktr = 1;
  double sineo1 = 0.0;
  double coseo1 = 0.0;

  while (std::fabs(tem5) >= 1.0e-12 && ktr <= 10)
  {
    sineo1 = std::sin(eo1);
    coseo1 = std::cos(eo1);
    tem5 = 1.0 - coseo1 * axnl - sineo1 * aynl;
    tem5 = (u - aynl * coseo1 + axnl * sineo1 - eo1) / tem5;

    if (std::fabs(tem5) >= 0.95)
    {
      tem5 = tem5 > 0.0 ? 0.95 : -0.95;
    }

    eo1 = eo1 + tem5;
    ktr = ktr + 1;
  }

  const double ecose = axnl * coseo1 + aynl * sineo1;
  const double esine = axnl * sineo1 - aynl * coseo1;
  const double el2 = axnl * axnl + aynl * aynl;
  const double pl = am * (1.0 - el2);

  if (pl < 0.0)
  {
    if (error)
    {
      *error = "Semi-latus rectum is less than zero.";
    }

    return false;
  }

  const double rl = am * (1.0 - ecose);
  const double rdotl = std::sqrt(am) * esine / rl;
  const double rvdotl = std::sqrt(pl) / rl;
  const double betal = std::sqrt(1.0 - el2);
  temp1d = esine / (1.0 + betal);
  const double sinu = am / rl * (sineo1 - aynl - axnl * temp1d);
  const double cosu = am / rl * (coseo1 - axnl + aynl * temp1d);
  double su = std::atan2(sinu, cosu);
  const double sin2u = (cosu + cosu) * sinu;
  const double cos2u = 1.0 - 2.0 * sinu * sinu;
  const double temp2 = 1.0 / pl;
  const double temp1 = 0.5 * j2_ * temp2;
  const double temp3 = temp1 * temp2;

  const double mrt = rl * (1.0 - 1.5 * temp3 * betal * con41_)
      + 0.5 * temp1 * x1mth2_ * cos2u;

  su = su - 0.25 * temp3 * x7thm1_ * sin2u;
  const double xnode = nodep + 1.5 * temp3 * cosip * sin2u;
  const double xinc = xincp + 1.5 * temp3 * cosip * sinip * cos2u;
  const double mvt = rdotl
      - nm * temp1 * x1mth2_ * sin2u / xke_;
  const double rvdot = rvdotl
      + nm * temp1 * (x1mth2_ * cos2u + 1.5 * con41_) / xke_;

  const double sinsu = std::sin(su);
  const double cossu = std::cos(su);
  const double snod = std::sin(xnode);
  const double cnod = std::cos(xnode);
  const double sini = std::sin(xinc);
  const double cosi = std::cos(xinc);
  const double xmx = -snod * cosi;
  const double xmy = cnod * cosi;
  const double ux = xmx * sinsu + cnod * cossu;
  const double uy = xmy * sinsu + snod * cossu;
  const double uz = sini * sinsu;
  const double vx = xmx * cossu - cnod * sinsu;
  const double vy = xmy * cossu - snod * sinsu;
  const double vz = sini * cossu;

  const double mr = mrt * radiusearthkm_;
  position = {mr * ux, mr * uy, mr * uz};
  velocity = {
    (mvt * ux + rvdot * vx) * vkmpersec,
    (mvt * uy + rvdot * vy) * vkmpersec,
    (mvt * uz + rvdot * vz) * vkmpersec
  };

  if (mrt < 1.0)
  {
    if (error)
    {
      *error = "Satellite has decayed below the Earth surface.";
    }

    return false;
  }

  return true;
}

double Sgp4::jdEpoch() const
{
  return jdEpoch_;
}

double Sgp4::periodMinutes() const
{
  return twopi / noUnkozai_;
}
