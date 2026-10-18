#include "plotwidget.hpp"

#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <QPolygonF>

#include <algorithm>

namespace
{
  constexpr double eps = 1.0e-9;

  struct Series
  {
    QVector<double> values;
    double minimum = 0.0;
    double maximum = 0.0;
  };

  Series makeSeries(const QVector<TrackSample>& samples, bool velocity)
  {
    Series series;

    for (const TrackSample& sample : samples)
    {
      series.values.append(
        velocity ? sample.velocityKmS : sample.altitudeKm
      );
    }

    if (series.values.isEmpty())
    {
      return series;
    }

    series.minimum = series.values.first();
    series.maximum = series.values.first();

    for (double value : series.values)
    {
      series.minimum = std::min(series.minimum, value);
      series.maximum = std::max(series.maximum, value);
    }

    return series;
  }

  QPolygonF buildPolygon(
    const Series& series,
    const QRectF& chart,
    bool closeArea
  )
  {
    QPolygonF points;
    const double range = std::max(0.1, series.maximum - series.minimum);
    const int count = series.values.size();

    for (int i = 0; i < count; ++i)
    {
      const qreal x = chart.left()
          + chart.width() * i / std::max(1, count - 1);

      const double normalized =
          (series.values.at(i) - series.minimum) / range;

      const qreal y = chart.bottom()
          - normalized * chart.height();

      points.append(QPointF(x, y));
    }

    if (closeArea && !points.isEmpty())
    {
      points.append(QPointF(points.last().x(), chart.bottom()));
      points.append(QPointF(points.first().x(), chart.bottom()));
    }

    return points;
  }

  QPainterPath polygonToPath(const QPolygonF& points)
  {
    QPainterPath path;

    if (points.isEmpty())
    {
      return path;
    }

    path.moveTo(points.first());

    for (qsizetype i = 1; i < points.size(); ++i)
    {
      path.lineTo(points.at(i));
    }

    return path;
  }
}

PlotWidget::PlotWidget(QWidget* parent)
    : QWidget(parent)
{
  setMinimumHeight(220);
  setSizePolicy(
    QSizePolicy::Expanding,
    QSizePolicy::Expanding
  );
}

void PlotWidget::setSamples(const QVector<TrackSample>& samples)
{
  samples_ = samples;
  update();
}

void PlotWidget::setLive(bool live)
{
  live_ = live;
  update();
}

void PlotWidget::paintEvent(QPaintEvent* event)
{
  Q_UNUSED(event);

  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);
  painter.fillRect(rect(), QColor("#0B1422"));

  const QRectF chart(
    64.0,
    30.0,
    width() - 88.0,
    height() - 66.0
  );

  if (chart.width() <= 0.0 || chart.height() <= 0.0)
  {
    return;
  }

  painter.setPen(QPen(QColor("#223348"), 1.0));

  for (int i = 0; i <= 4; ++i)
  {
    const qreal y = chart.top() + chart.height() * i / 4.0;

    painter.drawLine(
      QPointF(chart.left(), y),
      QPointF(chart.right(), y)
    );
  }

  painter.setPen(QColor("#91A5BD"));
  painter.drawText(
    QRectF(0.0, 4.0, width() - 10.0, 18.0),
    Qt::AlignRight | Qt::AlignVCenter,
    live_ ? "LIVE PROPAGATION" : "ARCHIVE REVIEW"
  );

  if (samples_.size() < 2)
  {
    painter.setPen(QColor("#91A5BD"));
    painter.drawText(
      chart,
      Qt::AlignCenter,
      "AWAITING TELEMETRY"
    );

    return;
  }

  const Series altitude = makeSeries(samples_, false);
  const Series velocity = makeSeries(samples_, true);

  const QPolygonF altitudeArea =
      buildPolygon(altitude, chart, true);

  painter.setPen(Qt::NoPen);
  painter.setBrush(QColor(39, 215, 232, 26));
  painter.drawPolygon(altitudeArea);

  painter.setBrush(Qt::NoBrush);
  painter.setPen(QPen(QColor("#27D7E8"), 2.4));
  painter.drawPath(polygonToPath(buildPolygon(altitude, chart, false)));

  painter.setPen(QPen(QColor("#F2C66D"), 1.8, Qt::DashLine));
  painter.drawPath(polygonToPath(buildPolygon(velocity, chart, false)));

  const TrackSample& latest = samples_.last();

  painter.setPen(QColor("#27D7E8"));
  painter.drawText(
    QRectF(chart.left(), chart.top() - 22.0, chart.width() / 2.0, 20.0),
    Qt::AlignLeft | Qt::AlignVCenter,
    QString("ALTITUDE  %1 KM")
        .arg(latest.altitudeKm, 0, 'f', 2)
  );

  painter.setPen(QColor("#F2C66D"));
  painter.drawText(
    QRectF(
      chart.left() + chart.width() / 2.0,
      chart.top() - 22.0,
      chart.width() / 2.0,
      20.0
    ),
    Qt::AlignRight | Qt::AlignVCenter,
    QString("VELOCITY  %1 KM/S")
        .arg(latest.velocityKmS, 0, 'f', 3)
  );

  painter.setPen(QColor("#91A5BD"));
  painter.drawText(
    QRectF(chart.left(), chart.bottom() + 10.0, chart.width(), 20.0),
    Qt::AlignLeft | Qt::AlignVCenter,
    samples_.first().timeUtc.toUTC().toString("HH:mm:ss")
        + " UTC"
  );

  painter.drawText(
    QRectF(chart.left(), chart.bottom() + 10.0, chart.width(), 20.0),
    Qt::AlignRight | Qt::AlignVCenter,
    latest.timeUtc.toUTC().toString("HH:mm:ss")
        + " UTC"
  );

  painter.setPen(QColor("#72869E"));
  painter.drawText(
    QRectF(2.0, chart.top(), chart.left() - 8.0, 20.0),
    Qt::AlignRight | Qt::AlignVCenter,
    QString::number(altitude.maximum, 'f', 1)
  );

  painter.drawText(
    QRectF(2.0, chart.bottom() - 20.0, chart.left() - 8.0, 20.0),
    Qt::AlignRight | Qt::AlignVCenter,
    QString::number(altitude.minimum, 'f', 1)
  );

  Q_UNUSED(eps);
}
