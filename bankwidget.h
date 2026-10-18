#pragma once
#include "bank.h"
#include <QWidget>

class BankWidget : public QWidget {
    Q_OBJECT
public:
    explicit BankWidget(QWidget *parent = nullptr);
    void setSnapshot(const BankSnapshot &snap);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    BankSnapshot snap;
    bool has_snap = false;
};
