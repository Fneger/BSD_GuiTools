#include "ctranslator.h"
#include <QFile>
#include <QDir>
#include <QDebug>
#include <QRegExp>
#include <QDataStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextCodec>

#define TS_VERSION 0x30303031
CTranslator::CTranslator(QObject *parent)
    : CMyThread(parent)
    , m_doType(DoParseSrc_E)
    , m_tsIsSaved(true)
{

}

bool CTranslator::openTsFile(const QString &fileName)
{
    bool res = false;
    QFile file(fileName);
    if(!file.exists())
        return res;
    if(file.open(QIODevice::ReadOnly))
    {
        QDataStream in(&file);
        in.setVersion(QDataStream::Qt_5_9); //设置版本号，写入和读取的版本号要兼容
        quint32 tsVersion;
        in >> tsVersion;
        if(tsVersion != S_TS_VERSION)
            goto CLOSE;
        in >> m_tsField.srcPath;
        m_tsField.tsFileInfo = QFileInfo(file);
        in >> m_tsField.tsPairs;
        in >> m_tsField.tsUndoneNum;
        in >> m_tsField.tsDoneNum;
        res = true;
        m_tsIsSaved = true;
        CLOSE:
        file.close();
    }
    return res;
}

bool CTranslator::saveTsFile()
{
    bool res = false;
    QFile file(m_tsField.tsFileInfo.absoluteFilePath());
    if(file.open(QIODevice::WriteOnly))
    {
        QDataStream out(&file);
        out.setVersion(QDataStream::Qt_5_9);
        out << S_TS_VERSION
            << m_tsField.srcPath
            << m_tsField.tsPairs
            << m_tsField.tsUndoneNum
            << m_tsField.tsDoneNum;
        res = true;
        file.close();
        m_tsIsSaved = true;
    }
    return res;
}

bool CTranslator::saveAsTsFile(const QString &fileName)
{
    bool res = false;
    QFile file(fileName);
    if(file.open(QIODevice::WriteOnly))
    {
        QDataStream out(&file);
        out.setVersion(QDataStream::Qt_5_9);
        m_tsField.tsFileInfo = QFileInfo(fileName);
        out << S_TS_VERSION
            << m_tsField.srcPath
            << m_tsField.tsPairs
            << m_tsField.tsUndoneNum
            << m_tsField.tsDoneNum;
        res = true;
        file.close();
    }
    return res;
}

void CTranslator::startParseSrc(const QString &path)
{
    m_tsIsSaved = false;
    m_doType = DoParseSrc_E;
    m_tsField.srcPath = path;
    startRun();
}

void CTranslator::updateTsField()
{
    startParseSrc(m_tsField.srcPath);
}

void CTranslator::parseSrc(const QString &path)
{
    //设置要遍历的目录
    QDir dir(path);
    //设置文件过滤器
    QStringList nameFilters;
    //设置文件过滤格式
    nameFilters << "*.cpp";
    //将过滤后的文件名称存入到files列表中
    QFileInfoList infoList = dir.entryInfoList(nameFilters, QDir::Files|QDir::Readable|QDir::AllDirs, QDir::Name);
    QFileInfo info;
    bool bIsDir;
    foreach(info,infoList)
    {
        qDebug() << info;
        if(info.fileName() == "." || info.fileName() == "..")
            continue;
        bIsDir = info.isDir();
        if(bIsDir)
        {
            parseSrc(info.filePath());
        }
        else
        {
            emit evt_ProcessProgress(ParseSrcProcessing_E,"Parsing source file......"+info.absoluteFilePath());
            QFile file(info.absoluteFilePath());
            if(file.open(QIODevice::Text|QIODevice::ReadOnly))
            {
                QString str;
                QStringList list;
                QStringList dstList;
                int pos = 0;
                QTextCodec *codec = QTextCodec::codecForName("GBK");
                while(!file.atEnd()){
                    list.clear();
                    pos = 0;
                    QString str = codec->toUnicode(file.readLine());

                    //匹配 tr("-----------")
                    QRegExp rx("(?:tr\\s*\\(\\s*\")(.*)(?:\"\\s*\\))");
                    rx.setMinimal(true);
                    while ((pos = rx.indexIn(str, pos,QRegExp::CaretAtOffset)) != -1) {
                        pos += rx.matchedLength();
                        list << rx.cap(0);
                    }
                    //得到tr()括号中字符串
                    foreach (str, list) {
                        str = str.replace(QRegExp("^tr\\s*\\(\\s*\""),"");
                        str = str.replace(QRegExp("\"\\s*\\)$"),"");
                        qDebug() << str;
                        m_srcStrings << str;
                    }
                }
                file.close();
            }
        }

    }
}

void CTranslator::updateTsFieldSrcStrs()
{
    QString srcStr;
    QString tmpStr;
    QHash<QString,QString> tmpTsPairs;
    QHash<QString,QString> &tsPairs = m_tsField.tsPairs;
    foreach (srcStr, m_srcStrings) {
        srcStr = QString::fromLocal8Bit(srcStr.toLocal8Bit());
        if(tsPairs.contains(srcStr))
        {
            tmpTsPairs[srcStr] = tsPairs[srcStr];
        }
        else
            tmpTsPairs[srcStr] = "";
    }
    m_tsField.tsPairs = tmpTsPairs;
}

void CTranslator::translateItem(const QString &srcStr,const QString &dstStr)
{
    if(!srcStr.isEmpty())
    {
        m_tsField.tsPairs[srcStr] = dstStr;
        m_tsIsSaved = false;
    }
}

void CTranslator::tsClear()
{
    m_tsIsSaved = false;
    m_tsField.srcPath.clear();
    m_tsField.tsFileInfo = QFileInfo();
    m_tsField.tsPairs.clear();
}

bool CTranslator::saveQmFile()
{
    bool res =false;

    if(m_tsField.tsFileInfo.fileName().length() > 0)
    {
        QString fileName = m_tsField.tsFileInfo.absoluteFilePath().replace(".ts",".qm");
        QFile file(fileName);
        if(file.open(QIODevice::WriteOnly|QIODevice::Truncate))
        {
            QJsonDocument doc;
            QHash<QString, QString>::const_iterator it;
            QString dstStr;
            QJsonObject jsObj;
            for( it=m_tsField.tsPairs.begin(); it!=m_tsField.tsPairs.end(); ++it)
            {
                jsObj[it.key()] = QString(it.value());
            }
            doc.setObject(jsObj);
            file.write(QString(doc.toJson(QJsonDocument::Indented)).toLocal8Bit()); //Indented:表示自动添加/n回车符
            file.close();
            res = true;
        }
    }
    return res;
}

void CTranslator::startReleaseQm()
{
    m_doType = DoReleaseQm_E;
    startRun();
}

void CTranslator::run()
{
    switch (m_doType) {
    case DoParseSrc_E:
        m_srcStrings.clear();
        parseSrc(m_tsField.srcPath);
        updateTsFieldSrcStrs();
        emit evt_ProcessProgress(ParseSrcProcessingDone_E,QString("Parsing source file......Done,%1").arg(m_tsField.tsFileInfo.absoluteFilePath()));
        break;
    case DoReleaseQm_E:
    {
        bool res = false;
        res = saveQmFile();
        if(res)
            emit evt_ProcessProgress(ParseSrcProcessingDone_E, QString("Release *.qm file......Done,%1").arg(m_tsField.tsFileInfo.absoluteFilePath().replace(".ts",".qm")));
        else
            emit evt_ProcessProgress(ReleaseQmProcessingFailed_E,"Release *.qm file......Failed");
    }
        break;
    }
}
