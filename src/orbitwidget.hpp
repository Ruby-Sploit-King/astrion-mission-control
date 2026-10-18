#pragma once

#include "missioncontroller.hpp"

#include <QWidget>

class OrbitWidget : public QWidget
{
    Q_OBJECT

  public:
    explicit OrbitWidget(QWidget* parent = nullptr);

    void setMissionState(const MissionState& state);
    void setLive(bool live);

  protected:
    void paintEvent(QPaintEvent* event) override;

  private:
    MissionState state_;
    bool live_ = true;
};
