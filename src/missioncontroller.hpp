#pragma once

#include "sgp4.hpp"

#include <QDateTime>
#include <QObject>
#include <QPointF>
#include <QVector>

class QTimer;
class TleSource;

struct TrackSample
{
  QDateTime timeUtc;
  double latitudeDeg = 0.0;
  double longitudeDeg = 0.0;
  double altitudeKm = 0.0;
  double velocityKmS = 0.0;
  bool sunlit = true;
};

struct MissionState
{
  bool valid = false;
  QDateTime timeUtc;

  double latitudeDeg = 0.0;
  double longitudeDeg = 0.0;
  double altitudeKm = 0.0;
  double velocityKmS = 0.0;
  bool sunlit = true;

  double periodMinutes = 0.0;
  double tleAgeHours = 0.0;
  QString satelliteName;
  int satnum = 0;
  QString dataSource;

  QVector<QPointF> groundTrack;
  int currentTrackIndex = 0;
  QVector<QPointF> trackPast;
  QVector<QPointF> trackFuture;

  double sunSubLatitudeDeg = 0.0;
  double sunSubLongitudeDeg = 0.0;
};

Q_DECLARE_METATYPE(TrackSample)
Q_DECLARE_METATYPE(MissionState)

class MissionController : public QObject
{
    Q_OBJECT

  public:
    explicit MissionController(QObject* parent = nullptr);

    void start();

    const MissionState& state() const;
    QVector<TrackSample> history() const;
    const Tle& tle() const;
    bool tracking() const;

  signals:
    void stateChanged(const MissionState& state);
    void historyChanged();
    void statusMessage(const QString& message, bool isError);

  public slots:
    void refreshOrbitData();
    void setFollowSystemClock();

  private slots:
    void onTleUpdated(const QString& source);
    void onFetchFailed(const QString& message);
    void onTick();
    void onSampleTick();

  private:
    bool computeStateAt(
      const QDateTime& whenUtc,
      double& latitudeDeg,
      double& longitudeDeg,
      double& altitudeKm,
      double& velocityKmS,
      bool& sunlit,
      Vec3& positionTeme,
      Vec3& velocityTeme
    ) const;

    void rebuildGroundTrack(
      MissionState& state,
      const QDateTime& nowUtc
    ) const;

    void rebuildHistory();

    TleSource* source_ = nullptr;
    Sgp4 propagator_;
    Tle tle_;
    QTimer* tickTimer_ = nullptr;
    QTimer* sampleTimer_ = nullptr;
    MissionState state_;
    QVector<TrackSample> history_;
    bool initialized_ = false;
    bool tracking_ = false;
};
