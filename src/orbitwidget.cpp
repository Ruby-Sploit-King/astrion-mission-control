#include "orbitwidget.hpp"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <QPolygonF>

#include <cmath>

namespace
{
  constexpr double pi = 3.14159265358979323846;
  constexpr double deg2rad = pi / 180.0;
  constexpr double rad2deg = 180.0 / pi;

  using Ring = QVector<QPointF>;

  const QVector<Ring>& landRings()
  {
    static const QVector<Ring> rings = []
    {
      QVector<Ring> result;

      QFile file(QStringLiteral(":/data/ne_110m_land.geojson"));

      if (!file.open(QIODevice::ReadOnly))
      {
        return result;
      }

      const QJsonDocument document =
          QJsonDocument::fromJson(file.readAll());

      const QJsonArray features = document
          .object()
          .value(QStringLiteral("features"))
          .toArray();

      for (const QJsonValue& feature : features)
      {
        const QJsonObject geometry = feature
            .toObject()
            .value(QStringLiteral("geometry"))
            .toObject();

        const QString type = geometry
            .value(QStringLiteral("type"))
            .toString();

        const QJsonArray coordinates = geometry
            .value(QStringLiteral("coordinates"))
            .toArray();

        if (type == QLatin1String("Polygon"))
        {
          for (const QJsonValue& ring : coordinates)
          {
            Ring points;

            for (const QJsonValue& node : ring.toArray())
            {
              const QJsonArray pair = node.toArray();

              if (pair.size() >= 2)
              {
                points.append(
                  QPointF(pair.at(0).toDouble(), pair.at(1).toDouble())
                );
              }
            }

            if (!points.isEmpty())
            {
              result.append(points);
            }
          }
        }
        else if (type == QLatin1String("MultiPolygon"))
        {
          for (const QJsonValue& polygon : coordinates)
          {
            for (const QJsonValue& ring : polygon.toArray())
            {
              Ring points;

              for (const QJsonValue& node : ring.toArray())
              {
                const QJsonArray pair = node.toArray();

                if (pair.size() >= 2)
                {
                  points.append(
                    QPointF(
                      pair.at(0).toDouble(),
                      pair.at(1).toDouble()
                    )
                  );
                }
              }

              if (!points.isEmpty())
              {
                result.append(points);
              }
            }
          }
        }
      }

      return result;
    }();

    return rings;
  }

  double mapX(double longitudeDeg, const QRectF& map)
  {
    return map.left()
        + (longitudeDeg + 180.0) / 360.0 * map.width();
  }

  double mapY(double latitudeDeg, const QRectF& map)
  {
    return map.top()
        + (90.0 - latitudeDeg) / 180.0 * map.height();
  }

  QPolygonF toScreen(const Ring& ring, const QRectF& map, double shift)
  {
    QPolygonF polygon;

    for (const QPointF& point : ring)
    {
      polygon.append(
        QPointF(
          mapX(point.x() + shift, map),
          mapY(point.y(), map)
        )
      );
    }

    return polygon;
  }

  void drawTrack(
    QPainter& painter,
    const QVector<QPointF>& points,
    const QRectF& map,
    const QPen& pen,
    double shift
  )
  {
    if (points.size() < 2)
    {
      return;
    }

    painter.setPen(pen);

    QPainterPath path;
    bool started = false;
    double previous = 0.0;

    for (const QPointF& point : points)
    {
      const double x = point.x() + shift;

      if (!started || std::fabs(x - previous) > 180.0)
      {
        path.moveTo(mapX(x, map), mapY(point.y(), map));
        started = true;
      }
      else
      {
        path.lineTo(mapX(x, map), mapY(point.y(), map));
      }

      previous = x;
    }

    painter.drawPath(path);
  }

  double deltaFromSun(double latitudeDeg, double sunSubLatitudeDeg)
  {
    const double x = -std::tan(latitudeDeg * deg2rad)
        * std::tan(sunSubLatitudeDeg * deg2rad);

    const double clamped = std::max(-1.0, std::min(1.0, x));

    return std::acos(clamped) * rad2deg;
  }
}

OrbitWidget::OrbitWidget(QWidget* parent)
    : QWidget(parent)
{
  setMinimumHeight(280);
  setSizePolicy(
    QSizePolicy::Expanding,
    QSizePolicy::Expanding
  );
}

void OrbitWidget::setMissionState(const MissionState& state)
{
  state_ = state;
  update();
}

void OrbitWidget::setLive(bool live)
{
  live_ = live;
  update();
}

void OrbitWidget::paintEvent(QPaintEvent* event)
{
  Q_UNUSED(event);

  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);
  painter.fillRect(rect(), QColor("#0B1422"));

  const QRectF map(
    10.0,
    30.0,
    width() - 20.0,
    height() - 46.0
  );

  if (map.width() <= 0.0 || map.height() <= 0.0)
  {
    return;
  }

  painter.setPen(QColor("#91A5BD"));
  painter.drawText(
    QRectF(10.0, 6.0, width() - 20.0, 18.0),
    Qt::AlignLeft | Qt::AlignVCenter,
    live_
        ? "GROUND TRACK / LIVE ORBIT PROPAGATION"
        : "GROUND TRACK / ARCHIVE REVIEW"
  );

  painter.setPen(QPen(QColor("#1C3045"), 1.0));

  for (int longitude = -180; longitude <= 180; longitude += 30)
  {
    const double x = mapX(longitude, map);
    painter.drawLine(
      QPointF(x, map.top()),
      QPointF(x, map.bottom())
    );
  }

  for (int latitude = -90; latitude <= 90; latitude += 30)
  {
    const double y = mapY(latitude, map);
    painter.drawLine(
      QPointF(map.left(), y),
      QPointF(map.right(), y)
    );
  }

  painter.setPen(QPen(QColor("#2A4A66"), 1.2));

  if (map.contains(QPointF(mapX(0.0, map), mapY(0.0, map))))
  {
    painter.drawLine(
      QPointF(mapX(0.0, map), map.top()),
      QPointF(mapX(0.0, map), map.bottom())
    );
  }

  const QVector<Ring>& rings = landRings();

  painter.setPen(QPen(QColor("#2E526E"), 0.8));

  for (double shift : {-360.0, 0.0, 360.0})
  {
    painter.setBrush(QColor("#12293D"));

    for (const Ring& ring : rings)
    {
      painter.drawPolygon(toScreen(ring, map, shift));
    }
  }

  painter.setBrush(Qt::NoBrush);

  if (state_.valid)
  {
    QPolygonF night;
    const int steps = 60;

    for (int i = 0; i <= steps; ++i)
    {
      const double latitude = -90.0 + 180.0 * i / steps;
      const double delta = deltaFromSun(
        latitude,
        state_.sunSubLatitudeDeg
      );

      night.append(
        QPointF(
          state_.sunSubLongitudeDeg + delta,
          latitude
        )
      );
    }

    for (int i = steps; i >= 0; --i)
    {
      const double latitude = -90.0 + 180.0 * i / steps;
      const double delta = deltaFromSun(
        latitude,
        state_.sunSubLatitudeDeg
      );

      night.append(
        QPointF(
          state_.sunSubLongitudeDeg + 360.0 - delta,
          latitude
        )
      );
    }

    painter.setPen(Qt::NoPen);

    for (double shift : {-360.0, 0.0, 360.0})
    {
      painter.setBrush(QColor(3, 10, 20, 120));
      painter.drawPolygon(toScreen(night, map, shift));
    }

    painter.setBrush(Qt::NoBrush);
  }

  for (double shift : {-360.0, 0.0, 360.0})
  {
    drawTrack(
      painter,
      state_.trackPast,
      map,
      QPen(QColor("#27D7E8"), 2.0),
      shift
    );

    QColor futureColor("#27D7E8");
    futureColor.setAlpha(150);

    drawTrack(
      painter,
      state_.trackFuture,
      map,
      QPen(futureColor, 1.6, Qt::DashLine),
      shift
    );
  }

  if (state_.valid)
  {
    const QPointF marker(
      mapX(state_.longitudeDeg, map),
      mapY(state_.latitudeDeg, map)
    );

    painter.setPen(QPen(QColor("#F2C66D"), 1.0));
    painter.drawEllipse(marker, 8.0, 8.0);

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor("#F2C66D"));
    painter.drawEllipse(marker, 3.5, 3.5);

    painter.setPen(QColor("#F2C66D"));
    painter.drawText(
      QRectF(marker.x() + 12.0, marker.y() - 10.0, 160.0, 20.0),
      Qt::AlignLeft | Qt::AlignVCenter,
      state_.satelliteName
    );
  }
  else
  {
    painter.setPen(QColor("#91A5BD"));
    painter.drawText(
      map,
      Qt::AlignCenter,
      "AWAITING ORBITAL DATA"
    );
  }

  painter.setPen(QColor("#72869E"));

  for (int longitude = -120; longitude <= 120; longitude += 60)
  {
    painter.drawText(
      QRectF(mapX(longitude, map) - 40.0, map.bottom() + 2.0, 80.0, 16.0),
      Qt::AlignCenter,
      QString::number(longitude)
    );
  }

  for (int latitude = 60; latitude >= -60; latitude -= 60)
  {
    painter.drawText(
      QRectF(map.right() - 34.0, mapY(latitude, map) - 8.0, 32.0, 16.0),
      Qt::AlignRight | Qt::AlignVCenter,
      QString::number(latitude)
    );
  }
}
