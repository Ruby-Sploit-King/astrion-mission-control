#pragma once

#include "missioncontroller.hpp"

#include <QWidget>

class PlotWidget : public QWidget
{
    Q_OBJECT

  public:
    explicit PlotWidget(QWidget* parent = nullptr);

    void setSamples(const QVector<TrackSample>& samples);
    void setLive(bool live);

  protected:
    void paintEvent(QPaintEvent* event) override;

  private:
    QVector<TrackSample> samples_;
    bool live_ = true;
};
