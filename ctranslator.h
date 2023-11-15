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
        DoParseSrc2Xlsx_E //解析源码，将解析出词条写入xlsx文件
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
    bool openXlsxFile(const QString &fileName);
    bool saveTsFile();
    bool saveXlsxFile(const QString &fileName);
    bool saveCsvFile(const QString &fileName);
    bool saveAsTsFile(const QString &fileName);
    void startParseSrc(const QString &path, int type);
    void translateItem(const QString &srcStr,const QString &dstStr);
    const QStringList &srcString() { return m_srcStrings; }
    const TSField_S &tsField() {return m_tsField; }
    QString tsFileName()const { return m_tsField.tsFileInfo.absoluteFilePath(); }
    void tsClear();
    void updateTsField();
    void updateXlsxField();
    void startReleaseQm();
    bool tsIsSaved()const { return m_tsIsSaved; }
    void setCode(const QString &code) { m_code = code; }

    void run();

signals:
    void evt_ProcessProgress(int processType,const QString &tips);

public slots:

private:
    void parseSrc(const QString &path);
    bool saveQmFile();
    void updateTsFieldSrcStrs();
    void updateXlsxFieldSrcStrs();
    static const quint32 S_TS_VERSION = 0x30303031;
    int m_doType;
    QStringList m_srcStrings; //源文
    TSField_S m_tsField;
    bool m_tsIsSaved; //Ts文件是否已保存标志
    QStringList m_languages; //语种，第一个时源语言
    QHash<QString,QStringList> m_xlsxPairs; //源语言(中文)，目标语言（英文，法文...),支持多各目标语言
    QHash<QString,QStringList> m_saveXlsxPairs; //保存在文件中的
    QString m_code; //编码格式
};

#endif // CTRANSLATOR_H
