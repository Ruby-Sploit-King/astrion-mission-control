#pragma once

#include "missioncontroller.hpp"
#include "telemetry.hpp"

#include <QMainWindow>

class QLabel;
class QLineEdit;
class QPushButton;
class QTableWidget;
class PlotWidget;
class OrbitWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

  public:
    explicit MainWindow(QWidget* parent = nullptr);

  private:
    void buildUi();
    void applyTheme();

    void onStateChanged(const MissionState& state);
    void onHistoryChanged();

    void importCsv();
    void exportCsv();
    void resumeLive();

    void updateMetrics();
    void updateTable();
    void setArchiveMode(bool archive);
    void setStatus(const QString& message, bool isError = false);

    QWidget* createMetricCard(
        const QString& title,
        QLabel*& valueLabel,
        const QString& unit
    );

    MissionController* controller_ = nullptr;
    QVector<TelemetryRecord> records_;
    MissionState latestState_;
    bool archiveMode_ = false;

    QLabel* statusLabel_ = nullptr;
    QLabel* recordCountLabel_ = nullptr;
    QLabel* sourceLabel_ = nullptr;
    QLabel* satelliteLabel_ = nullptr;
    QLabel* modeBadge_ = nullptr;

    QLabel* latitudeValue_ = nullptr;
    QLabel* longitudeValue_ = nullptr;
    QLabel* altitudeValue_ = nullptr;
    QLabel* velocityValue_ = nullptr;
    QLabel* periodValue_ = nullptr;
    QLabel* illuminationValue_ = nullptr;

    QLineEdit* filterInput_ = nullptr;
    QTableWidget* trackTable_ = nullptr;
    PlotWidget* plotWidget_ = nullptr;
    OrbitWidget* orbitWidget_ = nullptr;

    QPushButton* refreshButton_ = nullptr;
    QPushButton* exportButton_ = nullptr;
    QPushButton* importButton_ = nullptr;
    QPushButton* liveButton_ = nullptr;
};
