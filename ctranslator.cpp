#include "ctranslator.h"
#include <QFile>
#include <QDir>
#include <QDebug>
#include <QRegExp>
#include <QDataStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextCodec>

#include "xlsxdocument.h"
#include "xlsxchartsheet.h"
#include "xlsxcellrange.h"
#include "xlsxchart.h"
#include "xlsxrichstring.h"
#include "xlsxworkbook.h"

#define TS_VERSION 0x30303031
using namespace QXlsx;
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

bool CTranslator::openXlsxFile(const QString &fileName)
{
    bool res = false;
    Document xlsxR(fileName);
    if(!xlsxR.load()) {
        return res;
    }

    m_tsField.tsFileInfo = QFileInfo(fileName);
    m_xlsxPairs.clear();

    m_languages.clear();
    //查找时列表头信息，不为空的则认为存在目标语言
    for(int column = 1; column <= 50; ++column) {
        Cell* cell = xlsxR.cellAt(1, column);
        if(cell == nullptr || cell->value().isNull() || cell->value().toString().isEmpty())
            break;
        m_languages << cell->readValue().toString();
    }
    qDebug() << "languages" << m_languages;
    if(m_languages.isEmpty())
        return false;


    //读取已经翻译词条对
    int row = 1;
    bool isEnd = false;
    while (1) {
        row++;
        QStringList pairs;
        for(int column = 1; column <= m_languages.size(); ++column) {
            Cell* cell = xlsxR.cellAt(row, column);
            if(column == 1 && (cell == nullptr || cell->value().isNull() || cell->value().toString().isEmpty())) {
                isEnd = true;
                break;
            }

            if(cell == nullptr)
                pairs << "";
            else
                pairs << cell->value().toString();
        }
        if(isEnd)
            break;
        qDebug() << "read xlsx" << pairs;
        m_xlsxPairs.insert(pairs.first(), pairs);
    }
    if(m_languages.isEmpty())
        m_languages << QString::fromLocal8Bit("chinese_sim");
    return true;
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

// 自定义比较函数，按文件名称排序
static bool compareFiles(const QString &file1, const QString &file2) {
    return file1.toLower() < file2.toLower(); // 不区分大小写进行排序
}

bool CTranslator::saveXlsxFile(const QString &fileName)
{
    if(m_languages.size() <= 0)
        m_languages << QString::fromLocal8Bit("chinese_sim");
    Document xlsxW;
    QStringList keys = m_saveXlsxPairs.keys();
    //写表头信息
    for(int column = 1; column <= m_languages.size(); ++column) {
        xlsxW.write(1, column, m_languages.at(column - 1));
    }
    std::sort(keys.begin(), keys.end(), compareFiles);
    int row = 1;
    foreach (auto key, keys) {
        QStringList &pair = m_saveXlsxPairs[key];
        row++;
        qDebug() << "write xlsx" << pair;
        for(int column = 1; column <= m_languages.size(); ++column) {
            xlsxW.write(row, column, pair.at(column - 1));
        }
    }
    return xlsxW.saveAs(fileName);
}

bool CTranslator::saveCsvFile(const QString &fileName)
{
    if(m_languages.size() <= 0)
        m_languages << QString::fromLocal8Bit("chinese_sim");
    QFile file(fileName);
    if(file.open(QIODevice::WriteOnly)) {
        QTextCodec *codec = QTextCodec::codecForName(m_code.toLocal8Bit().data());
        QTextStream out(&file);
        out.setCodec(codec);
        out << m_languages.join("|") << "\n";

        QStringList keys = m_saveXlsxPairs.keys();
        std::sort(keys.begin(), keys.end(), compareFiles);
        foreach (auto key, keys) {
            QStringList &pair = m_saveXlsxPairs[key];
            out << pair.join("|") << "\n";
        }
        file.close();
        return true;
    }
    return false;
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

void CTranslator::startParseSrc(const QString &path, int type)
{
    m_tsIsSaved = false;
    m_doType = type;
    m_tsField.srcPath = path;
    startRun();
}

void CTranslator::updateTsField()
{
    startParseSrc(m_tsField.srcPath, DoParseSrc_E);
}

void CTranslator::updateXlsxField()
{
    startParseSrc(m_tsField.srcPath, DoParseSrc2Xlsx_E);
}

void CTranslator::parseSrc(const QString &path)
{
    //设置要遍历的目录
    QDir dir(path);
    //设置文件过滤器
    QStringList nameFilters;
    //设置文件过滤格式
    nameFilters << "*.cpp" << "*.qml";
    //将过滤后的文件名称存入到files列表中
    QFileInfoList infoList = dir.entryInfoList(nameFilters, QDir::Files|QDir::Readable|QDir::AllDirs, QDir::Name);
    QFileInfo info;
    bool bIsDir;
    foreach(info,infoList)
    {
        //qDebug() << info;
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
                QTextCodec *codec = QTextCodec::codecForName(m_code.toLocal8Bit().data());
                int lineCnt = 0;
                while(!file.atEnd()){
                    list.clear();
                    pos = 0;
                    lineCnt++;
                    QString str = codec->toUnicode(file.readLine());

                    //匹配 tr("-----------")
                    QRegExp rx("(?:tr\\s*\\(\\s*\")(.*)(?:\"\\s*\\))");
                    rx.setMinimal(true);
                    while ((pos = rx.indexIn(str, pos,QRegExp::CaretAtOffset)) != -1) {
                        pos += rx.matchedLength();
                        list << rx.cap(0);
                    }

                    //匹配 qsTr("-----------")
                    pos = 0;
                    QRegExp rx1("(?:qsTr\\s*\\(\\s*\")(.*)(?:\"\\s*\\))");
                    rx1.setMinimal(true);
                    while ((pos = rx1.indexIn(str, pos,QRegExp::CaretAtOffset)) != -1) {
                        pos += rx1.matchedLength();
                        list << rx1.cap(0);
                    }

                    //得到tr()括号中字符串
                    foreach (str, list) {
                        str = str.replace(QRegExp("^qsTr\\s*\\(\\s*\""),"");
                        str = str.replace(QRegExp("^tr\\s*\\(\\s*\""),"");
                        str = str.replace(QRegExp("\"\\s*\\)$"),"");
                        //qDebug() << str;
                        if(str.isEmpty())
                            continue;
                        m_srcStrings << str;
                    }

#if 0
                    QRegExp regexp("[\\x4e00-\\x9fff]+"); // 匹配中文字符的正则表达式
                    QStringList cns;
                    pos = 0;
                    while ((pos = regexp.indexIn(str, pos)) != -1) {
                        //qDebug() << "line:" << lineCnt << ",text:" << regexp.cap(0);
                        pos += regexp.matchedLength();
                        cns << regexp.cap(0);
                    }
                    if(!cns.isEmpty() && list.isEmpty()) {
                        if(!str.contains("//") && !str.contains("///")  && !str.contains("////")
                                && !str.contains("/*") && !str.contains("*/")) {
                            qDebug().noquote() << info.absoluteFilePath();
                            qDebug().noquote() << lineCnt << str;
                        }
                    }
#endif
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

void CTranslator::updateXlsxFieldSrcStrs()
{
    if(m_srcStrings.isEmpty())
        return;
    m_saveXlsxPairs = m_xlsxPairs;
    QHash<QString,QStringList> xlsxPairs = m_xlsxPairs;
    QHashIterator<QString,QStringList> it(xlsxPairs);

    while (it.hasNext()) {
        it.next();
        if(!m_srcStrings.contains(it.key())) //搜索文件中没有，xslx中有的，删除掉
            m_saveXlsxPairs.remove(it.key());
    }
    foreach (auto srcStr, m_srcStrings) {
        srcStr = QString::fromLocal8Bit(srcStr.toLocal8Bit());
        if(m_saveXlsxPairs.contains(srcStr))
            continue;
        else {
            QStringList strs;
            strs << srcStr;
            for(int i = 1; i < m_languages.size(); ++i) {
                strs << "";
            }
            m_saveXlsxPairs.insert(srcStr, strs);
        }
    }
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
    case DoParseSrc2Xlsx_E:
        m_srcStrings.clear();
        parseSrc(m_tsField.srcPath);
        updateXlsxFieldSrcStrs();
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
