#include "telemetry.hpp"

#include <QFile>
#include <QTextStream>
#include <QTimeZone>

#include <initializer_list>

namespace
{
  bool parseNumber(const QString& text, double& value)
  {
    bool ok = false;
    value = text.trimmed().toDouble(&ok);
    return ok;
  }

  bool parseBool(const QString& text, bool& value)
  {
    const QString normalized = text.trimmed().toLower();

    if (normalized == QLatin1String("1")
        || normalized == QLatin1String("true")
        || normalized == QLatin1String("sunlit")
        || normalized == QLatin1String("yes"))
    {
      value = true;
      return true;
    }

    if (normalized == QLatin1String("0")
        || normalized == QLatin1String("false")
        || normalized == QLatin1String("eclipse")
        || normalized == QLatin1String("no"))
    {
      value = false;
      return true;
    }

    return false;
  }

  bool parseRecord(const QStringList& fields, TelemetryRecord& record)
  {
    if (fields.size() != 6)
    {
      return false;
    }

    record.timestamp = QDateTime::fromString(
      fields.at(0).trimmed(),
      Qt::ISODateWithMs
    );

    if (!record.timestamp.isValid())
    {
      record.timestamp = QDateTime::fromString(
        fields.at(0).trimmed(),
        Qt::ISODate
      );
    }

    if (!record.timestamp.isValid())
    {
      return false;
    }

    record.timestamp.setTimeZone(QTimeZone::UTC);

    return parseNumber(fields.at(1), record.latitudeDeg)
        && parseNumber(fields.at(2), record.longitudeDeg)
        && parseNumber(fields.at(3), record.altitudeKm)
        && parseNumber(fields.at(4), record.velocityKmS)
        && parseBool(fields.at(5), record.sunlit)
        && record.latitudeDeg >= -90.0
        && record.latitudeDeg <= 90.0
        && record.longitudeDeg >= -180.0
        && record.longitudeDeg <= 180.0
        && record.altitudeKm >= 0.0
        && record.velocityKmS > 0.0;
  }
}

QStringList Telemetry::csvHeader()
{
  return {
    "timestamp",
    "latitude_deg",
    "longitude_deg",
    "altitude_km",
    "velocity_km_s",
    "sunlit"
  };
}

QVector<TelemetryRecord> Telemetry::loadCsv(
  const QString& filePath,
  QString* error
)
{
  if (error)
  {
    error->clear();
  }

  QFile file(filePath);

  if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
  {
    if (error)
    {
      *error = file.errorString();
    }

    return {};
  }

  QTextStream stream(&file);
  const QString headerLine = stream.readLine().trimmed();

  if (headerLine.split(',') != csvHeader())
  {
    if (error)
    {
      *error = "CSV header does not match the required format.";
    }

    return {};
  }

  QVector<TelemetryRecord> records;
  int lineNumber = 1;

  while (!stream.atEnd())
  {
    const QString line = stream.readLine().trimmed();
    ++lineNumber;

    if (line.isEmpty())
    {
      continue;
    }

    TelemetryRecord record;

    if (!parseRecord(line.split(','), record))
    {
      if (error)
      {
        *error = QString(
          "Invalid track record on line %1."
        ).arg(lineNumber);
      }

      return {};
    }

    records.append(record);
  }

  if (records.isEmpty())
  {
    if (error)
    {
      *error = "CSV contains no track records.";
    }

    return {};
  }

  return records;
}

bool Telemetry::saveCsv(
  const QString& filePath,
  const QVector<TelemetryRecord>& records,
  QString* error
)
{
  if (error)
  {
    error->clear();
  }

  QFile file(filePath);

  if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
  {
    if (error)
    {
      *error = file.errorString();
    }

    return false;
  }

  QTextStream stream(&file);
  stream << csvHeader().join(',') << '\n';

  for (const TelemetryRecord& record : records)
  {
    stream
      << record.timestamp.toUTC().toString(Qt::ISODateWithMs) << ','
      << QString::number(record.latitudeDeg, 'f', 5) << ','
      << QString::number(record.longitudeDeg, 'f', 5) << ','
      << QString::number(record.altitudeKm, 'f', 3) << ','
      << QString::number(record.velocityKmS, 'f', 4) << ','
      << (record.sunlit ? 1 : 0) << '\n';
  }

  if (stream.status() != QTextStream::Ok)
  {
    if (error)
    {
      *error = "Failed while writing the CSV file.";
    }

    return false;
  }

  return true;
}
