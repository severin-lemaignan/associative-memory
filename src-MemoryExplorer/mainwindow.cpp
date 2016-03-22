#include <string>
#include <fstream>
#include <iostream>
#include <chrono>
#include <map>
#include <vector>

#include <QMessageBox>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "../src-runner/parser.hpp"

#define set_param(PARAM) if(expe.parameters.count(PARAM)) {memory->set_parameter(PARAM, expe.parameters[PARAM]);}
#define S(x) #x
#define SX(x) S(x)
#define set_ui_param(PARAM) ui->PARAM##_label->setText(SX(PARAM) ": " + QString::number(memory->get_parameter(SX(PARAM))));ui->PARAM##_slider->setValue(memory->get_parameter(SX(PARAM))*100);


using namespace std;
using namespace std::chrono;

const vector<QColor> PALETTE = {QColor(242,146,70),
                               QColor(242,214,70),
                               QColor(101,156,27),
                               QColor(27,156,154),
                               QColor(37,68,171),
                               QColor(110,37,171),
                               QColor(171,37,124),
                               QColor(204,58,26),
                               QColor(191,180,163),
                               QColor(80,88,89)
                               };

const int HISTORY_SAMPLING_RATE=500; //Hz

vector<double> timestamps;
map<size_t, vector<double>> logs;

microseconds _last_log;

void logging(microseconds time_from_start,
             const MemoryVector& levels)
{

    // if necessary, store the activation level
    auto us_since_last_log = time_from_start - _last_log;
    if(us_since_last_log.count() > (1000000./HISTORY_SAMPLING_RATE)) {
        _last_log = time_from_start;
        timestamps.push_back(duration_cast<milliseconds>(time_from_start).count());
        for(size_t i = 0; i < levels.size(); i++) {
            logs[i].push_back(levels[i]);
        }
    }


}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    QCoreApplication::setOrganizationName("Plymouth_University");
    QCoreApplication::setApplicationName("MemoryExplorer");

    ui->setupUi(this);

    ui->menuFile->addSeparator();
    for (int i = 0; i < MaxRecentFiles; ++i) {

        recentFileActs[i] = new QAction(this);
        recentFileActs[i]->setVisible(false);
        connect(recentFileActs[i], SIGNAL(triggered()), this, SLOT(openRecentFile()));
        ui->menuFile->addAction(recentFileActs[i]);
    }

    // reload the list of recent files
    updateRecentFileActions();

    // ensure selection of graphs and legend are synchronized
    connect(ui->customPlot, SIGNAL(selectionChangedByUser()), this, SLOT(selectionChanged()));

    prepareWeightPlot();

    // set some pens, brushes and backgrounds:
    ui->customPlot->xAxis->setBasePen(QPen(Qt::white, 1));
    ui->customPlot->yAxis->setBasePen(QPen(Qt::white, 1));
    ui->customPlot->xAxis->setTickPen(QPen(Qt::white, 1));
    ui->customPlot->yAxis->setTickPen(QPen(Qt::white, 1));
    ui->customPlot->xAxis->setSubTickPen(QPen(Qt::white, 1));
    ui->customPlot->yAxis->setSubTickPen(QPen(Qt::white, 1));
    ui->customPlot->xAxis->setTickLabelColor(Qt::white);
    ui->customPlot->yAxis->setTickLabelColor(Qt::white);
    ui->customPlot->xAxis->grid()->setPen(QPen(QColor(140, 140, 140), 1, Qt::DotLine));
    ui->customPlot->yAxis->grid()->setPen(QPen(QColor(140, 140, 140), 1, Qt::DotLine));
    ui->customPlot->xAxis->grid()->setSubGridPen(QPen(QColor(80, 80, 80), 1, Qt::DotLine));
    ui->customPlot->yAxis->grid()->setSubGridPen(QPen(QColor(80, 80, 80), 1, Qt::DotLine));
    ui->customPlot->xAxis->grid()->setSubGridVisible(true);
    ui->customPlot->yAxis->grid()->setSubGridVisible(true);
    ui->customPlot->xAxis->grid()->setZeroLinePen(Qt::NoPen);
    ui->customPlot->yAxis->grid()->setZeroLinePen(Qt::NoPen);
    ui->customPlot->xAxis->setUpperEnding(QCPLineEnding::esSpikeArrow);
    ui->customPlot->yAxis->setUpperEnding(QCPLineEnding::esSpikeArrow);
    QLinearGradient plotGradient;
    plotGradient.setStart(0, 0);
    plotGradient.setFinalStop(0, 350);
    plotGradient.setColorAt(0, QApplication::palette().color(QPalette::Base));
    plotGradient.setColorAt(1, QApplication::palette().color(QPalette::Base));
    ui->customPlot->setBackground(plotGradient);
    QLinearGradient axisRectGradient;
    axisRectGradient.setStart(0, 0);
    axisRectGradient.setFinalStop(0, 350);
    axisRectGradient.setColorAt(0, QColor(80, 80, 80));
    axisRectGradient.setColorAt(1, QColor(30, 30, 30));
    ui->customPlot->axisRect()->setBackground(axisRectGradient);

    // give the axes some labels:
    ui->customPlot->xAxis->setLabel("Time (ms)");
    ui->customPlot->yAxis->setLabel("Activation level");
    // set axes ranges, so we see all data:
    ui->customPlot->xAxis->setRange(0, 100);
    ui->customPlot->yAxis->setRange(-0.2, 1);

    ui->customPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectAxes |
                                      QCP::iSelectLegend | QCP::iSelectPlottables);


    ui->customPlot->replot();
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::prepareWeightPlot() {

    // set some pens, brushes and backgrounds:
    ui->weightPlot->xAxis->setBasePen(Qt::NoPen);
    ui->weightPlot->yAxis->setBasePen(Qt::NoPen);
    ui->weightPlot->xAxis->setTickPen(Qt::NoPen);
    ui->weightPlot->yAxis->setTickPen(Qt::NoPen);
    ui->weightPlot->xAxis->setSubTickPen(Qt::NoPen);
    ui->weightPlot->yAxis->setSubTickPen(Qt::NoPen);
    ui->weightPlot->xAxis->setTickLabelColor(Qt::white);
    ui->weightPlot->yAxis->setTickLabelColor(Qt::white);
    ui->weightPlot->xAxis->setAutoTickStep(false);
    ui->weightPlot->yAxis->setAutoTickStep(false);

    ui->weightPlot->xAxis->setTickStep(1);
    ui->weightPlot->yAxis->setTickStep(1);
    ui->weightPlot->xAxis->grid()->setPen(QPen(QColor(140, 140, 140), 2, Qt::SolidLine));
    ui->weightPlot->yAxis->grid()->setPen(QPen(QColor(140, 140, 140), 2, Qt::SolidLine));
    ui->weightPlot->xAxis->grid()->setSubGridVisible(false);
    ui->weightPlot->yAxis->grid()->setSubGridVisible(false);
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

    //ui->weightPlot->axisRect()->setupFullAxesBox(true);

    // display the grid on top of the graph
    ui->weightPlot->addLayer("abovemain", ui->weightPlot->layer("main"), QCustomPlot::limAbove);
    ui->weightPlot->xAxis->grid()->setLayer("abovemain");
    ui->weightPlot->yAxis->grid()->setLayer("abovemain");


    if(!memory) return;

    ui->weightPlot->xAxis->setAutoTicks(false);
    ui->weightPlot->yAxis->setAutoTicks(false);
    ui->weightPlot->xAxis->setAutoTickLabels(false);
    ui->weightPlot->yAxis->setAutoTickLabels(false);

    QVector<double> tickVector;
    QVector<QString> tickLabels;
    for(size_t i=0;i<memory->units_names().size();i++) {
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
    QCPColorMap *colorMap = new QCPColorMap(ui->weightPlot->xAxis, ui->weightPlot->yAxis);
    ui->weightPlot->addPlottable(colorMap);
    colorMap->data()->setSize(weights.cols(), weights.rows()); // we want the color map to have nx * ny data points
    colorMap->data()->setRange(QCPRange(0.5, weights.cols()-0.5), QCPRange(0.5, weights.rows()-0.5)); // and span the coordinate range -4..4 in both key (x) and value (y) dimensions
    // now we assign some data, by accessing the QCPColorMapData instance of the color map:

    for (int xIndex=0; xIndex<weights.cols(); xIndex++)
    {
      for (int yIndex=0; yIndex<weights.rows(); yIndex++)
      {
        colorMap->data()->setCell(xIndex, yIndex, weights(xIndex, yIndex));
      }
    }

    // add a color scale:
    QCPColorScale *colorScale = new QCPColorScale(ui->weightPlot);
    ui->weightPlot->plotLayout()->addElement(0, 1, colorScale); // add it to the right of the main axis rect
    colorScale->setType(QCPAxis::atRight); // scale shall be vertical bar with tick/axis labels right (actually atRight is already the default)
    colorMap->setColorScale(colorScale); // associate the color map with the color scale
    colorScale->axis()->setLabel("Weights");

    // set the color gradient of the color map to one of the presets:
    colorMap->setGradient(QCPColorGradient::gpPolar);
    // we could have also created a QCPColorGradient instance and added own colors to
    // the gradient, see the documentation of QCPColorGradient for what's possible.

    // rescale the data dimension (color) such that all data points lie in the span visualized by the color gradient:
    colorMap->rescaleDataRange();

    // make sure the axis rect and color scale synchronize their bottom and top margins (so they line up):
    QCPMarginGroup *marginGroup = new QCPMarginGroup(ui->weightPlot);
    ui->weightPlot->axisRect()->setMarginGroup(QCP::msBottom|QCP::msTop, marginGroup);
    colorScale->setMarginGroup(QCP::msBottom|QCP::msTop, marginGroup);

    // rescale the key (x) and value (y) axes so the whole color map is visible:
    //ui->weightPlot->rescaleAxes();
    //ui->customPlot->xAxis->setRange(0, 6);
    //ui->customPlot->yAxis->setRange(0, 6);

    ui->weightPlot->replot();
}

void MainWindow::setCurrentFile(const QString &fileName)
{
    QSettings settings;
    QStringList files = settings.value("recentFileList").toStringList();
    files.removeAll(fileName);
    files.prepend(fileName);
    while (files.size() > MaxRecentFiles)
        files.removeLast();

    settings.setValue("recentFileList", files);

    foreach (QWidget *widget, QApplication::topLevelWidgets()) {
        MainWindow *mainWin = qobject_cast<MainWindow *>(widget);
        if (mainWin)
            mainWin->updateRecentFileActions();
    }
}

void MainWindow::updateRecentFileActions()
{
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

QString MainWindow::strippedName(const QString &fullFileName)
{
    return QFileInfo(fullFileName).fileName();
}

void MainWindow::on_runButton_clicked()
{

   QApplication::setOverrideCursor(Qt::WaitCursor);

   ui->runButton->setText("Running...");
   ui->runButton->setToolTip("Experiment running...");
   ui->runButton->setDisabled(true);


   timestamps.clear();
   logs.clear();

   _last_log = microseconds(0);

   memory->reset();

   memory->start();

    int last_activation = 0;
    auto start = high_resolution_clock::now();

    for (const auto& kv : expe.activations) {
        this_thread::sleep_for(milliseconds(kv.first - last_activation));

        for (auto& activation : kv.second) {
            cerr << " - Activating " << activation.first << " for " << activation.second.count() << "ms" << endl;
            memory->activate_unit(activation.first, 1.0, activation.second);
        }

        last_activation = kv.first;
    }

    auto remaining_time = expe.duration - (high_resolution_clock::now() - start);
    this_thread::sleep_for(remaining_time);

    memory->stop();
    cerr << endl << "EXPERIMENT COMPLETED. Total duration: " << duration_cast<std::chrono::milliseconds>(high_resolution_clock::now() - start).count() << "ms" << endl;



    QVector<double> qTimestamps = QVector<double>::fromStdVector(timestamps);
    for (const auto& kv : logs) {
        QVector<double> data = QVector<double>::fromStdVector(kv.second);
        ui->customPlot->graph(kv.first)->setData(qTimestamps, data);
    }

    ui->customPlot->replot();

    prepareWeightPlot();

    ui->runButton->setToolTip("Start the experiment");
    ui->runButton->setText("Update");

    ui->runButton->setDisabled(false);


    QApplication::restoreOverrideCursor();


}

void MainWindow::on_actionOpen_triggered()
{
    auto conf = QFileDialog::getOpenFileName(this,
        tr("Open experiment"), "", tr("Experiment Files (*.md *.csv *.txt)"));

    loadExperiment(conf);
}

void MainWindow::openRecentFile()
{
    QAction *action = qobject_cast<QAction *>(sender());
    if (action)
        loadExperiment(action->data().toString());
}

void MainWindow::loadExperiment(const QString& filename) {

    ifstream experiment(filename.toStdString());


    string str((istreambuf_iterator<char>(experiment)),
                istreambuf_iterator<char>());

    string::const_iterator iter = str.begin();
    string::const_iterator end = str.end();

    experiment_grammar<string::const_iterator> experiment_parser;

    bool r = qi::phrase_parse(iter, end, experiment_parser,ascii::space);


    if (r && iter == str.end()) {
        experiment_parser.expe.summary();
        setCurrentFile(filename);
    } else {
        QMessageBox::critical(this,
                              "Invalid experiment file",
                              QString("Parsing of ") + filename + " failed!");
    }

    expe = experiment_parser.expe;

    memory = make_unique<MemoryNetwork>(expe.units.size(), &logging);
    memory->units_names(expe.units);

    if (expe.parameters.count("MaxFreq")) {
        memory->max_frequency(expe.parameters["MaxFreq"]);
    }

    set_param("Dg")
    set_ui_param(Dg)
    set_param("Lg")
    set_ui_param(Lg)
    set_param("Eg")
    set_ui_param(Eg)
    set_param("Ig")
    set_ui_param(Ig)
    set_param("Amax")
    set_ui_param(Amax)
    set_param("Amin")
    set_ui_param(Amin)
    set_param("Arest")
    set_ui_param(Arest)
    set_param("Winit")

    // create one graph per unit
    for(size_t i=0;i<memory->units_names().size();i++) {
        auto graph = ui->customPlot->addGraph();
        graph->setName(QString::fromStdString(memory->units_names()[i]));

        QPen graphPen;
        graphPen.setColor(PALETTE[i % PALETTE.size()]);
        graphPen.setWidthF(3);
        graph->setPen(graphPen);

    }

    ui->customPlot->xAxis->setRange(0, duration_cast<milliseconds>(expe.duration).count());
    ui->customPlot->yAxis->setRange(memory->get_parameter("Amin"), memory->get_parameter("Amax"));

    ui->customPlot->legend->setVisible(true);
    QFont legendFont = font();
    legendFont.setPointSize(10);
    ui->customPlot->legend->setFont(legendFont);
    ui->customPlot->legend->setSelectedFont(legendFont);
    ui->customPlot->legend->setSelectableParts(QCPLegend::spItems); // legend box shall not be selectable, only legend items


    ui->customPlot->replot();



   ui->runButton->setToolTip("Start the experiment");
   ui->runButton->setDisabled(false);

}

void MainWindow::selectionChanged()
{
    // synchronize selection of graphs with selection of corresponding legend items:
    for (int i=0; i<ui->customPlot->graphCount(); ++i)
    {
      QCPGraph *graph = ui->customPlot->graph(i);
      QCPPlottableLegendItem *item = ui->customPlot->legend->itemWithPlottable(graph);
      if (item->selected() || graph->selected())
      {
        item->setSelected(true);
        graph->setSelected(true);
      }
    }
}

void MainWindow::on_Dg_slider_sliderMoved(int position)
{
    if(memory) {
        auto Dg = position * 1. / 100;
        memory->set_parameter("Dg", Dg);
        set_ui_param(Dg)
    }
}


void MainWindow::on_Lg_slider_sliderMoved(int position)
{
     if(memory) {
        auto Lg = position * 1. / 100;
        memory->set_parameter("Lg", Lg);
        set_ui_param(Lg)
    }

}

void MainWindow::on_Eg_slider_sliderMoved(int position)
{
      if(memory) {
        auto Eg = position * 1. / 100;
        memory->set_parameter("Eg", Eg);
        set_ui_param(Eg)
    }

}

void MainWindow::on_Ig_slider_sliderMoved(int position)
{
     if(memory) {
        auto Ig = position * 1. / 100;
        memory->set_parameter("Ig", Ig);
        set_ui_param(Ig)
    }

}

void MainWindow::on_Amax_slider_sliderMoved(int position)
{
     if(memory) {
        auto Amax = position * 1. / 100;
        memory->set_parameter("Amax", Amax);
        set_ui_param(Amax)
    }

}

void MainWindow::on_Amin_slider_sliderMoved(int position)
{
     if(memory) {
        auto Amin = position * 1. / 100;
        memory->set_parameter("Amin", Amin);
        set_ui_param(Amin)
    }

}

void MainWindow::on_Arest_slider_sliderMoved(int position)
{
     if(memory) {
        auto Arest = position * 1. / 100;
        memory->set_parameter("Arest", Arest);
        set_ui_param(Arest)
    }

}
