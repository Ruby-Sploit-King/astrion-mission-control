#pragma once

#include <QDateTime>
#include <QString>
#include <QStringList>
#include <QVector>

struct TelemetryRecord
{
  QDateTime timestamp;
  double latitudeDeg = 0.0;
  double longitudeDeg = 0.0;
  double altitudeKm = 0.0;
  double velocityKmS = 0.0;
  bool sunlit = true;
};

class Telemetry
{
  public:
    static QStringList csvHeader();

    static QVector<TelemetryRecord> loadCsv(
      const QString& filePath,
      QString* error = nullptr
    );

    static bool saveCsv(
      const QString& filePath,
      const QVector<TelemetryRecord>& records,
      QString* error = nullptr
    );
};
