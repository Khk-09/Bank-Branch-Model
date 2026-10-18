#include "bankwidget.h"
#include <QPainter>
#include <QPaintEvent>
#include <algorithm>

BankWidget::BankWidget(QWidget *parent) : QWidget(parent) {
    setMinimumSize(640, 420);
    setAutoFillBackground(false);
}

void BankWidget::setSnapshot(const BankSnapshot &snap) {
    this->snap = snap;
    has_snap = true;
    update();
}

void BankWidget::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), QColor(244, 241, 234));

    if (!has_snap) {
        p.drawText(rect(), Qt::AlignCenter, "Нажмите «Запустить»");
        return;
    }

    const int pad = 16;
    QRect area = rect().adjusted(pad, pad, -pad, -pad);

    QRect head(area.left(), area.top(), area.width(), 36);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(55, 71, 79));
    p.drawRoundedRect(head, 6, 6);
    p.setPen(QColor(236, 239, 241));
    QFont f = p.font();
    f.setPixelSize(15);
    f.setBold(true);
    p.setFont(f);
    long long shown = snap.time;
    if (snap.done && shown >= SimDays * DayMs) {
        shown = SimDays * DayMs - 1000;
    }
    QString title = QString::fromStdString(Bank::clock_text(shown));
    if (snap.open) {
        title += "   открыто";
    } else {
        title += "   закрыто";
    }
    if (snap.done) {
        title += "   ·   месяц закончен";
    }
    p.drawText(head.adjusted(12, 0, -12, 0), Qt::AlignVCenter | Qt::AlignLeft, title);
    QRect board(area.left(), head.bottom() + 12, area.width(), 64);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(26, 35, 126));
    p.drawRoundedRect(board, 8, 8);
    p.setPen(QColor(255, 236, 179));
    f.setPixelSize(13);
    f.setBold(false);
    p.setFont(f);
    p.drawText(board.adjusted(14, 6, -14, 0), Qt::AlignLeft | Qt::AlignTop, "Информационное табло");
    f.setPixelSize(20);
    f.setBold(true);
    p.setFont(f);
    QString board_text = "—";
    if (snap.board_client > 0) {
        board_text = QString("Клиент %1    →    окно %2").arg(snap.board_client).arg(snap.board_window);
    }
    p.drawText(board.adjusted(14, 24, -14, -6), Qt::AlignLeft | Qt::AlignVCenter, board_text);

    int n = snap.clerks.size();
    int top = board.bottom() + 16;
    int counter_h = 132;
    int gap = 10;
    int avail = area.width();
    int cw = 120;
    if (n > 0) {
        cw = std::max(56, std::min(150, (avail - gap * (n - 1)) / n));
    }
    int row_w = 0;
    if (n > 0) {
        row_w = n * cw + (n - 1);
    }
    int x0 = area.left() + (avail - row_w) / 2;

    f.setBold(false);
    for (int i = 0; i < n; i++) {
        const ClerkView &c = snap.clerks[i];
        QRect box(x0 + i * (cw + gap), top, cw, counter_h);
        QColor body(141, 110, 99);
        QColor state(46, 125, 50);
        QString state_text = "свободно";
        QString who;
        if (c.phase == ClerkView::Serving) {
            state = QColor(21, 101, 192);
            state_text = "обслуживает";
            who = QString("№ %1").arg(c.client);
        } else if (c.phase == ClerkView::Lunch) {
            state = QColor(239, 108, 0);
            state_text = "обед";
        }
        p.setPen(Qt::NoPen);
        p.setBrush(body);
        p.drawRoundedRect(box, 8, 8);
        QRect glass = box.adjusted(8, 28, -8, -36);
        p.setBrush(state);
        p.drawRoundedRect(glass, 6, 6);
        p.setPen(Qt::white);
        f.setPixelSize(12);
        f.setBold(true);
        p.setFont(f);
        p.drawText(QRect(box.left(), box.top() + 4, box.width(), 22), Qt::AlignCenter, QString("Окно %1").arg(i + 1));
        f.setPixelSize(who.isEmpty() ? 13 : 18);
        p.setFont(f);
        p.drawText(glass.adjusted(4, 4, -4, -18), Qt::AlignCenter, who.isEmpty() ? " " : who);
        f.setPixelSize(12);
        f.setBold(false);
        p.setFont(f);
        p.drawText(glass.adjusted(4, glass.height() - 22, -4, -4), Qt::AlignCenter, state_text);
    }
    int qtop = top + counter_h + 18;
    p.setPen(QColor(55, 71, 79));
    f.setPixelSize(14);
    f.setBold(true);
    p.setFont(f);
    QString s = QString("Очередь  %1 / %2     выдано номеров сегодня: %3");
    s = s.arg(snap.queue.size());
    s = s.arg(snap.queue_limit);
    s = s.arg(snap.issued_today);
    p.drawText(QRect(area.left(), qtop, area.width(), 22), Qt::AlignLeft | Qt::AlignVCenter, s);
    int y = qtop + 28;
    if (snap.queue.empty()) {
        f.setBold(false);
        p.setFont(f);
        p.setPen(QColor(120, 120, 120));
        p.drawText(QRect(area.left(), y, area.width(), 24), Qt::AlignLeft, snap.open ? "в зале никого" : "зал пуст");
    } else {
        const int diam = 34;
        const int step = 40;
        int cols = std::max(1, area.width() / step);
        for (int i = 0; i < snap.queue.size(); i++) {
            int col = i % cols;
            int row = i / cols;
            QRect dot(area.left() + col * step, y + row * step, diam, diam);
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(92, 107, 192));
            p.drawEllipse(dot);
            p.setPen(Qt::white);
            f.setPixelSize(11);
            f.setBold(true);
            p.setFont(f);
            p.drawText(dot, Qt::AlignCenter, QString::number(snap.queue[i]));
        }
    }
}
