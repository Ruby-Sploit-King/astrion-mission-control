#pragma once

#include <QDateTime>
#include <QString>

#include <optional>

struct Tle
{
  int satnum = 0;
  QString name;

  QDateTime epochUtc;
  double jdEpoch = 0.0;

  double ndot = 0.0;
  double nddot = 0.0;
  double bstar = 0.0;

  double ecco = 0.0;
  double inclo = 0.0;
  double nodeo = 0.0;
  double argpo = 0.0;
  double mo = 0.0;

  double noKozai = 0.0;
  int revnum = 0;

  static std::optional<Tle> parse(
    const QString& line1,
    const QString& line2,
    QString* error = nullptr
  );

  static std::optional<Tle> fromText(
    const QString& text,
    QString* error = nullptr
  );

  double minutesSinceEpoch(const QDateTime& whenUtc) const;
};
