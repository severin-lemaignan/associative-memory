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

using namespace std;
using namespace std::chrono;

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
    ui->setupUi(this);

    // ensure selection of graphs and legend are synchronized
    connect(ui->customPlot, SIGNAL(selectionChangedByUser()), this, SLOT(selectionChanged()));

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
    plotGradient.setColorAt(0, QColor(80, 80, 80));
    plotGradient.setColorAt(1, QColor(50, 50, 50));
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

void MainWindow::on_runButton_clicked()
{

   ui->runButton->setText("Running...");
   ui->runButton->setToolTip("Experiment running...");
   ui->runButton->setDisabled(true);

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

    ui->runButton->setToolTip("Start the experiment");
    ui->runButton->setText("Update");

    ui->runButton->setDisabled(false);


    QVector<double> qTimestamps = QVector<double>::fromStdVector(timestamps);
    for (const auto& kv : logs) {
        QVector<double> data = QVector<double>::fromStdVector(kv.second);
        ui->customPlot->graph(kv.first)->setData(qTimestamps, data);
    }

    ui->customPlot->replot();



}

void MainWindow::on_Dg_slider_sliderMoved(int position)
{
    auto Dg = position * 1. / 100;
    ui->Dg_label->setText(QString::fromStdString(std::string("Dg: ") + std::to_string(Dg)));
}

void MainWindow::on_actionOpen_triggered()
{
    auto conf = QFileDialog::getOpenFileName(this,
        tr("Open experiment"), "", tr("Experiment Files (*.md *.csv *.txt)"));

    ifstream experiment(conf.toStdString());


    string str((istreambuf_iterator<char>(experiment)),
                istreambuf_iterator<char>());

    string::const_iterator iter = str.begin();
    string::const_iterator end = str.end();

    experiment_grammar<string::const_iterator> experiment_parser;

    bool r = qi::phrase_parse(iter, end, experiment_parser,ascii::space);


    if (r && iter == str.end()) {
        experiment_parser.expe.summary();
    } else {
        QMessageBox::critical(this,
                              "Invalid experiment file",
                              QString("Parsing of ") + conf + " failed!");
    }

    expe = experiment_parser.expe;

    memory = make_unique<MemoryNetwork>(expe.units.size(), &logging);
    memory->units_names(expe.units);

    if (expe.parameters.count("MaxFreq")) {
        memory->max_frequency(expe.parameters["MaxFreq"]);
    }

#define set_param(PARAM) if(expe.parameters.count(PARAM)) {memory->set_parameter(PARAM, expe.parameters[PARAM]);}

    set_param("Dg")
    set_param("Lg")
    set_param("Eg")
    set_param("Ig")
    set_param("Amax")
    set_param("Amin")
    set_param("Arest")
    set_param("Winit")

    // create one graph per unit
    for(size_t i=0;i<memory->units_names().size();i++) {
        auto graph = ui->customPlot->addGraph();
        graph->setName(QString::fromStdString(memory->units_names()[i]));

        QPen graphPen;
        graphPen.setColor(QColor(rand()%245+10, rand()%245+10, rand()%245+10));
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
