#include "tlesource.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStandardPaths>

namespace
{
  const QString kIssTleUrl =
      QStringLiteral(
        "https://celestrak.org/NORAD/elements/gp.php"
        "?CATNR=25544&FORMAT=tle"
      );

  const QString kUserAgent =
      QStringLiteral("ASTRION-Mission-Control/1.0 (Qt6)");

  constexpr qint64 networkTimeoutMs = 15000;
}

TleSource::TleSource(QObject* parent)
    : QObject(parent)
{
  network_ = new QNetworkAccessManager(this);
}

void TleSource::start()
{
  if (loadFromCache())
  {
    emit tleUpdated(QStringLiteral("cache"));
  }

  refresh();
}

void TleSource::refresh()
{
  if (pendingReply_)
  {
    return;
  }

  QNetworkRequest request{QUrl(kIssTleUrl)};
  request.setHeader(
    QNetworkRequest::UserAgentHeader,
    kUserAgent
  );
  request.setTransferTimeout(networkTimeoutMs);

  pendingReply_ = network_->get(request);

  connect(
    pendingReply_,
    &QNetworkReply::finished,
    this,
    &TleSource::onReplyFinished
  );
}

void TleSource::onReplyFinished()
{
  if (!pendingReply_)
  {
    return;
  }

  QNetworkReply* reply = pendingReply_;
  pendingReply_ = nullptr;
  reply->deleteLater();

  if (reply->error() != QNetworkReply::NoError)
  {
    emit fetchFailed(
      QStringLiteral("Orbital data request failed: %1")
        .arg(reply->errorString())
    );
    return;
  }

  const QString text = QString::fromUtf8(reply->readAll());

  if (!applyText(text))
  {
    emit fetchFailed(
      QStringLiteral("Orbital data from CelesTrak was unreadable.")
    );
    return;
  }

  storeToCache(text);

  lastUpdatedUtc_ = QDateTime::currentDateTimeUtc();
  hasData_ = true;

  emit tleUpdated(QStringLiteral("celestrak"));
}

QString TleSource::cachePath() const
{
  const QString root = QStandardPaths::writableLocation(
    QStandardPaths::GenericDataLocation
  );

  return QDir(root).filePath(
    QStringLiteral("astrion/tle-25544.txt")
  );
}

bool TleSource::loadFromCache()
{
  QFile file(cachePath());

  if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
  {
    return false;
  }

  const QString text = QString::fromUtf8(file.readAll());
  file.close();

  if (!applyText(text))
  {
    return false;
  }

  const QDateTime modified = QFileInfo(cachePath()).lastModified().toUTC();
  lastUpdatedUtc_ = modified.isValid()
      ? modified
      : QDateTime::currentDateTimeUtc();
  hasData_ = true;

  return true;
}

bool TleSource::storeToCache(const QString& text)
{
  const QString path = cachePath();

  QDir().mkpath(QFileInfo(path).absolutePath());

  QFile file(path);

  if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
  {
    return false;
  }

  file.write(text.toUtf8());
  return true;
}

bool TleSource::applyText(const QString& text)
{
  QString error;
  const std::optional<Tle> parsed = Tle::fromText(text, &error);

  if (!parsed || !parsed->epochUtc.isValid())
  {
    return false;
  }

  tle_ = *parsed;
  return true;
}

bool TleSource::hasData() const
{
  return hasData_;
}

const Tle& TleSource::tle() const
{
  return tle_;
}

QDateTime TleSource::lastUpdatedUtc() const
{
  return lastUpdatedUtc_;
}

int TleSource::ageSeconds() const
{
  if (!lastUpdatedUtc_.isValid())
  {
    return -1;
  }

  return static_cast<int>(
    lastUpdatedUtc_.secsTo(QDateTime::currentDateTimeUtc())
  );
}
