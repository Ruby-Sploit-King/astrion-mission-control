#pragma once

#include "tle.hpp"

#include <QDateTime>
#include <QObject>
#include <QString>

class QNetworkAccessManager;
class QNetworkReply;

class TleSource : public QObject
{
    Q_OBJECT

  public:
    explicit TleSource(QObject* parent = nullptr);

    void start();

    bool hasData() const;
    const Tle& tle() const;
    QDateTime lastUpdatedUtc() const;
    int ageSeconds() const;

    QString cachePath() const;

  signals:
    void tleUpdated(const QString& source);
    void fetchFailed(const QString& message);

  public slots:
    void refresh();

  private slots:
    void onReplyFinished();

  private:
    bool loadFromCache();
    bool storeToCache(const QString& text);
    bool applyText(const QString& text);

    QNetworkAccessManager* network_ = nullptr;
    QNetworkReply* pendingReply_ = nullptr;
    QDateTime lastUpdatedUtc_;
    Tle tle_;
    bool hasData_ = false;
};
