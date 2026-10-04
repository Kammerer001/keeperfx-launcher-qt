#pragma once

#include <QDialog>

namespace Ui { class LoadReplayDialog; }

class LoadReplayDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoadReplayDialog(QWidget *parent = nullptr);
    ~LoadReplayDialog();

    QString getReplayFileName();

private slots:
    void on_cancelButton_clicked();
    void on_startButton_clicked();
    void updateStartButton();

private:
    Ui::LoadReplayDialog *ui;

    QString replayFileName;
};
