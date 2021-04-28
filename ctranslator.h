#ifndef CTRANSLATOR_H
#define CTRANSLATOR_H

#include "mythread.h"
#include <QHash>
#include <QFileInfo>

class CTranslator : public CMyThread
{
    Q_OBJECT
public:
    //ts�ṹ��
    typedef struct __TSField_S{
        QString srcPath;    //Դ��·��
        QFileInfo tsFileInfo;   //ts�ļ���Ϣ
        QHash<QString,QString> tsPairs; //������<Ӣ�Ĵ���, Ŀ�����Դ���>
        int tsUndoneNum;   //δ�������
        int tsDoneNum;     //�������
    }TSField_S;
    //��������
    enum DoType{
        DoParseSrc_E,   //����Դ�룬�����»�����ts�ļ�
        DoReleaseQm_E,   //���»�����qm�ļ�
    };
    //�������
    enum ProcessType{
        ParseSrcProcessing_E,   //������
        ParseSrcProcessingDone_E,   //�������
        ReleaseQmProcessing_E,   //������
        ReleaseQmProcessingDone_E,   //�������
        ReleaseQmProcessingFailed_E,    //����QM�ļ�ʧ��
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
    QStringList m_srcStrings; //Դ��
    TSField_S m_tsField;
    bool m_tsIsSaved; //Ts�ļ��Ƿ��ѱ����־
};

#endif // CTRANSLATOR_H
