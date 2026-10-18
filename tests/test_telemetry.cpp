#include "telemetry.hpp"

#include <QFile>
#include <QTemporaryDir>
#include <QTimeZone>
#include <QtTest>

class TelemetryTests : public QObject
{
    Q_OBJECT

private slots:
    void validCsvLoads();
    void validCsvParsesEclipseFlag();
    void invalidHeaderIsRejected();
    void invalidRecordIsRejected();
    void outOfRangeLatitudeIsRejected();
    void saveCsvRoundTrips();
};

void TelemetryTests::validCsvLoads()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString path =
        directory.filePath("valid-track.csv");

    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));

    const QByteArray csv =
        "timestamp,latitude_deg,longitude_deg,"
        "altitude_km,velocity_km_s,sunlit\n"
        "2026-10-17T12:00:00Z,51.6326,53.2215,"
        "420.125,7.660,1\n"
        "2026-10-17T12:00:15Z,52.1043,54.8901,"
        "421.004,7.661,1\n";

    QCOMPARE(file.write(csv), qint64(csv.size()));
    file.close();

    QString error;
    const QVector<TelemetryRecord> records =
        Telemetry::loadCsv(path, &error);

    QVERIFY2(error.isEmpty(), qPrintable(error));
    QCOMPARE(records.size(), 2);
    QVERIFY(qAbs(records.first().latitudeDeg - 51.6326) < 1e-9);
    QVERIFY(qAbs(records.first().altitudeKm - 420.125) < 1e-9);
    QVERIFY(records.first().sunlit);
}

void TelemetryTests::validCsvParsesEclipseFlag()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString path =
        directory.filePath("eclipse-track.csv");

    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));

    const QByteArray csv =
        "timestamp,latitude_deg,longitude_deg,"
        "altitude_km,velocity_km_s,sunlit\n"
        "2026-10-17T12:00:00Z,51.6326,53.2215,420.125,7.660,0\n";

    file.write(csv);
    file.close();

    QString error;
    const QVector<TelemetryRecord> records =
        Telemetry::loadCsv(path, &error);

    QVERIFY2(error.isEmpty(), qPrintable(error));
    QCOMPARE(records.size(), 1);
    QVERIFY(!records.first().sunlit);
}

void TelemetryTests::invalidHeaderIsRejected()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString path =
        directory.filePath("invalid-header.csv");

    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));

    const QByteArray csv =
        "time,lat,lon,alt,speed,light\n"
        "2026-10-17T12:00:00Z,51,53,420,7.6,1\n";

    QCOMPARE(file.write(csv), qint64(csv.size()));
    file.close();

    QString error;
    const QVector<TelemetryRecord> records =
        Telemetry::loadCsv(path, &error);

    QVERIFY(records.isEmpty());
    QVERIFY(!error.isEmpty());
}

void TelemetryTests::invalidRecordIsRejected()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString path =
        directory.filePath("invalid-record.csv");

    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));

    const QByteArray csv =
        "timestamp,latitude_deg,longitude_deg,"
        "altitude_km,velocity_km_s,sunlit\n"
        "2026-10-17T12:00:00Z,not-a-number,53.2215,420.125,7.660,1\n";

    QCOMPARE(file.write(csv), qint64(csv.size()));
    file.close();

    QString error;
    const QVector<TelemetryRecord> records =
        Telemetry::loadCsv(path, &error);

    QVERIFY(records.isEmpty());
    QVERIFY(!error.isEmpty());
}

void TelemetryTests::outOfRangeLatitudeIsRejected()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString path =
        directory.filePath("range-record.csv");

    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));

    const QByteArray csv =
        "timestamp,latitude_deg,longitude_deg,"
        "altitude_km,velocity_km_s,sunlit\n"
        "2026-10-17T12:00:00Z,120.0,53.2215,420.125,7.660,1\n";

    file.write(csv);
    file.close();

    QString error;
    const QVector<TelemetryRecord> records =
        Telemetry::loadCsv(path, &error);

    QVERIFY(records.isEmpty());
    QVERIFY(!error.isEmpty());
}

void TelemetryTests::saveCsvRoundTrips()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString path =
        directory.filePath("round-trip.csv");

    QVector<TelemetryRecord> records;

    TelemetryRecord first;
    first.timestamp = QDateTime(
        QDate(2026, 10, 17), QTime(12, 0, 0), QTimeZone::UTC
    );
    first.latitudeDeg = 51.6326;
    first.longitudeDeg = 53.2215;
    first.altitudeKm = 420.125;
    first.velocityKmS = 7.660;
    first.sunlit = true;
    records.append(first);

    TelemetryRecord second;
    second.timestamp = QDateTime(
        QDate(2026, 10, 17), QTime(12, 0, 15), QTimeZone::UTC
    );
    second.latitudeDeg = -33.9;
    second.longitudeDeg = -70.5;
    second.altitudeKm = 418.5;
    second.velocityKmS = 7.658;
    second.sunlit = false;
    records.append(second);

    QString error;
    QVERIFY2(
        Telemetry::saveCsv(path, records, &error),
        qPrintable(error)
    );

    const QVector<TelemetryRecord> loaded =
        Telemetry::loadCsv(path, &error);

    QVERIFY2(error.isEmpty(), qPrintable(error));
    QCOMPARE(loaded.size(), records.size());
    QCOMPARE(
        loaded.first().timestamp.toUTC(),
        records.first().timestamp.toUTC()
    );
    QVERIFY(qAbs(loaded.first().latitudeDeg - 51.6326) < 1e-5);
    QVERIFY(qAbs(loaded.last().longitudeDeg + 70.5) < 1e-5);
    QCOMPARE(loaded.last().sunlit, false);
}

QTEST_MAIN(TelemetryTests)

#include "test_telemetry.moc"
