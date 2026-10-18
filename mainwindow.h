#pragma once
#include "bank.h"
#include <QMainWindow>
#include <memory>

class BankWidget;
class QPlainTextEdit;
class QLabel;
class QSpinBox;
class QDoubleSpinBox;
class QComboBox;
class QTimer;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void startOver();
    void stepOnce();
    void togglePlay();
    void runToEnd();
    void lawChanged();

private:
    void buildUi();
    void refresh();
    void setPlaying(bool on);
    DistRange readRange(QComboBox *law, QDoubleSpinBox *min_v, QDoubleSpinBox *max_v, QDoubleSpinBox *mean, QDoubleSpinBox *sigma, bool *ok) const;
    std::unique_ptr<Bank> bank;
    BankWidget *view = nullptr;
    QPlainTextEdit *log_view = nullptr;
    QLabel *stats = nullptr;
    QSpinBox *clerks_spin = nullptr;
    QSpinBox *limit_spin = nullptr;
    QSpinBox *step_spin = nullptr;
    QSpinBox *seed_spin = nullptr;
    QComboBox *arr_law = nullptr;
    QDoubleSpinBox *arr_min = nullptr;
    QDoubleSpinBox *arr_max = nullptr;
    QDoubleSpinBox *arr_mean = nullptr;
    QDoubleSpinBox *arr_sigma = nullptr;
    QComboBox *srv_law = nullptr;
    QDoubleSpinBox *srv_min = nullptr;
    QDoubleSpinBox *srv_max = nullptr;
    QDoubleSpinBox *srv_mean = nullptr;
    QDoubleSpinBox *srv_sigma = nullptr;
    QSpinBox *profit_min = nullptr;
    QSpinBox *profit_max = nullptr;
    QTimer *play_timer = nullptr;
    class QPushButton *play_btn = nullptr;
};
