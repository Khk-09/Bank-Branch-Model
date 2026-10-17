#include "mainwindow.h"
#include <QtWidgets>
#include <chrono>
#include <cmath>

constexpr double MsPerMin = 60000.0;

QDoubleSpinBox *minuteSpin(double value, double max_v) {
    QDoubleSpinBox *s = new QDoubleSpinBox;
    s->setRange(0, max_v);
    s->setDecimals(2);
    s->setSingleStep(1);
    s->setSuffix(" мин");
    s->setValue(value);
    return s;
}

QWidget *rangeForm(QComboBox *&law, QDoubleSpinBox *&min_v, QDoubleSpinBox *&max_v,
                   QDoubleSpinBox *&mean, QDoubleSpinBox *&sigma,
                   double dmin, double dmax, double dmean, double dsigma) {
    QWidget *box = new QWidget;
    QFormLayout *form = new QFormLayout(box);
    form->setContentsMargins(0, 0, 0, 0);
    law = new QComboBox;
    law->addItem("Равномерное");
    law->addItem("Нормальное");
    min_v = minuteSpin(dmin, 180);
    max_v = minuteSpin(dmax, 180);
    mean = minuteSpin(dmean, 180);
    sigma = minuteSpin(dsigma, 60);
    mean->setSuffix("");
    sigma->setSuffix("");
    form->addRow("Закон", law);
    form->addRow("Минимум", min_v);
    form->addRow("Максимум", max_v);
    form->addRow("μ", mean);
    form->addRow("σ", sigma);
    return box;
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    buildUi();
    play_timer = new QTimer(this);
    play_timer->setInterval(40);
    connect(play_timer, &QTimer::timeout, this, &MainWindow::stepOnce);
    startOver();
}

MainWindow::~MainWindow() {}

void MainWindow::buildUi() {
    QSplitter *split = new QSplitter(Qt::Horizontal, this);
    QScrollArea *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setMinimumWidth(300);
    QWidget *side = new QWidget;
    QVBoxLayout *side_lay = new QVBoxLayout(side);
    QGroupBox *common = new QGroupBox("Параметры");
    QFormLayout *common_form = new QFormLayout(common);
    clerks_spin = new QSpinBox;
    clerks_spin->setRange(2, 7);
    clerks_spin->setValue(3);
    limit_spin = new QSpinBox;
    limit_spin->setRange(10, 25);
    limit_spin->setValue(15);
    step_spin = new QSpinBox;
    step_spin->setRange(10, 60);
    step_spin->setSuffix(" мин");
    step_spin->setValue(15);
    /*seed_spin = new QSpinBox;
    seed_spin->setRange(0, 1000000000);
    seed_spin->setValue(1);
    seed_spin->setReadOnly(true);
    seed_spin->setToolTip("Seed текущего прогона. Новый при каждом «Запустить сначала»");*/
    common_form->addRow("Клерки N", clerks_spin);
    common_form->addRow("Очередь K", limit_spin);
    common_form->addRow("Шаг", step_spin);
    //common_form->addRow("Seed", seed_spin);

    QGroupBox *arr = new QGroupBox("Интервал между клиентами");
    QVBoxLayout *arr_lay = new QVBoxLayout(arr);
    arr_lay->addWidget(rangeForm(arr_law, arr_min, arr_max, arr_mean, arr_sigma, 0, 10, 5, 2));

    QGroupBox *srv = new QGroupBox("Длительность обслуживания");
    QVBoxLayout *srv_lay = new QVBoxLayout(srv);
    srv_lay->addWidget(rangeForm(srv_law, srv_min, srv_max, srv_mean, srv_sigma, 2, 30, 16, 5));

    QGroupBox *money = new QGroupBox("Прибыль с клиента, ₽");
    QFormLayout *money_form = new QFormLayout(money);
    profit_min = new QSpinBox;
    profit_max = new QSpinBox;
    profit_min->setRange(0, 1000000);
    profit_max->setRange(0, 1000000);
    profit_min->setValue(50);
    profit_max->setValue(3000);
    money_form->addRow("От", profit_min);
    money_form->addRow("До", profit_max);

    QLabel *hint = new QLabel(
        "Плотность потока считается сама: к концу дня и к концу недели она выше, "
        "а при очереди длиннее 7 человек падает. В момент закрытия очередь распускается.");
    hint->setWordWrap(true);

    QPushButton *restart = new QPushButton("Запустить сначала");
    QPushButton *step = new QPushButton("Шаг");
    play_btn = new QPushButton("Пуск");
    QPushButton *till_end = new QPushButton("До конца");
    QHBoxLayout *buttons = new QHBoxLayout;
    buttons->addWidget(step);
    buttons->addWidget(play_btn);
    buttons->addWidget(till_end);

    side_lay->addWidget(common);
    side_lay->addWidget(arr);
    side_lay->addWidget(srv);
    side_lay->addWidget(money);
    side_lay->addWidget(hint);
    side_lay->addWidget(restart);
    side_lay->addLayout(buttons);
    side_lay->addStretch();
    scroll->setWidget(side);

    QWidget *right = new QWidget;
    QVBoxLayout *right_lay = new QVBoxLayout(right);
    stats = new QLabel;
    stats->setWordWrap(true);
    stats->setTextInteractionFlags(Qt::TextSelectableByMouse);
    log_view = new QPlainTextEdit;
    log_view->setReadOnly(true);
    log_view->setFont(QFont("Consolas", 9));
    log_view->setPlaceholderText("Журнал событий");
    right_lay->addWidget(stats);
    right_lay->addWidget(log_view, 1);
    right->setMinimumWidth(320);

    split->addWidget(scroll);
    split->addWidget(right);
    split->setStretchFactor(0, 0);
    split->setStretchFactor(1, 1);
    split->setStretchFactor(2, 0);
    split->setSizes({340, 760, 360});
    setCentralWidget(split);
    setWindowTitle("Филиал банка");
    resize(1280, 780);

    connect(restart, &QPushButton::clicked, this, &MainWindow::startOver);
    connect(step, &QPushButton::clicked, this, &MainWindow::stepOnce);
    connect(play_btn, &QPushButton::clicked, this, &MainWindow::togglePlay);
    connect(till_end, &QPushButton::clicked, this, &MainWindow::runToEnd);
    connect(arr_law, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::lawChanged);
    connect(srv_law, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::lawChanged);
    lawChanged();
}

void MainWindow::lawChanged() {
    bool arr_normal = arr_law->currentIndex() == 1;
    arr_mean->setEnabled(arr_normal);
    arr_sigma->setEnabled(arr_normal);
    bool srv_normal = srv_law->currentIndex() == 1;
    srv_mean->setEnabled(srv_normal);
    srv_sigma->setEnabled(srv_normal);
}

DistRange MainWindow::readRange(QComboBox *law, QDoubleSpinBox *min_v, QDoubleSpinBox *max_v,
                                QDoubleSpinBox *mean, QDoubleSpinBox *sigma, bool *ok) const {
    DistRange r;
    r.normal = law->currentIndex() == 1;
    r.min_ms = std::llround(min_v->value() * MsPerMin);
    r.max_ms = std::llround(max_v->value() * MsPerMin);
    r.mean_ms = mean->value() * MsPerMin;
    r.sigma_ms = sigma->value() * MsPerMin;
    if (r.min_ms > r.max_ms || r.max_ms <= 0) {
        *ok = false;
    }
    if (r.normal && r.sigma_ms <= 0) {
        *ok = false;
    }
    return r;
}

void MainWindow::startOver() {
    setPlaying(false);
    bool ok = true;
    BankParams p;
    p.clerks = clerks_spin->value();
    p.limit = limit_spin->value();
    p.arrival = readRange(arr_law, arr_min, arr_max, arr_mean, arr_sigma, &ok);
    p.service = readRange(srv_law, srv_min, srv_max, srv_mean, srv_sigma, &ok);
    p.profit_min = profit_min->value();
    p.profit_max = profit_max->value();
    if (p.profit_min > p.profit_max) ok = false;
    if (!ok) {
        QMessageBox::warning(this, "Параметры", "Минимум не больше максимума, максимум интервала больше нуля, ""а у нормального закона σ > 0.");
        return;
    }
    static long long salt = 0;
    long long seed = (std::chrono::steady_clock::now().time_since_epoch().count());
    seed ^= ++salt * 0x9E3779B9u;
    if (seed == 0) {
        seed = 1;
    }
    seed %= 1000000000u;
    if (seed == 0) {
        seed = 1;
    }
    //seed_spin->setValue(int(seed));
    p.seed = seed;
    bank = std::make_unique<Bank>(p);
    bank->advance_step(MinuteMs);
}

void MainWindow::stepOnce() {
    if (!bank) {
        return;
    }
    if (bank->finished()) {
        setPlaying(false);
        return;
    }
    bank->advance_step(step_spin->value() * MinuteMs);
    if (bank->finished()) {
        setPlaying(false);
    }
}

void MainWindow::togglePlay() {
    if (!bank) {
        return;
    }
    setPlaying(!play_timer->isActive());
}

void MainWindow::setPlaying(bool on) {
    if (on) {
        play_timer->start();
    } else {
        play_timer->stop();
    }
    play_btn->setText(play_timer->isActive() ? "Пауза" : "Пуск");
}

void MainWindow::runToEnd() {
    if (!bank) {
        return;
    }
    setPlaying(false);
    QApplication::setOverrideCursor(Qt::WaitCursor);
    bank->advance_to(bank->end_time());
    QApplication::restoreOverrideCursor();
}
