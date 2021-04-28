#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "ctranslator.h"
#include <QFileDialog>
#include <QFileInfo>
#include <QListWidgetItem>
#include <QDebug>
#include <QFile>
#include <QKeyEvent>
#include <QMessageBox>
#include <QSettings>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    m_isLoading = true;
    connect(ui->menuFile,&QMenu::triggered,this,&MainWindow::slot_menuFileTriggered);
    m_translator = new CTranslator();
    connect(m_translator,&CTranslator::evt_ProcessProgress,
            this,&MainWindow::slot_ProcessProgress,Qt::QueuedConnection);
    QString trTest0(tr("Test0  Abc def" ) );
    QString trTest1(tr ("Test1  Abc def" ));
    QString trTest2(tr( "Test2  Abc def "));
    QString trTest3(tr ( "Test3  Abc def "));
    QString trTest4(tr(" Test4  Abc def "));
    QString trTest5 = "tr(\" Test5  Abc def \" 12334444";
    QString str = tr("tr(\" Test6  Abc def \") 12334444") + "Test" + QString("") + tr("8888888");
    str = "dxxxdxxxd";
    QRegExp exp("((d)(/w+?)(d)");
    //QRegExp exp("(?:()(.*)(?:))"); //里面那两个 > < 可以替换成其他，但是要注意转义字符
    //QRegExp exp("(?:\\[)(.*)(?:\\])");

    qDebug() << "index:" << exp.indexIn(str);
    if(str.indexOf(exp) >= 0)
    {
        qDebug() << "something cap:" << exp.matchedLength();
        qDebug() << exp.cap(0);
    }
    m_statusLabel = new QLabel(this);
    ui->statusbar->addWidget(m_statusLabel);
    m_settings = new QSettings("./AppSettings.ini", QSettings::IniFormat);
    m_isLoading = false;
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *pEv)
{
    if(!m_translator->tsIsSaved())
    {
        QMessageBox msgBox;
        msgBox.setText("The translation file has been modified.");
        msgBox.setInformativeText("Do you want to save your changes?");
        msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::Save);
        int ret = msgBox.exec();
        switch (ret) {
        case QMessageBox::Save:
            slot_menuFileTriggered(ui->actionSave);
            break;
        case QMessageBox::Discard:
            // Don't Save was clicked
            break;
        case QMessageBox::Cancel:
            pEv->ignore();
            return;
            break;
        default:
            // should never be reached
            break;
        }
    }
    QMainWindow::closeEvent(pEv);
}

void MainWindow::slot_menuFileTriggered(QAction* pAct)
{
    if(pAct == ui->actionUpdate_ts) //更新ts文件
    {
        if(m_translator->tsField().srcPath.length() <= 0)
        {
            QString path = QFileDialog::getExistingDirectory(this,tr("Source Code Path"),m_settings->value("TsSettings/SrcPath",".").toString());
            if(path.length() > 0)
            {
                m_settings->setValue("TsSettings/SrcPath",path);
                m_translator->startParseSrc(path);
            }
        }
        else
        {
            m_translator->updateTsField();
        }
    }
    else if(pAct == ui->actionTranslation_File_ts) //打开ts文件
    {
        QString fileName = QFileDialog::getOpenFileName(this,tr("Open ts file"),m_settings->value("TsSettings/TsFileOpenPath",".").toString(),"*.ts");
        if(fileName.length() > 0)
        {
            if(m_translator->openTsFile(fileName))
            {
                QFileInfo fileInfo(fileName);
                m_settings->setValue("TsSettings/TsFileOpenPath",fileInfo.absolutePath());
                ui->TranslationFileLabel->setText(fileName);
                updateTsList(m_translator->tsField().tsPairs);
            }
        }
    }
    else if(pAct == ui->actionSave) //保存ts文件
    {
        bool res =false;
        if(m_translator->tsFileName().length() <= 0)
        {
            QString fileName = QFileDialog::getSaveFileName(this,tr("Save ts file"),m_settings->value("TsSettings/TsFileSavePath",".").toString(),"*.ts");
            if(fileName.length() >= 0)
            {
                QFileInfo fileInfo(fileName);
                m_settings->setValue("TsSettings/TsFileSavePath",fileInfo.absolutePath());
                res = m_translator->saveAsTsFile(fileName);
            }
        }
        else
            res = m_translator->saveTsFile();
        if(res)
            ui->TranslationFileLabel->setText(m_translator->tsFileName());
    }
    else if(pAct == ui->actionSave_As) //另存为ts文件
    {
        bool res = false;
        QString fileName = QFileDialog::getSaveFileName(this,tr("Save as ts file"),m_settings->value("TsSettings/TsFileSaveAsPath",".").toString(),"*.ts");
        if(fileName.length() >= 0)
        {
            QFileInfo fileInfo(fileName);
            m_settings->setValue("TsSettings/TsFileSaveAsPath",fileInfo.absolutePath());
            res = m_translator->saveAsTsFile(fileName);
        }
        if(res)
        ui->TranslationFileLabel->setText(m_translator->tsFileName());
    }
    else if(pAct == ui->actionRelease_qm)
    {
        m_translator->startReleaseQm();
    }
}

void MainWindow::slot_ProcessProgress(int processType,const QString &tips)
{
    switch (processType) {
    case CTranslator::ReleaseQmProcessing_E:
    case CTranslator::ReleaseQmProcessingFailed_E:
    case CTranslator::ParseSrcProcessing_E:
        m_statusLabel->setText(tips);
        break;
    case CTranslator::ParseSrcProcessingDone_E:
        m_statusLabel->setText(tips);
        updateTsList(m_translator->tsField().tsPairs);
        break;
    case CTranslator::ReleaseQmProcessingDone_E:
        m_statusLabel->setText(tips);
        break;
    }
}

void MainWindow::updateTsList(const QHash<QString,QString> &tsPairsv)
{
    m_isLoading = true;
    ui->TsSrcListWidget->clear();
    ui->TsDstListWidget->clear();
    QHash<QString, QString>::const_iterator it;
    QListWidgetItem *item = nullptr;
    QString dstStr;
    for( it=tsPairsv.begin(); it!=tsPairsv.end(); ++it)
    {
        item = new QListWidgetItem(it.key());
        item->setIcon(QIcon(":/images/linguist/win/phrase.png"));
        ui->TsSrcListWidget->addItem(item);
    }
    ui->TsSrcListWidget->sortItems(Qt::AscendingOrder);
    int count = ui->TsSrcListWidget->count();
    QListWidget *pWidgets = ui->TsSrcListWidget;
    for(int i=0; i<count; i++)
    {
        dstStr = tsPairsv.value(pWidgets->item(i)->text());
        item = new QListWidgetItem(dstStr);
        if(dstStr.length() <= 0)
            item->setIcon(QIcon(":/images/linguist/s_check_off.png"));
        else
            item->setIcon(QIcon(":/images/linguist/s_check_on.png"));
        ui->TsDstListWidget->addItem(item);
    }
    m_isLoading = false;
}

void MainWindow::updateTsEditArea()
{
    ui->TsSrcStringLabel->setText(ui->TsSrcListWidget->currentItem()->text());
    ui->TsDstStrEdit->setText(ui->TsDstListWidget->currentItem()->text());
}

void MainWindow::on_TsSrcListWidget_currentRowChanged(int currentRow)
{
    if(m_isLoading)
        return;
    ui->TsDstListWidget->setCurrentRow(currentRow);
    updateTsEditArea();
}

void MainWindow::on_TsDstListWidget_currentRowChanged(int currentRow)
{
    if(m_isLoading)
        return;
    ui->TsSrcListWidget->setCurrentRow(currentRow);
    updateTsEditArea();
}

void MainWindow::on_PrevBtn_clicked()
{
    QListWidget *pWidgets = ui->TsSrcListWidget;
    if(pWidgets->currentRow() > 0)
    {
        pWidgets->setCurrentRow(pWidgets->currentRow()-1);
    }
}

void MainWindow::on_NextBtn_clicked()
{
    QListWidget *pWidgets = ui->TsSrcListWidget;
    if(pWidgets->currentRow() < pWidgets->count() && pWidgets->count() > 0)
    {
        pWidgets->setCurrentRow(pWidgets->currentRow()+1);
    }
}

void MainWindow::on_prevunfinishedBtn_clicked()
{
    QListWidget *pWidgets = ui->TsDstListWidget;
    int index = pWidgets->currentRow();
    if(pWidgets->count() > 0 && index > 0)
    {
        for(int i=index-1; i>=0; i--)
        {
            if(pWidgets->item(i)->text().length() <= 0)
            {
                pWidgets->setCurrentRow(i);
                break;
            }
        }
    }
}

void MainWindow::on_nextunfinishedBtn_clicked()
{
    QListWidget *pWidgets = ui->TsDstListWidget;
    int index = pWidgets->currentRow();
    if(pWidgets->count() > 0 && index < pWidgets->count())
    {
        for(int i=index+1; i<pWidgets->count(); i++)
        {
            if(pWidgets->item(i)->text().length() <= 0)
            {
                pWidgets->setCurrentRow(i);
                break;
            }
        }
    }
}

void MainWindow::on_doneandBtn_clicked()
{
    if(ui->TsSrcListWidget->count() <= 0)
        return;
    QString dstStr = ui->TsDstStrEdit->toPlainText();
    QListWidgetItem *item = ui->TsDstListWidget->currentItem();
    if(dstStr.length() <= 0)
        item->setText(ui->TsSrcListWidget->currentItem()->text());
    else
        item->setText(dstStr);
    item->setIcon(QIcon(":/images/linguist/s_check_on.png"));
    m_translator->translateItem(ui->TsSrcStringLabel->text(),item->text());
}

void MainWindow::on_doneandnextBtn_clicked()
{
    if(ui->TsSrcListWidget->count() <= 0)
        return;
    on_doneandBtn_clicked();
    on_nextunfinishedBtn_clicked();
}

void MainWindow::on_TsFileColseBtn_clicked()
{
    m_isLoading = true;
    ui->TsSrcListWidget->clear();
    ui->TsDstListWidget->clear();
    m_translator->tsClear();
    m_isLoading = false;
}
