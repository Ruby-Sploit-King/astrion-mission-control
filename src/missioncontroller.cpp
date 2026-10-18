#include "missioncontroller.hpp"

#include "astro.hpp"
#include "tlesource.hpp"

#include <QTimer>

namespace
{
  constexpr int tickIntervalMs = 1000;
  constexpr int sampleIntervalMs = 15000;
  constexpr int historyLimit = 480;
  constexpr int backfillMinutes = 30;
  constexpr int trackHalfSpanMinutes = 45;

  constexpr double rEarthKm = 6378.137;
}

MissionController::MissionController(QObject* parent)
    : QObject(parent)
{
  source_ = new TleSource(this);

  connect(
    source_,
    &TleSource::tleUpdated,
    this,
    &MissionController::onTleUpdated
  );

  connect(
    source_,
    &TleSource::fetchFailed,
    this,
    &MissionController::onFetchFailed
  );

  tickTimer_ = new QTimer(this);
  tickTimer_->setInterval(tickIntervalMs);

  connect(
    tickTimer_,
    &QTimer::timeout,
    this,
    &MissionController::onTick
  );

  sampleTimer_ = new QTimer(this);
  sampleTimer_->setInterval(sampleIntervalMs);

  connect(
    sampleTimer_,
    &QTimer::timeout,
    this,
    &MissionController::onSampleTick
  );
}

void MissionController::start()
{
  source_->start();
}

void MissionController::refreshOrbitData()
{
  emit statusMessage(
    QStringLiteral("REQUESTING FRESH ORBITAL DATA // CELESTRAK"),
    false
  );

  source_->refresh();
}

void MissionController::setFollowSystemClock()
{
  onTick();
}

bool MissionController::computeStateAt(
  const QDateTime& whenUtc,
  double& latitudeDeg,
  double& longitudeDeg,
  double& altitudeKm,
  double& velocityKmS,
  bool& sunlit,
  Vec3& positionTeme,
  Vec3& velocityTeme
) const
{
  const double tsince = tle_.minutesSinceEpoch(whenUtc);

  if (!propagator_.propagate(tsince, positionTeme, velocityTeme))
  {
    return false;
  }

  const double jd = Astro::julianDate(whenUtc);
  const double gmst = Astro::gstime(jd);
  const Vec3 sunDir = Astro::sunDirectionTeme(jd);

  sunlit = Astro::isSunlit(positionTeme, sunDir);

  const Vec3 ecef = Astro::temeToEcef(positionTeme, gmst);
  const Astro::Geodetic geodetic = Astro::ecefToGeodetic(ecef);

  latitudeDeg = geodetic.latitudeDeg;
  longitudeDeg = geodetic.longitudeDeg;
  altitudeKm = geodetic.altitudeKm;
  velocityKmS = velocityTeme.magnitude();

  return true;
}

void MissionController::rebuildGroundTrack(
  MissionState& state,
  const QDateTime& nowUtc
) const
{
  state.groundTrack.clear();
  state.trackPast.clear();
  state.trackFuture.clear();

  for (int offset = -trackHalfSpanMinutes;
       offset <= trackHalfSpanMinutes;
       ++offset)
  {
    const QDateTime when = nowUtc.addSecs(offset * 60);

    double latitudeDeg = 0.0;
    double longitudeDeg = 0.0;
    double altitudeKm = 0.0;
    double velocityKmS = 0.0;
    bool sunlit = true;
    Vec3 positionTeme;
    Vec3 velocityTeme;

    if (!computeStateAt(
        when,
        latitudeDeg,
        longitudeDeg,
        altitudeKm,
        velocityKmS,
        sunlit,
        positionTeme,
        velocityTeme))
    {
      continue;
    }

    const QPointF point(longitudeDeg, latitudeDeg);
    state.groundTrack.append(point);

    if (offset < 0)
    {
      state.trackPast.append(point);
    }
    else if (offset == 0)
    {
      state.trackFuture.append(point);
    }
    else
    {
      state.trackFuture.append(point);
    }
  }

  state.currentTrackIndex = trackHalfSpanMinutes;
}

void MissionController::onTleUpdated(const QString& sourceName)
{
  QString error;

  if (!propagator_.init(source_->tle(), &error))
  {
    tracking_ = false;
    emit statusMessage(
      QStringLiteral("ORBITAL DATA REJECTED // %1").arg(error),
      true
    );
    return;
  }

  tle_ = source_->tle();
  initialized_ = true;
  tracking_ = true;

  state_.periodMinutes = propagator_.periodMinutes();
  state_.satnum = tle_.satnum;
  state_.satelliteName = tle_.name.isEmpty()
      ? QStringLiteral("NORAD %1").arg(tle_.satnum)
      : tle_.name.trimmed();
  state_.dataSource = sourceName;

  rebuildHistory();

  emit statusMessage(
    sourceName == QStringLiteral("cache")
        ? QStringLiteral(
            "TRACKING %1 // CACHED ELEMENTS %2")
            .arg(state_.satelliteName)
            .arg(tle_.epochUtc.toString("yyyy-MM-dd HH:mm")
                 + "Z")
        : QStringLiteral(
            "TRACKING %1 // TLE UPDATED FROM %2")
            .arg(state_.satelliteName)
            .arg(sourceName.toUpper()),
    false
  );

  onTick();

  if (!tickTimer_->isActive())
  {
    tickTimer_->start();
  }

  if (!sampleTimer_->isActive())
  {
    sampleTimer_->start();
  }
}

void MissionController::onFetchFailed(const QString& message)
{
  if (tracking_)
  {
    emit statusMessage(
      QStringLiteral("OFFLINE // USING CACHED ELEMENTS"),
      false
    );
    return;
  }

  emit statusMessage(
    message.toUpper(),
    true
  );
}

void MissionController::onTick()
{
  if (!initialized_ || !tracking_)
  {
    return;
  }

  MissionState next;
  next.periodMinutes = propagator_.periodMinutes();
  next.satnum = tle_.satnum;
  next.satelliteName = state_.satelliteName;
  next.dataSource = state_.dataSource;

  const QDateTime nowUtc = QDateTime::currentDateTimeUtc();

  Vec3 positionTeme;
  Vec3 velocityTeme;

  if (!computeStateAt(
      nowUtc,
      next.latitudeDeg,
      next.longitudeDeg,
      next.altitudeKm,
      next.velocityKmS,
      next.sunlit,
      positionTeme,
      velocityTeme))
  {
    emit statusMessage(
      QStringLiteral("PROPAGATION ERROR // ORBITAL ELEMENTS MAY BE STALE"),
      true
    );
    tracking_ = false;
    return;
  }

  next.valid = true;
  next.timeUtc = nowUtc;
  next.tleAgeHours = source_->ageSeconds() / 3600.0;

  const double jd = Astro::julianDate(nowUtc);
  const double gmst = Astro::gstime(jd);
  const Vec3 sunDir = Astro::sunDirectionTeme(jd);
  const Vec3 sunEcef = Astro::temeToEcef(sunDir, gmst);
  const Astro::Geodetic sunGeodetic =
      Astro::ecefToGeodetic(sunEcef * rEarthKm);

  next.sunSubLatitudeDeg = sunGeodetic.latitudeDeg;
  next.sunSubLongitudeDeg = sunGeodetic.longitudeDeg;

  rebuildGroundTrack(next, nowUtc);

  state_ = next;
  emit stateChanged(state_);
}

void MissionController::onSampleTick()
{
  if (!state_.valid)
  {
    return;
  }

  TrackSample sample;
  sample.timeUtc = state_.timeUtc;
  sample.latitudeDeg = state_.latitudeDeg;
  sample.longitudeDeg = state_.longitudeDeg;
  sample.altitudeKm = state_.altitudeKm;
  sample.velocityKmS = state_.velocityKmS;
  sample.sunlit = state_.sunlit;

  history_.append(sample);

  while (history_.size() > historyLimit)
  {
    history_.removeFirst();
  }

  emit historyChanged();
}

void MissionController::rebuildHistory()
{
  history_.clear();

  const QDateTime nowUtc = QDateTime::currentDateTimeUtc();
  const int stepSeconds = sampleIntervalMs / 1000;
  const int backfillSamples = backfillMinutes * 60 / stepSeconds;

  for (int i = backfillSamples; i >= 0; --i)
  {
    const QDateTime when = nowUtc.addSecs(-i * stepSeconds);

    double latitudeDeg = 0.0;
    double longitudeDeg = 0.0;
    double altitudeKm = 0.0;
    double velocityKmS = 0.0;
    bool sunlit = true;
    Vec3 positionTeme;
    Vec3 velocityTeme;

    if (!computeStateAt(
        when,
        latitudeDeg,
        longitudeDeg,
        altitudeKm,
        velocityKmS,
        sunlit,
        positionTeme,
        velocityTeme))
    {
      continue;
    }

    TrackSample sample;
    sample.timeUtc = when;
    sample.latitudeDeg = latitudeDeg;
    sample.longitudeDeg = longitudeDeg;
    sample.altitudeKm = altitudeKm;
    sample.velocityKmS = velocityKmS;
    sample.sunlit = sunlit;

    history_.append(sample);
  }

  emit historyChanged();
}

const MissionState& MissionController::state() const
{
  return state_;
}

QVector<TrackSample> MissionController::history() const
{
  return history_;
}

const Tle& MissionController::tle() const
{
  return tle_;
}

bool MissionController::tracking() const
{
  return tracking_;
}
