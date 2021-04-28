#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class QLabel;
class CTranslator;
class QSettings;
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void slot_menuFileTriggered(QAction*);
    void slot_ProcessProgress(int processType,const QString &tips);

    void on_TsSrcListWidget_currentRowChanged(int currentRow);

    void on_TsDstListWidget_currentRowChanged(int currentRow);

    void on_PrevBtn_clicked();

    void on_NextBtn_clicked();

    void on_prevunfinishedBtn_clicked();

    void on_nextunfinishedBtn_clicked();

    void on_doneandBtn_clicked();

    void on_doneandnextBtn_clicked();

    void on_TsFileColseBtn_clicked();

private:
    void closeEvent(QCloseEvent *);
    void updateTsList(const QHash<QString,QString> &tsPairsv); //���´�����ʾ�б�
    void updateTsEditArea();//���·�������༭��
    Ui::MainWindow *ui;
    CTranslator *m_translator;
    QLabel *m_statusLabel;
    QSettings *m_settings;
    bool m_isLoading;   //������ڼ�����
};
#endif // MAINWINDOW_H
