#include <string>
#include <fstream>
#include <iostream>
#include <chrono>
#include <map>
#include <vector>
#include <functional>

#include <QMessageBox>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "../src-runner/parser.hpp"

#define set_param(PARAM)                                         \
    if (expe.parameters.count(PARAM)) {                          \
        memory->set_parameter(PARAM, expe.parameters.at(PARAM)); \
    }
#define S(x) #x
#define SX(x) S(x)
#define set_ui_param(PARAM, NAME)                                                  \
    ui->PARAM##_label->setText(                                              \
        NAME ": " + QString::number(memory->get_parameter(SX(PARAM)))); \
    ui->PARAM##_slider->setValue(memory->get_parameter(SX(PARAM)) * 100);

using namespace std;
using namespace std::chrono;

const vector<QColor> PALETTE = {QColor(242, 146, 70),  QColor(242, 214, 70),
                                QColor(101, 156, 27),  QColor(27, 156, 154),
                                QColor(37, 68, 171),   QColor(110, 37, 171),
                                QColor(171, 37, 124),  QColor(204, 58, 26),
                                QColor(191, 180, 163), QColor(80, 88, 89)};

const QColor fillColor = QColor(200,200,200,20);

const int HISTORY_SAMPLING_RATE = 500;  // Hz

vector<double> activations_timestamps;
vector<double> external_activations_timestamps;
map<size_t, vector<double>> activations_logs;
map<size_t, vector<double>> external_activations_logs;

microseconds _last_activations_log;
microseconds _last_external_activations_log;

void logging(map<size_t, vector<double>> &logs, vector<double> &timestamps,
             microseconds &last_log, microseconds time_from_start,
             const MemoryVector &levels) {
    // if necessary, store the activation level
    microseconds us_since_last_log = time_from_start - last_log;
    if (us_since_last_log.count() >
        (std::micro::den * 1. / HISTORY_SAMPLING_RATE)) {
        last_log = time_from_start;
        timestamps.push_back(
            duration_cast<milliseconds>(time_from_start).count());
        for (size_t i = 0; i < levels.size(); i++) {
            logs[i].push_back(levels[i]);
        }
    }
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
    QCoreApplication::setOrganizationName("Plymouth_University");
    QCoreApplication::setApplicationName("MemoryExplorer");

    ui->setupUi(this);

    const QFont fixedFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    ui->experiment_editor->document()->setDefaultFont(fixedFont);

    // enable syntax highlighting
    highlighter = make_unique<SimpleMarkdownHighlighter>(ui->experiment_editor->document());

    connect(ui->Dg_slider, SIGNAL(sliderReleased()), this, SLOT(autoupdateActivationsPlot()));
    connect(ui->Lg_slider, SIGNAL(sliderReleased()), this, SLOT(autoupdateActivationsPlot()));
    connect(ui->Eg_slider, SIGNAL(sliderReleased()), this, SLOT(autoupdateActivationsPlot()));
    connect(ui->Ig_slider, SIGNAL(sliderReleased()), this, SLOT(autoupdateActivationsPlot()));
    connect(ui->Amax_slider, SIGNAL(sliderReleased()), this, SLOT(autoupdateActivationsPlot()));
    connect(ui->Amin_slider, SIGNAL(sliderReleased()), this, SLOT(autoupdateActivationsPlot()));
    connect(ui->Arest_slider, SIGNAL(sliderReleased()), this, SLOT(autoupdateActivationsPlot()));
    connect(ui->MaxFreq_spinBox, SIGNAL(editingFinished()), this, SLOT(autoupdateActivationsPlot()));

    ///// RECENT FILES
    //////////////////////////////////////
    ui->menuFile->addSeparator();
    for (int i = 0; i < MaxRecentFiles; ++i) {
        recentFileActs[i] = new QAction(this);
        recentFileActs[i]->setVisible(false);
        connect(recentFileActs[i], SIGNAL(triggered()), this,
                SLOT(openRecentFile()));
        ui->menuFile->addAction(recentFileActs[i]);
    }
    // reload the list of recent files
    updateRecentFileActions();

    //////////////////////////////////////

    ///// PLOTS
    //////////////////////////////////////

    initializeWeightsPlot();
    initializeActivationsPlot();
    //////////////////////////////////////

    // Read the template experiment and update accordingly the interface
    on_experiment_editor_textChanged();
    setWindowModified(false);
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::initializeWeightsPlot() {
    // set some pens, brushes and backgrounds:
    ui->weightPlot->xAxis->setBasePen(
        QPen(QColor(140, 140, 140), 2, Qt::SolidLine));
    ui->weightPlot->yAxis->setBasePen(
        QPen(QColor(140, 140, 140), 2, Qt::SolidLine));
    ui->weightPlot->xAxis->setTickPen(Qt::NoPen);
    ui->weightPlot->yAxis->setTickPen(Qt::NoPen);
    ui->weightPlot->xAxis->setSubTickPen(Qt::NoPen);
    ui->weightPlot->yAxis->setSubTickPen(Qt::NoPen);
    ui->weightPlot->xAxis->setTickLabelColor(Qt::white);
    ui->weightPlot->yAxis->setTickLabelColor(Qt::white);
    ui->weightPlot->xAxis->setAutoTickStep(false);
    ui->weightPlot->yAxis->setAutoTickStep(false);
    ui->weightPlot->xAxis->setAutoSubTicks(false);
    ui->weightPlot->yAxis->setAutoSubTicks(false);

    ui->weightPlot->xAxis->setTickStep(1);
    ui->weightPlot->yAxis->setTickStep(1);
    ui->weightPlot->xAxis->setSubTickCount(1);
    ui->weightPlot->yAxis->setSubTickCount(1);
    ui->weightPlot->yAxis->setTickLabelRotation(-90);
    ui->weightPlot->xAxis->grid()->setPen(Qt::NoPen);
    ui->weightPlot->yAxis->grid()->setPen(Qt::NoPen);
    ui->weightPlot->xAxis->grid()->setSubGridVisible(true);
    ui->weightPlot->yAxis->grid()->setSubGridVisible(true);
    ui->weightPlot->xAxis->grid()->setSubGridPen(
        QPen(QColor(140, 140, 140), 2, Qt::SolidLine));
    ui->weightPlot->yAxis->grid()->setSubGridPen(
        QPen(QColor(140, 140, 140), 2, Qt::SolidLine));
    ui->weightPlot->xAxis->grid()->setZeroLinePen(Qt::NoPen);
    ui->weightPlot->yAxis->grid()->setZeroLinePen(Qt::NoPen);
    QLinearGradient plotGradient;
    plotGradient.setStart(0, 0);
    plotGradient.setFinalStop(0, 350);
    plotGradient.setColorAt(0, QApplication::palette().color(QPalette::Base));
    plotGradient.setColorAt(1, QApplication::palette().color(QPalette::Base));
    ui->weightPlot->setBackground(plotGradient);
    QLinearGradient axisRectGradient;
    axisRectGradient.setStart(0, 0);
    axisRectGradient.setFinalStop(0, 350);
    axisRectGradient.setColorAt(0, QColor(80, 80, 80));
    axisRectGradient.setColorAt(1, QColor(30, 30, 30));
    ui->weightPlot->axisRect()->setBackground(axisRectGradient);

    // display the grid on top of the graph
    if (!ui->weightPlot->layer("abovemain")) {
        ui->weightPlot->addLayer("abovemain", ui->weightPlot->layer("main"),
                                 QCustomPlot::limAbove);
    }
    ui->weightPlot->xAxis->grid()->setLayer("abovemain");
    ui->weightPlot->yAxis->grid()->setLayer("abovemain");

    ui->weightPlot->xAxis->setAutoTicks(false);
    ui->weightPlot->yAxis->setAutoTicks(false);
    ui->weightPlot->xAxis->setAutoTickLabels(false);
    ui->weightPlot->yAxis->setAutoTickLabels(false);

    // set up the QCPColorMap:
    QCPColorMap *colorMap =
        new QCPColorMap(ui->weightPlot->xAxis, ui->weightPlot->yAxis);
    ui->weightPlot->addPlottable(colorMap);
    colorMap->setInterpolate(false);

    // add a color scale:
    QCPColorScale *colorScale = new QCPColorScale(ui->weightPlot);
    ui->weightPlot->plotLayout()->addElement(
        0, 1, colorScale);  // add it to the right of the main axis rect
    colorScale->setType(QCPAxis::atRight);  // scale shall be vertical bar with
                                            // tick/axis labels right (actually
                                            // atRight is already the default)
    colorMap->setColorScale(
        colorScale);  // associate the color map with the color scale
    colorScale->axis()->setLabel("Weights");

    // set the color gradient of the color map to one of the presets:
    colorMap->setGradient(QCPColorGradient::gpHot);
    // we could have also created a QCPColorGradient instance and added own
    // colors to
    // the gradient, see the documentation of QCPColorGradient for what's
    // possible.

    // make sure the axis rect and color scale synchronize their bottom and top
    // margins (so they line up):
    QCPMarginGroup *marginGroup = new QCPMarginGroup(ui->weightPlot);
    ui->weightPlot->axisRect()->setMarginGroup(QCP::msBottom | QCP::msTop,
                                               marginGroup);
    colorScale->setMarginGroup(QCP::msBottom | QCP::msTop, marginGroup);

    ui->weightPlot->replot();
}

void MainWindow::prepareWeightsPlot() {
    QVector<double> tickVector;
    QVector<QString> tickLabels;
    for (size_t i = 0; i < memory->units_names().size(); i++) {
        tickVector << i;
        tickLabels << QString::fromStdString(memory->units_names()[i]);
    }
    tickVector << memory->units_names().size();
    tickLabels << "";

    ui->weightPlot->xAxis->setTickVector(tickVector);
    ui->weightPlot->yAxis->setTickVector(tickVector);
    ui->weightPlot->xAxis->setTickVectorLabels(tickLabels);
    ui->weightPlot->yAxis->setTickVectorLabels(tickLabels);

    auto weights = memory->weights();

    // set up the QCPColorMap:
    QCPColorMap *colorMap =
        dynamic_cast<QCPColorMap *>(ui->weightPlot->plottable());

    colorMap->data()->setSize(
        weights.cols(),
        weights.rows());  // we want the color map to have nx * ny data points
    colorMap->data()->setRange(
        QCPRange(0, weights.cols() - 1),
        QCPRange(0, weights.rows() - 1));  // and span the coordinate range
                                           // -4..4 in both key (x) and value
                                           // (y) dimensions

    colorMap->setDataRange(QCPRange(memory->Winit,2));
    ui->weightPlot->xAxis->setRange(-0.5, memory->units_names().size() - 0.5);
    ui->weightPlot->yAxis->setRange(-0.5, memory->units_names().size() - 0.5);

    ui->weightPlot->replot();
}

void MainWindow::updateWeightsPlot() {
    auto weights = memory->weights();

    // set up the QCPColorMap:
    QCPColorMap *colorMap =
        dynamic_cast<QCPColorMap *>(ui->weightPlot->plottable());

    for (int xIndex = 0; xIndex < weights.cols(); xIndex++) {
        for (int yIndex = 0; yIndex < weights.rows(); yIndex++) {
            colorMap->data()->setCell(xIndex, yIndex, weights(xIndex, yIndex));
        }
    }
    ui->weightPlot->replot();
}

void MainWindow::initializeActivationsPlot() {
    // ensure selection of graphs and legend are synchronized
    connect(ui->activationPlot, SIGNAL(selectionChangedByUser()), this,
            SLOT(selectionChanged()));
    // set some pens, brushes and backgrounds:
    ui->activationPlot->xAxis->setBasePen(QPen(Qt::white, 1));
    ui->activationPlot->yAxis->setBasePen(QPen(Qt::white, 1));
    ui->activationPlot->xAxis->setTickPen(QPen(Qt::white, 1));
    ui->activationPlot->yAxis->setTickPen(QPen(Qt::white, 1));
    ui->activationPlot->xAxis->setSubTickPen(QPen(Qt::white, 1));
    ui->activationPlot->yAxis->setSubTickPen(QPen(Qt::white, 1));
    ui->activationPlot->xAxis->setTickLabelColor(Qt::white);
    ui->activationPlot->yAxis->setTickLabelColor(Qt::white);
    ui->activationPlot->xAxis->grid()->setPen(
        QPen(QColor(140, 140, 140), 1, Qt::DotLine));
    ui->activationPlot->yAxis->grid()->setPen(
        QPen(QColor(140, 140, 140), 1, Qt::DotLine));
    ui->activationPlot->xAxis->grid()->setSubGridPen(
        QPen(QColor(80, 80, 80), 1, Qt::DotLine));
    ui->activationPlot->yAxis->grid()->setSubGridPen(
        QPen(QColor(80, 80, 80), 1, Qt::DotLine));
    ui->activationPlot->xAxis->grid()->setSubGridVisible(true);
    ui->activationPlot->yAxis->grid()->setSubGridVisible(true);
    ui->activationPlot->xAxis->grid()->setZeroLinePen(Qt::NoPen);
    ui->activationPlot->yAxis->grid()->setZeroLinePen(Qt::NoPen);
    ui->activationPlot->xAxis->setUpperEnding(QCPLineEnding::esSpikeArrow);
    ui->activationPlot->yAxis->setUpperEnding(QCPLineEnding::esSpikeArrow);
    QLinearGradient plotGradient;
    plotGradient.setStart(0, 0);
    plotGradient.setFinalStop(0, 350);
    plotGradient.setColorAt(0, QApplication::palette().color(QPalette::Base));
    plotGradient.setColorAt(1, QApplication::palette().color(QPalette::Base));
    ui->activationPlot->setBackground(plotGradient);
    QLinearGradient axisRectGradient;
    axisRectGradient.setStart(0, 0);
    axisRectGradient.setFinalStop(0, 350);
    axisRectGradient.setColorAt(0, QColor(80, 80, 80));
    axisRectGradient.setColorAt(1, QColor(30, 30, 30));
    ui->activationPlot->axisRect()->setBackground(axisRectGradient);

    // give the axes some labels:
    ui->activationPlot->xAxis->setLabel("Time (ms)");
    ui->activationPlot->yAxis->setLabel("Activation level");
    // set axes ranges, so we see all data:
    ui->activationPlot->xAxis->setRange(0, 100);
    ui->activationPlot->yAxis->setRange(-0.2, 1);

    ui->activationPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom |
                                        QCP::iSelectLegend | QCP::iSelectPlottables);

    connect(ui->activationPlot,
            SIGNAL(legendDoubleClick(QCPLegend *, QCPAbstractLegendItem *,
                                     QMouseEvent *)),
            this, SLOT(activationsLegendDoubleClick(QCPLegend *,
                                                    QCPAbstractLegendItem *)));

    ui->activationPlot->replot();
}

void MainWindow::prepareActivationsPlot() {

    QPen graphSelectedPen;
    graphSelectedPen.setColor(Qt::darkRed);
    graphSelectedPen.setWidthF(3);

    // create one graph per unit
    ui->activationPlot->clearGraphs();
    for (size_t i = 0; i < memory->units_names().size(); i++) {
        auto graph = ui->activationPlot->addGraph();
        graph->setName(QString::fromStdString(memory->units_names()[i]));

        QPen graphPen;
        graphPen.setColor(PALETTE[i % PALETTE.size()]);
        graphPen.setWidthF(2);
        graph->setPen(graphPen);
        graph->setSelectedPen(graphSelectedPen);
    }

    // create graphs for external activations
    for (size_t i = 0; i < memory->units_names().size(); i++) {
        auto graph = ui->activationPlot->addGraph();
        graph->setName(QString::fromStdString(memory->units_names()[i]) +
                       " (ext.)");

        graph->setBrush(fillColor);
        graph->setSelectedBrush(fillColor);
        QPen graphPen;
        graphPen.setColor(PALETTE[i % PALETTE.size()]);
        graphPen.setWidthF(1);
        graphPen.setStyle(Qt::DashLine);
        graph->setPen(graphPen);
        graph->setSelectedPen(graphSelectedPen);
        graph->setChannelFillGraph(ui->activationPlot->graph(i));

        // initially hidden!
        graph->setVisible(false);
        ui->activationPlot->legend->itemWithPlottable(graph)
            ->setTextColor(QColor(100, 100, 100));
    }

    ui->activationPlot->xAxis->setRange(
        0, duration_cast<milliseconds>(expe.duration).count());
    ui->activationPlot->yAxis->setRange(memory->get_parameter("Amin"),
                                        memory->get_parameter("Amax"));

    ui->activationPlot->legend->setVisible(true);
    QFont legendFont = font();
    legendFont.setPointSize(10);
    ui->activationPlot->legend->setFont(legendFont);
    ui->activationPlot->legend->setSelectedFont(legendFont);
    ui->activationPlot->legend->setSelectableParts(
        QCPLegend::spItems);  // legend box shall not be selectable, only legend
                              // items

    ui->activationPlot->replot();
}

void MainWindow::updateActivationsPlot() {

    auto qTimestamps = QVector<double>::fromStdVector(activations_timestamps);

    for (const auto &kv : activations_logs) {
        auto data = QVector<double>::fromStdVector(kv.second);
        ui->activationPlot->graph(kv.first)
                          ->setData(qTimestamps, data);
    }

    auto qExternalTimestamps = QVector<double>::fromStdVector(external_activations_timestamps);

    for (const auto &kv : external_activations_logs) {
        auto data = QVector<double>::fromStdVector(kv.second);
        ui->activationPlot->graph(kv.first + external_activations_logs.size())
                          ->setData(qExternalTimestamps, data);
    }

    ui->activationPlot->replot();
}

void MainWindow::toggleGraph(QCPPlottableLegendItem* plItem, bool only_hide, bool only_show)
{
        for (int i = 0; i < ui->activationPlot->graphCount(); ++i) {
            QCPGraph *graph = ui->activationPlot->graph(i);
            if (plItem ==
                ui->activationPlot->legend->itemWithPlottable(graph)) {
                if (graph->visible() && !only_show) {
                    plItem->setTextColor(QColor(100, 100, 100));
                    graph->setVisible(false);

                    // if the graph is an activation graph, hide
                    // the fill on the corresponding external activation
                    if(i < ui->activationPlot->graphCount() / 2)
                    {
                        ui->activationPlot->graph(ui->activationPlot->graphCount() / 2 + i)->setBrush(Qt::NoBrush);
                        ui->activationPlot->graph(ui->activationPlot->graphCount() / 2 + i)->setSelectedBrush(Qt::NoBrush);
                    }

                } else if (!graph->visible() && !only_hide) {
                    plItem->setTextColor(QColor(0, 0, 0));
                    graph->setVisible(true);

                    // if the graph is an activation graph, show
                    // the fill on the corresponding external activation
                    if(i < ui->activationPlot->graphCount() / 2)
                    {
                        ui->activationPlot->graph(ui->activationPlot->graphCount() / 2 + i)->setBrush(fillColor);
                        ui->activationPlot->graph(ui->activationPlot->graphCount() / 2 + i)->setSelectedBrush(fillColor);
                    }

                }
                plItem->setSelected(false); // does not seem to do anything... :(
                graph->setSelected(false); // does not seem to do anything... :(


                return;
            }
        }

}

/** Hide/show a graph by double clicking on the legend
 */
void MainWindow::activationsLegendDoubleClick(QCPLegend *legend,
                                              QCPAbstractLegendItem *item) {

    Q_UNUSED(legend)
    if (item)  // only react if item was clicked (user could have clicked on
               // border padding of legend where there is no item, then item is
               // 0)
    {
        QCPPlottableLegendItem *plItem =
            qobject_cast<QCPPlottableLegendItem *>(item);

        toggleGraph(plItem);
        ui->activationPlot->replot();
    }
}

void MainWindow::setCurrentFile(const QString &fileName) {
    curFile = fileName;
    QFileInfo f(curFile);
    setWindowTitle("Associative Memory Explorer - " + f.fileName() + "[*]");

    QSettings settings;
    QStringList files = settings.value("recentFileList").toStringList();
    files.removeAll(fileName);
    files.prepend(fileName);
    while (files.size() > MaxRecentFiles) files.removeLast();

    settings.setValue("recentFileList", files);

    foreach (QWidget *widget, QApplication::topLevelWidgets()) {
        MainWindow *mainWin = qobject_cast<MainWindow *>(widget);
        if (mainWin) mainWin->updateRecentFileActions();
    }
}

void MainWindow::updateRecentFileActions() {
    QSettings settings;
    QStringList files = settings.value("recentFileList").toStringList();

    int numRecentFiles = qMin(files.size(), (int)MaxRecentFiles);

    for (int i = 0; i < numRecentFiles; ++i) {
        QString text = tr("&%1 %2").arg(i + 1).arg(strippedName(files[i]));
        recentFileActs[i]->setText(text);
        recentFileActs[i]->setData(files[i]);
        recentFileActs[i]->setVisible(true);
    }
    for (int j = numRecentFiles; j < MaxRecentFiles; ++j)
        recentFileActs[j]->setVisible(false);
}

QString MainWindow::strippedName(const QString &fullFileName) {
    return QFileInfo(fullFileName).fileName();
}

void MainWindow::on_runButton_clicked() {
    QApplication::setOverrideCursor(Qt::WaitCursor);

    ui->runButton->setText("Running...");
    ui->runButton->setToolTip("Experiment running...");
    statusBar()->showMessage(
        "Experiment running... - Total duration: " +
        QString::number(duration_cast<milliseconds>(expe.duration).count()) +
        "ms");
    ui->runButton->setDisabled(true);

    // reset logs
    activations_logs.clear();
    activations_timestamps.clear();
    external_activations_logs.clear();
    external_activations_timestamps.clear();

    _last_activations_log = microseconds(0);
    _last_external_activations_log = microseconds(0);

    memory->reset();

    memory->start();

    int last_activation = 0;
    auto start = high_resolution_clock::now();

    for (const auto &kv : expe.activations) {
        this_thread::sleep_for(milliseconds(kv.first - last_activation));

        for (auto &activation : kv.second) {
            cerr << " - Activating " << activation.first << " for "
                 << activation.second.count() << "ms" << endl;
            memory->activate_unit(activation.first, 1.0, activation.second);
        }

        last_activation = kv.first;
    }

    auto remaining_time =
        expe.duration - (high_resolution_clock::now() - start);
    this_thread::sleep_for(remaining_time);

    memory->stop();
    cerr << endl
         << "EXPERIMENT COMPLETED. Total duration: "
         << duration_cast<std::chrono::milliseconds>(
                high_resolution_clock::now() - start).count() << "ms" << endl;

    statusBar()->showMessage("Experiment completed!", 2000);

    updateActivationsPlot();
    updateWeightsPlot();

    ui->runButton->setToolTip("Start the experiment");
    ui->runButton->setText("Run!");

    ui->runButton->setDisabled(false);

    QApplication::restoreOverrideCursor();
}

void MainWindow::on_actionOpen_triggered() {
    auto conf =
        QFileDialog::getOpenFileName(this, tr("Open experiment"), "",
                                     tr("Experiment Files (*.md *.csv *.txt)"));

    loadExperiment(conf);
}

void MainWindow::openRecentFile() {
    QAction *action = qobject_cast<QAction *>(sender());
    if (action) loadExperiment(action->data().toString());
}

void MainWindow::loadExperiment(const QString &filename) {
    ifstream experiment(filename.toStdString());

    string str((istreambuf_iterator<char>(experiment)),
               istreambuf_iterator<char>());

    string::const_iterator iter = str.begin();
    string::const_iterator end = str.end();

    experiment_grammar<string::const_iterator> experiment_parser;

    bool r = qi::phrase_parse(iter, end, experiment_parser, ascii::space);

    if (r && iter == str.end()) {
        experiment_parser.expe.summary();
        setCurrentFile(filename);
        ui->experiment_editor->setPlainText(QString::fromStdString(str));
        setWindowModified(false);
    } else {
        QMessageBox::critical(this, "Invalid experiment file",
                              QString("Parsing of ") + filename + " failed!");
    }

    setupExperiment(experiment_parser.expe);
}
void MainWindow::setupExperiment(const Experiment &_expe) {
    expe = _expe;

    auto activations_logging =
        std::bind(logging, ref(activations_logs), ref(activations_timestamps),
                  ref(_last_activations_log), std::placeholders::_1,
                  std::placeholders::_2);

    auto external_activations_logging =
        std::bind(logging, ref(external_activations_logs),
                  ref(external_activations_timestamps),
                  ref(_last_external_activations_log), std::placeholders::_1,
                  std::placeholders::_2);

    memory = make_unique<MemoryNetwork>(expe.units.size(), activations_logging,
                                        external_activations_logging);

    memory->units_names(expe.units);

    set_param("Dg")
    set_ui_param(Dg, "Decay")
    set_param("Lg")
    set_ui_param(Lg, "Learning")
    set_param("Eg")
    set_ui_param(Eg, "Ext. influence")
    set_param("Ig")
    set_ui_param(Ig, "Int. influence")
    set_param("Amax")
    set_ui_param(Amax, "Act. max")
    set_param("Amin")
    set_ui_param(Amin, "Act. min")
    set_param("Arest")
    set_ui_param(Arest, "Act. rest")
    set_param("Winit")

    if (expe.parameters.count("MaxFreq")) {
        memory->max_frequency(expe.parameters.at("MaxFreq"));
        ui->MaxFreq_spinBox->setValue(expe.parameters.at("MaxFreq"));
        ui->period_label->setText("dt: " + QString::number(duration_cast<microseconds>(memory->internal_period()).count()) + "us");
    }

    prepareActivationsPlot();
    prepareWeightsPlot();

    ui->runButton->setToolTip("Start the experiment");
    ui->runButton->setDisabled(false);
}

void MainWindow::saveFile(const QString &fileName) {
    QFile file(fileName);
    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("Saving experiment"),
                             tr("Cannot write file %1:\n%2.")
                                 .arg(fileName)
                                 .arg(file.errorString()));
        return;
    }

    QTextStream out(&file);
    QApplication::setOverrideCursor(Qt::WaitCursor);
    out << ui->experiment_editor->toPlainText();
    QApplication::restoreOverrideCursor();

    setCurrentFile(fileName);
    statusBar()->showMessage(tr("Experiment saved"), 2000);
    setWindowModified(false);
}

void MainWindow::selectionChanged() {
    // synchronize selection of graphs with selection of corresponding legend
    // items:
    for (int i = 0; i < ui->activationPlot->graphCount(); ++i) {
        QCPGraph *graph = ui->activationPlot->graph(i);
        QCPPlottableLegendItem *item =
            ui->activationPlot->legend->itemWithPlottable(graph);
        if (item->selected() || graph->selected()) {
            item->setSelected(true);
            graph->setSelected(true);
        }
    }
}

void MainWindow::update_expe_description_parameter(const QString &name,
                                                   double value) {
    auto expe_desc = ui->experiment_editor->toPlainText();
    auto init_text = expe_desc;
    expe_desc.replace(QRegExp(name + ": [0-9\\.\\-]*"),
                      name + ": " + QString::number(value));

    if (init_text != expe_desc) {
        ui->experiment_editor->setPlainText(expe_desc);
        setWindowModified(true);
    }
}

void MainWindow::updateParameter(const string& name, double value) {

    if (memory) {
        memory->set_parameter(name, value);
    }

    update_expe_description_parameter(QString::fromStdString(name), value);


}

void MainWindow::on_Dg_slider_sliderMoved(int position) {
    auto Dg = position * 1. / 100;
    set_ui_param(Dg, "Decay")
    updateParameter("Dg", Dg);

 }


void MainWindow::autoupdateActivationsPlot()
{
     if(ui->autoupdate_checkbox->isChecked()) {
        on_runButton_clicked();
    }

}

void MainWindow::on_Lg_slider_sliderMoved(int position) {
    auto Lg = position * 1. / 100;
    set_ui_param(Lg, "Learning")

    updateParameter("Lg", Lg);
}

void MainWindow::on_Eg_slider_sliderMoved(int position) {
    auto Eg = position * 1. / 100;
    set_ui_param(Eg, "Ext. influence")

    updateParameter("Eg", Eg);

}

void MainWindow::on_Ig_slider_sliderMoved(int position) {
    auto Ig = position * 1. / 100;
    set_ui_param(Ig, "Int. influence")

    updateParameter("Ig", Ig);

}

void MainWindow::on_Amax_slider_sliderMoved(int position) {
    auto Amax = position * 1. / 100;
    set_ui_param(Amax, "Act. max")

    updateParameter("Amax", Amax);

}

void MainWindow::on_Amin_slider_sliderMoved(int position) {
    auto Amin = position * 1. / 100;
    set_ui_param(Amin, "Act. min")

    updateParameter("Amin", Amin);
}

void MainWindow::on_Arest_slider_sliderMoved(int position) {
    auto Arest = position * 1. / 100;
    set_ui_param(Arest, "Act. rest")

    updateParameter("Arest", Arest);
}

void MainWindow::on_MaxFreq_spinBox_valueChanged(int MaxFreq) {

    if (memory) {
        memory->max_frequency(MaxFreq);
    }

    update_expe_description_parameter("MaxFreq", MaxFreq);
}

void MainWindow::on_experiment_editor_textChanged() {
    setWindowModified(true);

    auto description = ui->experiment_editor->toPlainText().toStdString();

    string::const_iterator iter = description.begin();
    string::const_iterator end = description.end();

    experiment_grammar<string::const_iterator> experiment_parser;

    bool r = qi::phrase_parse(iter, end, experiment_parser, ascii::space);

    if (r && iter == description.end()) {
        ui->experiment_parsing_status->setText("ok");
        setupExperiment(experiment_parser.expe);
    } else {
        ui->experiment_parsing_status->setText(
            "invalid experiment description");
    }
}

void MainWindow::on_actionSave_triggered() {
    if (curFile.isEmpty())
        on_action_Save_as_triggered();
    else
        saveFile(curFile);
}

void MainWindow::on_action_Save_as_triggered() {
    QString fileName = QFileDialog::getSaveFileName(
        this, tr("Save experiment"), "", tr("Experiment Files (*.md *.txt)"));
    if (fileName.isEmpty()) return;

    saveFile(fileName);
}

void MainWindow::on_actionQuit_triggered() { QApplication::closeAllWindows(); }

void MainWindow::on_export_activation_plot_clicked() {
    QString fileName =
        QFileDialog::getSaveFileName(this, tr("Export activations plot to PDF"),
                                     "", tr("PDF Document (*.pdf)"));
    if (fileName.isEmpty()) return;

    ui->activationPlot->savePdf(fileName, true, 0, 0,
                                "Associative Memory Explorer",
                                "Activations plot for experiment <" +
                                    QString::fromStdString(expe.name) + ">");
    QFileInfo f(fileName);
    statusBar()->showMessage(tr("Plot exported to ") + f.fileName(), 2000);
}

void MainWindow::on_show_all_activations_plots_clicked()
{
        for (int i = 0; i < ui->activationPlot->graphCount(); ++i) {
                QCPGraph *graph = ui->activationPlot->graph(i);
                toggleGraph(ui->activationPlot->legend->itemWithPlottable(graph), false, true);
        }
        ui->activationPlot->replot();

}
void MainWindow::on_hide_all_activations_plots_clicked()
{
        for (int i = 0; i < ui->activationPlot->graphCount(); ++i) {
                QCPGraph *graph = ui->activationPlot->graph(i);
                toggleGraph(ui->activationPlot->legend->itemWithPlottable(graph), true, false);
        }
        ui->activationPlot->replot();

}

void MainWindow::on_export_weights_plot_clicked() {
    QString fileName = QFileDialog::getSaveFileName(
        this, tr("Export weights plot to PDF"), "", tr("PDF Document (*.pdf)"));
    if (fileName.isEmpty()) return;

    ui->weightPlot->savePdf(fileName, true, 0, 0, "Associative Memory Explorer",
                            "Weights plot for experiment <" +
                                QString::fromStdString(expe.name) + ">");
    QFileInfo f(fileName);
    statusBar()->showMessage(tr("Plot exported to ") + f.fileName(), 2000);
}


SimpleMarkdownHighlighter::SimpleMarkdownHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    HighlightingRule rule;

    QTextCharFormat lineFormat;
    lineFormat.setForeground(Qt::darkBlue);
    lineFormat.setFontWeight(QFont::Bold);

    QTextCharFormat paramFormat;
    paramFormat.setForeground(Qt::darkGreen);
    paramFormat.setFontWeight(QFont::Bold);

    QTextCharFormat unitFormat;
    unitFormat.setForeground(Qt::darkYellow);
    unitFormat.setFontWeight(QFont::Bold);

    QTextCharFormat errorFormat;
    errorFormat.setUnderlineStyle(QTextCharFormat::SpellCheckUnderline);
    errorFormat.setUnderlineColor(Qt::red);
    errorFormat.setFontUnderline(true);

    rule.pattern = QRegExp("^[\\-=]{2,}");
    rule.format = lineFormat;
    highlightingRules.append(rule);

    rule.pattern = QRegExp("^\\-\\s[A-Za-z0-9\\-_]+:\\s*\\-?[\\.0-9]+");
    rule.format = paramFormat;
    highlightingRules.append(rule);

    rule.pattern = QRegExp("^\\-\\s[A-Za-z0-9\\-_]+:?\\s*$");
    rule.format = unitFormat;
    highlightingRules.append(rule);
}

void SimpleMarkdownHighlighter::highlightBlock(const QString &text)
{
    for(const auto& rule : highlightingRules) {
        QRegExp expression(rule.pattern);
        int index = expression.indexIn(text);
        while (index >= 0) {
            int length = expression.matchedLength();
            setFormat(index, length, rule.format);
            index = expression.indexIn(text, index + length);
        }
    }
}
