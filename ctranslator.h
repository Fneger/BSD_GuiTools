#ifndef CTRANSLATOR_H
#define CTRANSLATOR_H

#include "mythread.h"
#include <QHash>
#include <QFileInfo>

class CTranslator : public CMyThread
{
    Q_OBJECT
public:
    //ts结构体
    typedef struct __TSField_S{
        QString srcPath;    //源码路径
        QFileInfo tsFileInfo;   //ts文件信息
        QHash<QString,QString> tsPairs; //词条对<英文词条, 目标语言词条>
        int tsUndoneNum;   //未完成项数
        int tsDoneNum;     //完成项数
    }TSField_S;
    //工作类型
    enum DoType{
        DoParseSrc_E,   //解析源码，并更新或生成ts文件
        DoReleaseQm_E,   //更新或生成qm文件
    };
    //处理过程
    enum ProcessType{
        ParseSrcProcessing_E,   //处理中
        ParseSrcProcessingDone_E,   //处理完成
        ReleaseQmProcessing_E,   //处理中
        ReleaseQmProcessingDone_E,   //处理完成
        ReleaseQmProcessingFailed_E,    //发布QM文件失败
    };
    explicit CTranslator(QObject *parent = nullptr);
    bool openTsFile(const QString &fileName);
    bool saveTsFile();
    bool saveAsTsFile(const QString &fileName);
    void startParseSrc(const QString &path);
    void translateItem(const QString &srcStr,const QString &dstStr);
    const QStringList &srcString() { return m_srcStrings; }
    const TSField_S &tsField() {return m_tsField; }
    QString tsFileName()const { return m_tsField.tsFileInfo.absoluteFilePath(); }
    void tsClear();
    void updateTsField();
    void startReleaseQm();
    bool tsIsSaved()const { return m_tsIsSaved; }

    void run();

signals:
    void evt_ProcessProgress(int processType,const QString &tips);

public slots:

private:
    void parseSrc(const QString &path);
    bool saveQmFile();
    void updateTsFieldSrcStrs();
    static const quint32 S_TS_VERSION = 0x30303031;
    int m_doType;
    QStringList m_srcStrings; //源文
    TSField_S m_tsField;
    bool m_tsIsSaved; //Ts文件是否已保存标志
};

#endif // CTRANSLATOR_H
