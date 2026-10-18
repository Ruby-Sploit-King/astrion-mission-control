#include "tle.hpp"

#include <QTimeZone>

#include <cmath>

namespace
{
  constexpr double deg2rad = 3.14159265358979323846 / 180.0;
  constexpr double xpdotp = 1440.0 / (2.0 * 3.14159265358979323846);

  QString padded(const QString& line)
  {
    QString result = line;
    while (result.size() < 69)
    {
      result.append(QLatin1Char(' '));
    }
    return result;
  }

  bool hasTleColumns(const QString& line)
  {
    return line.size() >= 64
        && line.startsWith(QLatin1String("1 "))
        && line[8] == QLatin1Char(' ')
        && line[23] == QLatin1Char('.')
        && line[32] == QLatin1Char(' ')
        && line[34] == QLatin1Char('.')
        && line[43] == QLatin1Char(' ')
        && line[52] == QLatin1Char(' ')
        && line[61] == QLatin1Char(' ')
        && line[63] == QLatin1Char(' ');
  }

  bool hasTle2Columns(const QString& line)
  {
    return line.size() >= 68
        && line.startsWith(QLatin1String("2 "))
        && line[7] == QLatin1Char(' ')
        && line[11] == QLatin1Char('.')
        && line[16] == QLatin1Char(' ')
        && line[20] == QLatin1Char('.')
        && line[25] == QLatin1Char(' ')
        && line[33] == QLatin1Char(' ')
        && line[37] == QLatin1Char('.')
        && line[42] == QLatin1Char(' ')
        && line[46] == QLatin1Char('.')
        && line[51] == QLatin1Char(' ');
  }

  bool parseReal(const QString& field, double& value)
  {
    bool ok = false;
    value = field.trimmed().toDouble(&ok);
    return ok;
  }

  bool parseInt(const QString& field, int& value)
  {
    bool ok = false;
    value = field.trimmed().toInt(&ok);
    return ok;
  }

  double daysFromCivil(int year, int month, int day)
  {
    year -= month <= 2;
    const int era = (year >= 0 ? year : year - 399) / 400;
    const unsigned yoe = static_cast<unsigned>(year - era * 400);
    const unsigned doy =
        (153u * static_cast<unsigned>(month + (month > 2 ? -3 : 9)) + 2u) / 5u
        + static_cast<unsigned>(day) - 1u;
    const unsigned doe =
        yoe * 365u + yoe / 4u - yoe / 100u + doy;
    return static_cast<double>(era * 146097 + static_cast<int>(doe))
        - 719468.0;
  }

  QDateTime julianToUtc(double jd)
  {
    const double mjd = jd - 2400000.5;
    const qint64 msecs =
        static_cast<qint64>(std::llround((mjd - 40587.0) * 86400000.0));
    return QDateTime::fromMSecsSinceEpoch(msecs, QTimeZone::UTC);
  }
}

std::optional<Tle> Tle::parse(
  const QString& line1In,
  const QString& line2In,
  QString* error
)
{
  const QString line1 = padded(line1In.trimmed());
  const QString line2 = padded(line2In.trimmed());

  if (!hasTleColumns(line1) || !hasTle2Columns(line2))
  {
    if (error)
    {
      *error = "TLE lines do not match the standard two-line format.";
    }
    return std::nullopt;
  }

  Tle tle;

  tle.satnum = line1.mid(2, 5).trimmed().toInt();

  int twoDigitYear = 0;
  double epochDays = 0.0;
  int nExp = 0;
  int bExp = 0;

  bool ok = parseInt(line1.mid(18, 2), twoDigitYear)
         && parseReal(line1.mid(20, 12), epochDays)
         && parseReal(line1.mid(33, 10), tle.ndot)
         && parseInt(line1.mid(50, 2), nExp)
         && parseInt(line1.mid(59, 2), bExp);

  if (!ok)
  {
    if (error)
    {
      *error = "TLE line 1 contains malformed numeric fields.";
    }
    return std::nullopt;
  }

  double mantissa = 0.0;
  ok = parseReal(
         line1.mid(44, 1) + QLatin1Char('.')
             + line1.mid(45, 5).replace(QLatin1Char(' '), QLatin1Char('0')),
         mantissa
       );

  if (ok)
  {
    tle.nddot = mantissa * std::pow(10.0, nExp);
  }

  double bstarMantissa = 0.0;
  ok = parseReal(
         line1.mid(53, 1) + QLatin1Char('.')
             + line1.mid(54, 5).replace(QLatin1Char(' '), QLatin1Char('0')),
         bstarMantissa
       );

  if (ok)
  {
    tle.bstar = bstarMantissa * std::pow(10.0, bExp);
  }

  if (line2.mid(2, 5).trimmed().toInt() != tle.satnum)
  {
    if (error)
    {
      *error = "TLE object numbers in lines 1 and 2 do not match.";
    }
    return std::nullopt;
  }

  QString eccoText = QStringLiteral("0.") 
      + line2.mid(26, 7).replace(QLatin1Char(' '), QLatin1Char('0'));

  ok = parseReal(line2.mid(8, 8), tle.inclo)
    && parseReal(line2.mid(17, 8), tle.nodeo)
    && parseReal(eccoText, tle.ecco)
    && parseReal(line2.mid(34, 8), tle.argpo)
    && parseReal(line2.mid(43, 8), tle.mo)
    && parseReal(line2.mid(52, 11), tle.noKozai)
    && parseInt(line2.mid(63, 5), tle.revnum);

  if (!ok)
  {
    if (error)
    {
      *error = "TLE line 2 contains malformed numeric fields.";
    }
    return std::nullopt;
  }

  tle.noKozai /= xpdotp;

  tle.inclo *= deg2rad;
  tle.nodeo *= deg2rad;
  tle.argpo *= deg2rad;
  tle.mo *= deg2rad;

  int year = 0;

  if (twoDigitYear < 57)
  {
    year = twoDigitYear + 2000;
  }
  else
  {
    year = twoDigitYear + 1900;
  }

  const double jdJan1 =
      daysFromCivil(year, 1, 1) + 2440587.5;
  tle.jdEpoch = jdJan1 + (epochDays - 1.0);
  tle.epochUtc = julianToUtc(tle.jdEpoch);

  return tle;
}

std::optional<Tle> Tle::fromText(const QString& text, QString* error)
{
  QStringList rawLines = text.split(QLatin1Char('\n'));

  QStringList lines;

  for (const QString& raw : rawLines)
  {
    const QString trimmed = raw.trimmed();

    if (!trimmed.isEmpty())
    {
      lines.append(trimmed);
    }
  }

  int start = 0;

  if (lines.size() >= 3 && !lines[0].startsWith(QLatin1String("1 ")))
  {
    start = 1;
  }

  if (lines.size() - start < 2)
  {
    if (error)
    {
      *error = "Orbital data does not contain two TLE lines.";
    }
    return std::nullopt;
  }

  std::optional<Tle> tle =
      parse(lines[start], lines[start + 1], error);

  if (tle && start > 0)
  {
    tle->name = lines[0];
  }

  return tle;
}

double Tle::minutesSinceEpoch(const QDateTime& whenUtc) const
{
  const double jd = 2440587.5
      + static_cast<double>(whenUtc.toMSecsSinceEpoch()) / 86400000.0;
  return (jd - jdEpoch) * 1440.0;
}
