#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <memory>
#include <QMainWindow>

#include "qcustomplot.h"
#include "../src/memory_network.hpp"
#include "../src-runner/experiment.hpp"

namespace Ui {
class MainWindow;
}

class SimpleMarkdownHighlighter;

class MainWindow : public QMainWindow {
    Q_OBJECT

   public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

   private slots:
    void on_Dg_slider_sliderMoved(int position);

    void on_actionOpen_triggered();

    void on_runButton_clicked();

    void selectionChanged();

    void on_Lg_slider_sliderMoved(int position);

    void on_Eg_slider_sliderMoved(int position);

    void on_Ig_slider_sliderMoved(int position);

    void on_Amax_slider_sliderMoved(int position);

    void on_Amin_slider_sliderMoved(int position);

    void on_Arest_slider_sliderMoved(int position);

    void openRecentFile();

    void on_experiment_editor_textChanged();

    void on_actionSave_triggered();

    void on_action_Save_as_triggered();

    void on_actionQuit_triggered();

    void on_export_activation_plot_clicked();

    void on_export_weights_plot_clicked();

    void on_MaxFreq_spinBox_valueChanged(int value);

    void activationsLegendDoubleClick(QCPLegend *legend,
                                      QCPAbstractLegendItem *item);

    void on_show_all_activations_plots_clicked();

    void on_hide_all_activations_plots_clicked();

    void autoupdateActivationsPlot();

private:
    void initializeWeightsPlot();
    void initializeActivationsPlot();

    /** Set up the plots size and captions.
     *
     * Typically called once an experiment is loaded,
     * but before the experiment ran.
     */
    void prepareActivationsPlot();
    void prepareWeightsPlot();

    void updateActivationsPlot();
    void updateWeightsPlot();

    /** Pass a legend item and toggle the visibility of the corresponding
     * graph.
     */
    void toggleGraph(QCPPlottableLegendItem* item, bool only_hide = false, bool only_show = false);

    void loadExperiment(const QString &filename);
    void setupExperiment(const Experiment &expe);

    void updateParameter(const std::string &name, double value);
    void update_expe_description_parameter(const QString &name, double value);

    void setCurrentFile(const QString &fileName);
    void updateRecentFileActions();
    QString strippedName(const QString &filename);

    enum { MaxRecentFiles = 5 };
    QAction *recentFileActs[MaxRecentFiles];

    Ui::MainWindow *ui;

    std::unique_ptr<MemoryNetwork> memory;

    Experiment expe;
    void saveFile(const QString &fileName);

    QString curFile;
    std::unique_ptr<SimpleMarkdownHighlighter> highlighter;
};

class SimpleMarkdownHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    SimpleMarkdownHighlighter(QTextDocument *parent = 0);

protected:
    void highlightBlock(const QString &text) Q_DECL_OVERRIDE;

private:
    struct HighlightingRule
    {
        QRegExp pattern;
        QTextCharFormat format;
    };
    QVector<HighlightingRule> highlightingRules;

};


#endif  // MAINWINDOW_H
