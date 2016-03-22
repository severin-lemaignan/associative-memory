#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <memory>
#include <QMainWindow>

#include "../src/memory_network.hpp"
#include "../src-runner/experiment.hpp"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_Dg_slider_sliderMoved(int position);

    void on_actionOpen_triggered();

    void on_runButton_clicked();

    void selectionChanged();

private:
    Ui::MainWindow *ui;

    std::unique_ptr<MemoryNetwork> memory;

    Experiment expe;
};

#endif // MAINWINDOW_H
