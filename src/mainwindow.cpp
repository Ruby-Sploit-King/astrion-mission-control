#include "mainwindow.hpp"

#include "orbitwidget.hpp"
#include "plotwidget.hpp"

#include <QAbstractItemView>
#include <QDateTime>
#include <QFileDialog>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QStatusBar>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <QSplitter>
#include <QWidget>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    resize(1440, 920);
    setMinimumSize(1050, 700);
    setWindowTitle("ASTRION // Mission Control");

    controller_ = new MissionController(this);

    buildUi();
    applyTheme();

    connect(
        controller_,
        &MissionController::stateChanged,
        this,
        &MainWindow::onStateChanged
    );

    connect(
        controller_,
        &MissionController::historyChanged,
        this,
        &MainWindow::onHistoryChanged
    );

    connect(
        controller_,
        &MissionController::statusMessage,
        this,
        &MainWindow::setStatus
    );

    controller_->start();

    setStatus("ACQUIRING ORBITAL ELEMENTS // CELESTRAK");
}

QWidget* MainWindow::createMetricCard(
    const QString& title,
    QLabel*& valueLabel,
    const QString& unit
)
{
    auto* card = new QFrame;
    card->setObjectName("metricCard");

    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(18, 16, 18, 16);
    layout->setSpacing(8);

    auto* titleLabel = new QLabel(title);
    titleLabel->setObjectName("metricTitle");

    valueLabel = new QLabel("--");
    valueLabel->setObjectName("metricValue");

    auto* unitLabel = new QLabel(unit);
    unitLabel->setObjectName("metricUnit");

    layout->addWidget(titleLabel);
    layout->addWidget(valueLabel);
    layout->addWidget(unitLabel);
    layout->addStretch();

    return card;
}

void MainWindow::buildUi()
{
    auto* central = new QWidget;
    auto* root = new QVBoxLayout(central);
    root->setContentsMargins(24, 20, 24, 18);
    root->setSpacing(16);

    auto* header = new QHBoxLayout;

    auto* brandColumn = new QVBoxLayout;
    auto* brand = new QLabel("ASTRION");
    brand->setObjectName("brandTitle");

    auto* subtitle = new QLabel(
        "MISSION CONTROL  /  LIVE GROUND TRACKING"
    );
    subtitle->setObjectName("eyebrow");

    brandColumn->addWidget(brand);
    brandColumn->addWidget(subtitle);

    auto* headerRight = new QVBoxLayout;
    headerRight->setAlignment(Qt::AlignRight);

    satelliteLabel_ = new QLabel("OBJECT 25544 // AWAITING TLE");
    satelliteLabel_->setObjectName("missionLabel");

    modeBadge_ = new QLabel("ACQUIRING");
    modeBadge_->setObjectName("simulationBadge");

    sourceLabel_ = new QLabel("SOURCE: -- // TLE AGE --");
    sourceLabel_->setObjectName("eyebrow");

    headerRight->addWidget(satelliteLabel_, 0, Qt::AlignRight);
    headerRight->addWidget(modeBadge_, 0, Qt::AlignRight);
    headerRight->addWidget(sourceLabel_, 0, Qt::AlignRight);

    header->addLayout(brandColumn);
    header->addStretch();
    header->addLayout(headerRight);

    root->addLayout(header);

    auto* separator = new QFrame;
    separator->setFrameShape(QFrame::HLine);
    separator->setObjectName("separator");
    root->addWidget(separator);

    auto* metrics = new QGridLayout;
    metrics->setSpacing(12);

    metrics->addWidget(
        createMetricCard("LATITUDE", latitudeValue_, "DEGREES"),
        0,
        0
    );

    metrics->addWidget(
        createMetricCard("LONGITUDE", longitudeValue_, "DEGREES"),
        0,
        1
    );

    metrics->addWidget(
        createMetricCard("ALTITUDE", altitudeValue_, "KM ABOVE WGS72"),
        0,
        2
    );

    metrics->addWidget(
        createMetricCard("VELOCITY", velocityValue_, "KM / SECOND"),
        0,
        3
    );

    metrics->addWidget(
        createMetricCard("ORBITAL PERIOD", periodValue_, "MINUTES"),
        0,
        4
    );

    metrics->addWidget(
        createMetricCard(
            "ILLUMINATION",
            illuminationValue_,
            "SOLAR EXPOSURE"
        ),
        0,
        5
    );

    root->addLayout(metrics);

    auto* panels = new QSplitter(Qt::Horizontal);
    panels->setChildrenCollapsible(false);

    auto* orbitPanel = new QFrame;
    orbitPanel->setObjectName("panel");

    auto* orbitLayout = new QVBoxLayout(orbitPanel);
    orbitLayout->setContentsMargins(16, 14, 16, 16);

    auto* orbitHeading = new QLabel("GROUND TRACK");
    orbitHeading->setObjectName("panelTitle");

    auto* orbitDescription = new QLabel(
        "EQUIRECTANGULAR PROJECTION / ORBIT PROPAGATION"
    );
    orbitDescription->setObjectName("eyebrow");

    orbitWidget_ = new OrbitWidget;

    orbitLayout->addWidget(orbitHeading);
    orbitLayout->addWidget(orbitDescription);
    orbitLayout->addSpacing(8);
    orbitLayout->addWidget(orbitWidget_, 1);

    auto* plotPanel = new QFrame;
    plotPanel->setObjectName("panel");

    auto* plotLayout = new QVBoxLayout(plotPanel);
    plotLayout->setContentsMargins(16, 14, 16, 16);

    auto* plotHeading = new QLabel("ORBITAL TELEMETRY");
    plotHeading->setObjectName("panelTitle");

    auto* plotDescription = new QLabel(
        "ALTITUDE + VELOCITY / SAMPLED PROPAGATION"
    );
    plotDescription->setObjectName("eyebrow");

    plotWidget_ = new PlotWidget;

    plotLayout->addWidget(plotHeading);
    plotLayout->addWidget(plotDescription);
    plotLayout->addSpacing(8);
    plotLayout->addWidget(plotWidget_, 1);

    panels->addWidget(orbitPanel);
    panels->addWidget(plotPanel);
    panels->setStretchFactor(0, 1);
    panels->setStretchFactor(1, 1);

    root->addWidget(panels, 3);

    auto* tablePanel = new QFrame;
    tablePanel->setObjectName("panel");

    auto* tableLayout = new QVBoxLayout(tablePanel);
    tableLayout->setContentsMargins(16, 14, 16, 14);
    tableLayout->setSpacing(12);

    auto* tableHeader = new QHBoxLayout;

    auto* tableHeading = new QLabel("TRACK ARCHIVE");
    tableHeading->setObjectName("panelTitle");

    recordCountLabel_ = new QLabel("0 RECORDS");
    recordCountLabel_->setObjectName("eyebrow");

    filterInput_ = new QLineEdit;
    filterInput_->setPlaceholderText("Filter visible records...");
    filterInput_->setClearButtonEnabled(true);
    filterInput_->setMaximumWidth(220);

    refreshButton_ = new QPushButton("REFRESH TLE");
    refreshButton_->setObjectName("primaryButton");

    exportButton_ = new QPushButton("EXPORT CSV");
    exportButton_->setObjectName("secondaryButton");

    importButton_ = new QPushButton("IMPORT CSV");
    importButton_->setObjectName("secondaryButton");

    liveButton_ = new QPushButton("RESUME LIVE");
    liveButton_->setObjectName("secondaryButton");
    liveButton_->setEnabled(false);

    tableHeader->addWidget(tableHeading);
    tableHeader->addSpacing(10);
    tableHeader->addWidget(recordCountLabel_);
    tableHeader->addStretch();
    tableHeader->addWidget(filterInput_);
    tableHeader->addWidget(refreshButton_);
    tableHeader->addWidget(exportButton_);
    tableHeader->addWidget(importButton_);
    tableHeader->addWidget(liveButton_);

    tableLayout->addLayout(tableHeader);

    trackTable_ = new QTableWidget;
    trackTable_->setColumnCount(6);
    trackTable_->setHorizontalHeaderLabels({
        "TIMESTAMP UTC",
        "LATITUDE DEG",
        "LONGITUDE DEG",
        "ALTITUDE KM",
        "VELOCITY KM/S",
        "ILLUMINATION"
    });

    trackTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    trackTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    trackTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    trackTable_->setAlternatingRowColors(true);
    trackTable_->verticalHeader()->setVisible(false);
    trackTable_->horizontalHeader()->setStretchLastSection(true);
    trackTable_->horizontalHeader()->setSectionResizeMode(
        QHeaderView::Stretch
    );

    tableLayout->addWidget(trackTable_, 1);
    root->addWidget(tablePanel, 2);

    statusLabel_ = new QLabel("SYSTEM INITIALIZING");
    statusLabel_->setObjectName("statusText");
    root->addWidget(statusLabel_);

    setCentralWidget(central);

    connect(
        refreshButton_,
        &QPushButton::clicked,
        controller_,
        &MissionController::refreshOrbitData
    );

    connect(
        exportButton_,
        &QPushButton::clicked,
        this,
        &MainWindow::exportCsv
    );

    connect(
        importButton_,
        &QPushButton::clicked,
        this,
        &MainWindow::importCsv
    );

    connect(
        liveButton_,
        &QPushButton::clicked,
        this,
        &MainWindow::resumeLive
    );

    connect(
        filterInput_,
        &QLineEdit::textChanged,
        this,
        &MainWindow::updateTable
    );

    statusBar()->hide();
}

void MainWindow::applyTheme()
{
    setStyleSheet(R"(
        QMainWindow, QWidget {
            background-color: #07101C;
            color: #DCE8F4;
            font-family: "Inter", "Noto Sans", sans-serif;
            font-size: 12px;
        }

        QLabel#brandTitle {
            color: #F2C66D;
            font-size: 28px;
            font-weight: 800;
            letter-spacing: 3px;
        }

        QLabel#eyebrow {
            color: #7F96AF;
            font-size: 10px;
            font-weight: 600;
            letter-spacing: 1px;
        }

        QLabel#missionLabel {
            color: #C9D7E8;
            font-size: 11px;
            font-weight: 700;
        }

        QLabel#simulationBadge {
            color: #F2C66D;
            background-color: #292317;
            border: 1px solid #725C31;
            border-radius: 4px;
            padding: 5px 9px;
            font-size: 10px;
            font-weight: 700;
        }

        QFrame#separator {
            color: #1C3045;
            max-height: 1px;
        }

        QFrame#metricCard, QFrame#panel {
            background-color: #0B1422;
            border: 1px solid #20334A;
            border-radius: 8px;
        }

        QLabel#metricTitle {
            color: #91A5BD;
            font-size: 10px;
            font-weight: 700;
            letter-spacing: 1px;
        }

        QLabel#metricValue {
            color: #27D7E8;
            font-size: 26px;
            font-weight: 800;
        }

        QLabel#metricUnit {
            color: #72869E;
            font-size: 9px;
            letter-spacing: 1px;
        }

        QLabel#panelTitle {
            color: #E6EEF7;
            font-size: 13px;
            font-weight: 800;
            letter-spacing: 1px;
        }

        QLabel#statusText {
            color: #27D7E8;
            font-size: 10px;
            font-weight: 600;
        }

        QPushButton {
            min-height: 28px;
            padding: 3px 12px;
            border-radius: 4px;
            font-weight: 700;
            font-size: 10px;
        }

        QPushButton#primaryButton {
            color: #06121D;
            background-color: #27D7E8;
            border: 1px solid #27D7E8;
        }

        QPushButton#primaryButton:hover {
            background-color: #75EAF3;
        }

        QPushButton#secondaryButton {
            color: #F2C66D;
            background-color: #171D28;
            border: 1px solid #655431;
        }

        QPushButton#secondaryButton:hover {
            background-color: #292317;
        }

        QPushButton:disabled {
            color: #55657A;
            border-color: #26374A;
        }

        QLineEdit {
            color: #E6EEF7;
            background-color: #080F19;
            border: 1px solid #29415B;
            border-radius: 4px;
            padding: 7px;
            selection-background-color: #146B7A;
        }

        QTableWidget {
            color: #C9D7E8;
            background-color: #080F19;
            alternate-background-color: #0D1928;
            gridline-color: #20334A;
            border: 1px solid #20334A;
            selection-background-color: #143D50;
            selection-color: #FFFFFF;
        }

        QHeaderView::section {
            color: #91A5BD;
            background-color: #111E2E;
            border: none;
            border-bottom: 1px solid #29415B;
            padding: 9px 6px;
            font-size: 10px;
            font-weight: 700;
        }

        QSplitter::handle {
            background-color: #07101C;
            width: 8px;
        }
    )");
}

void MainWindow::onStateChanged(const MissionState& state)
{
    latestState_ = state;

    if (state.valid)
    {
        satelliteLabel_->setText(
            QString("OBJECT %1 // %2")
                .arg(state.satnum)
                .arg(state.satelliteName.toUpper())
        );

        sourceLabel_->setText(
            QString("SOURCE: %1 // TLE AGE %2 H")
                .arg(state.dataSource.toUpper())
                .arg(state.tleAgeHours, 0, 'f', 2)
        );
    }

    orbitWidget_->setMissionState(state);

    if (!archiveMode_)
    {
        updateMetrics();
    }
}

void MainWindow::onHistoryChanged()
{
    if (archiveMode_)
    {
        return;
    }

    records_.clear();

    for (const TrackSample& sample : controller_->history())
    {
        TelemetryRecord record;
        record.timestamp = sample.timeUtc;
        record.latitudeDeg = sample.latitudeDeg;
        record.longitudeDeg = sample.longitudeDeg;
        record.altitudeKm = sample.altitudeKm;
        record.velocityKmS = sample.velocityKmS;
        record.sunlit = sample.sunlit;
        records_.append(record);
    }

    plotWidget_->setSamples(controller_->history());
    updateTable();
}

void MainWindow::importCsv()
{
    const QString filePath = QFileDialog::getOpenFileName(
        this,
        "Import Track CSV",
        QString(),
        "CSV Files (*.csv);;All Files (*)"
    );

    if (filePath.isEmpty())
    {
        return;
    }

    QString error;
    const QVector<TelemetryRecord> imported =
        Telemetry::loadCsv(filePath, &error);

    if (!error.isEmpty())
    {
        QMessageBox::warning(
            this,
            "Track Import Failed",
            error
        );

        setStatus("IMPORT FAILED // " + error, true);
        return;
    }

    records_ = imported;
    setArchiveMode(true);
    updateMetrics();
    updateTable();

    setStatus(
        QString("ARCHIVE LOADED // %1 RECORDS")
            .arg(records_.size())
    );
}

void MainWindow::exportCsv()
{
    if (records_.isEmpty())
    {
        setStatus("EXPORT ABORTED // NO RECORDS IN MEMORY", true);
        return;
    }

    const QString filePath = QFileDialog::getSaveFileName(
        this,
        "Export Track CSV",
        "astrion-track.csv",
        "CSV Files (*.csv)"
    );

    if (filePath.isEmpty())
    {
        return;
    }

    QString error;

    if (!Telemetry::saveCsv(filePath, records_, &error))
    {
        QMessageBox::warning(this, "Track Export Failed", error);
        setStatus("EXPORT FAILED // " + error, true);
        return;
    }

    setStatus(
        QString("EXPORT COMPLETE // %1 RECORDS WRITTEN")
            .arg(records_.size())
    );
}

void MainWindow::resumeLive()
{
    setArchiveMode(false);
    onHistoryChanged();
    updateMetrics();

    setStatus("LIVE TRACKING RESUMED");
}

void MainWindow::setArchiveMode(bool archive)
{
    archiveMode_ = archive;

    plotWidget_->setLive(!archive);
    orbitWidget_->setLive(!archive);
    liveButton_->setEnabled(archive);

    modeBadge_->setText(
        archive ? "ARCHIVE REVIEW" : "LIVE TRACKING"
    );
}

void MainWindow::updateMetrics()
{
    if (archiveMode_)
    {
        if (records_.isEmpty())
        {
            return;
        }

        const TelemetryRecord& latest = records_.last();

        latitudeValue_->setText(
            QString::number(latest.latitudeDeg, 'f', 3)
        );
        longitudeValue_->setText(
            QString::number(latest.longitudeDeg, 'f', 3)
        );
        altitudeValue_->setText(
            QString::number(latest.altitudeKm, 'f', 2)
        );
        velocityValue_->setText(
            QString::number(latest.velocityKmS, 'f', 3)
        );
        periodValue_->setText("--");
        illuminationValue_->setText(
            latest.sunlit ? "SUNLIT" : "ECLIPSE"
        );

        return;
    }

    if (!latestState_.valid)
    {
        return;
    }

    latitudeValue_->setText(
        QString::number(latestState_.latitudeDeg, 'f', 3)
    );
    longitudeValue_->setText(
        QString::number(latestState_.longitudeDeg, 'f', 3)
    );
    altitudeValue_->setText(
        QString::number(latestState_.altitudeKm, 'f', 2)
    );
    velocityValue_->setText(
        QString::number(latestState_.velocityKmS, 'f', 3)
    );
    periodValue_->setText(
        QString::number(latestState_.periodMinutes, 'f', 2)
    );
    illuminationValue_->setText(
        latestState_.sunlit ? "SUNLIT" : "ECLIPSE"
    );
}

void MainWindow::updateTable()
{
    trackTable_->setRowCount(0);

    const QString filter = filterInput_->text().trimmed();

    for (const TelemetryRecord& record : records_)
    {
        const QStringList fields = {
            record.timestamp.toUTC().toString("yyyy-MM-dd HH:mm:ss"),
            QString::number(record.latitudeDeg, 'f', 3),
            QString::number(record.longitudeDeg, 'f', 3),
            QString::number(record.altitudeKm, 'f', 2),
            QString::number(record.velocityKmS, 'f', 3),
            record.sunlit ? "SUNLIT" : "ECLIPSE"
        };

        if (!filter.isEmpty())
        {
            bool matches = false;

            for (const QString& field : fields)
            {
                if (field.contains(filter, Qt::CaseInsensitive))
                {
                    matches = true;
                    break;
                }
            }

            if (!matches)
            {
                continue;
            }
        }

        const int row = trackTable_->rowCount();
        trackTable_->insertRow(row);

        for (int column = 0; column < fields.size(); ++column)
        {
            auto* item = new QTableWidgetItem(fields.at(column));

            if (column > 0)
            {
                item->setTextAlignment(
                    Qt::AlignRight | Qt::AlignVCenter
                );
            }

            trackTable_->setItem(row, column, item);
        }
    }

    recordCountLabel_->setText(
        QString("%1 / %2 RECORDS")
            .arg(trackTable_->rowCount())
            .arg(records_.size())
    );
}

void MainWindow::setStatus(
    const QString& message,
    bool isError
)
{
    statusLabel_->setText("●  " + message);

    statusLabel_->setStyleSheet(
        isError
            ? "color: #FF7B82;"
            : "color: #27D7E8;"
    );
}
