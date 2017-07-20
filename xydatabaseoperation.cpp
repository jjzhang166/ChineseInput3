﻿#include "xydatabaseoperation.h"
#include "xytranslateitem.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QFile>
#include <QVariant>

XYDatabaseOperation *XYDatabaseOperation::DB = NULL;
XYDatabaseOperation *XYDatabaseOperation::getInstance()
{
    if (DB == NULL)
    {
        DB = new XYDatabaseOperation;
    }
    return DB;
}

XYDatabaseOperation::XYDatabaseOperation(QObject *parent)
    : QObject(parent)
{

}

XYDatabaseOperation::~XYDatabaseOperation()
{

}

bool XYDatabaseOperation::createDatabaseFile(const QString &filePath, const QString &passwd, bool fource)
{
    if (QFile::exists(filePath)) // 文件存在
    {
        if (fource)
        {
            if (!QFile::remove(filePath)) // 删除失败也返回错误
            {
                return false;
            }
        }
        else    // 不强制创建返回错误
        {
            return false;
        }
    }

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "XYInout");
    db.setDatabaseName(filePath);
    db.setPassword(passwd);
    db.setConnectOptions("QSQLITE_CREATE_KEY");
    return db.open();
}

bool XYDatabaseOperation::openDatabaseFile(const QString &filePath, const QString &passwd)
{
    if (QSqlDatabase::database("XYInout").isOpen())
    {
        return true;
    }
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "XYInout");
    db.setDatabaseName(filePath);
    db.setPassword(passwd);
    return db.open();
}

bool XYDatabaseOperation::createInputTable()
{
    QSqlQuery query(QSqlDatabase::database("XYInout"));

    // 单字表
    bool ok = query.exec("CREATE TABLE IF NOT EXISTS  singlePingying ("
                         "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                         "pingying VARCHAR NOT NULL,"
                         "chinese VARCHAR NOT NULL,"
                         "extra VARCHAR NULL,"
                         "times INTEGER NOT NULL,"
                         "stick BOOL NULL);");
    if (!ok)
    {
        qDebug("error: %s", query.lastError().text().toUtf8().data());
        return false;
    }

    // 基础表
    for (int i = 'A'; i <= 'Z'; ++i)
    {
        ok = query.exec(QString("CREATE TABLE IF NOT EXISTS  basePintying_%1 ("
                        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                        "pingying VARCHAR NOT NULL,"
                        "chinese VARCHAR NOT NULL,"
                        "extra VARCHAR NULL,"
                        "times INTEGER NOT NULL,"
                        "stick BOOL NULL);").arg(QChar(i)));
        if (!ok)
        {
            qDebug("error: %s", query.lastError().text().toUtf8().data());
            return false;
        }
    }

    // 用户表
    ok = query.exec("CREATE TABLE IF NOT EXISTS  userPingying ("
                    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                    "pingying VARCHAR NOT NULL,"
                    "chinese VARCHAR NOT NULL,"
                    "extra VARCHAR NULL,"
                    "times INTEGER NOT NULL,"
                    "stick BOOL NULL);");
    if (!ok)
    {
        qDebug("error: %s", query.lastError().text().toUtf8().data());
        return false;
    }

    // 英文表
    ok = query.exec("CREATE TABLE IF NOT EXISTS  englishTable ("
                    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                    "english VARCHAR NOT NULL,"
                    "translate VARCHAR NOT NULL,"
                    "extra VARCHAR NULL,"
                    "times INTEGER NOT NULL,"
                    "stick BOOL NULL);");
    if (!ok)
    {
        qDebug("error: %s", query.lastError().text().toUtf8().data());
        return false;
    }

    // 英文用户表
    ok = query.exec("CREATE TABLE IF NOT EXISTS  userEnglishTable ("
                    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                    "english VARCHAR NOT NULL,"
                    "translate VARCHAR NOT NULL,"
                    "extra VARCHAR NULL,"
                    "times INTEGER NOT NULL,"
                    "stick BOOL NULL);");
    if (!ok)
    {
        qDebug("error: %s", query.lastError().text().toUtf8().data());
        return false;
    }
    return ok;
}

bool XYDatabaseOperation::insertData(XYTranslateItem *item, const QString &table)
{
    QString field1, field2;
    if (table.toLower().contains("english"))
    {
        field1 = "english";
        field2 = "translate";
    }
    else
    {
        field1 = "pingying";
        field2 = "chinese";
    }

    QSqlQuery query(QSqlDatabase::database("XYInout"));
    bool ok = true;

    // 先查找词组是否存在
    query.prepare(QString("SELECT id, %1, %2, extra, times, stick FROM %3 "
                          "WHERE %4 like \"%5\" AND %6 like \"%7\";")
                  .arg(field1)
                  .arg(field2)
                  .arg(table)
                  .arg(field1)
                  .arg(item->msComplete)
                  .arg(field2)
                  .arg(item->msTranslate));
    ok = query.exec();
    if (!ok)
    {
        qDebug("error: %s", query.lastError().text().toUtf8().data());
    }

    int id = -1;
    int times = 1;
    while (query.next())
    {
        id = query.value(0).toInt();
        times = query.value(4).toInt();
        break;
    }

    if (id == -1)
    {
        query.prepare(QString("INSERT INTO %1 (id, %2, %3, extra, times, stick) "
                              "VALUES (:id, :%4, :%5, :extra, :times, :stick);")
                      .arg(table)
                      .arg(field1)
                      .arg(field2)
                      .arg(field1)
                      .arg(field2));
        query.bindValue(QString(":%1").arg(field1), item->msComplete);
        query.bindValue(QString(":%1").arg(field2), item->msTranslate);
        query.bindValue(":extra", item->msExtra);
        query.bindValue(":times", item->miTimes);
        query.bindValue(":stick", item->mbStick);
        ok = query.exec();
    }
    else
    {
        query.prepare(QString("UPDATE %1 SET times=%2, stick=%3 WHERE ID=%4;")
                      .arg(table)
                      .arg(item->miTimes)
                      .arg(item->mbStick)
                      .arg(id));
        ok = query.exec();
    }
    if (!ok)
    {
        qDebug("error: %s", query.lastError().text().toUtf8().data());
    }
    return ok;
}

bool XYDatabaseOperation::insertData(const QList<XYTranslateItem *> &list, const QString &table)
{
    QString field1, field2;
    if (table.toLower().contains("english"))
    {
        field1 = "english";
        field2 = "translate";
    }
    else
    {
        field1 = "pingying";
        field2 = "chinese";
    }
    QSqlDatabase::database("XYInout").transaction();   //开始一个事务
    QSqlQuery query(QSqlDatabase::database("XYInout"));
    bool ok = true;
    for (int i = 0; i < list.size(); ++i)
    {
        query.prepare(QString("INSERT INTO %1 (id, %2, %3, extra, times, stick) "
                              "VALUES (:id, :%4, :%5, :extra, :times, :stick);")
                      .arg(table)
                      .arg(field1)
                      .arg(field2)
                      .arg(field1)
                      .arg(field2));
        query.bindValue(QString(":%1").arg(field1), list.at(i)->msComplete);
        query.bindValue(QString(":%1").arg(field2), list.at(i)->msTranslate);
        query.bindValue(":extra", list.at(i)->msExtra);
        query.bindValue(":times", list.at(i)->miTimes);
        query.bindValue(":stick", list.at(i)->mbStick);
        ok = query.exec();
        if (!ok)
        {
            qDebug("error: %s", query.lastError().text().toUtf8().data());
            break;
        }
    }
    if (ok)
    {
        QSqlDatabase::database("XYInout").commit();
    }
    else
    {
        QSqlDatabase::database("XYInout").rollback();
    }
    return ok;
}

bool XYDatabaseOperation::delItem(XYTranslateItem *item)
{
    QSqlQuery query(QSqlDatabase::database("XYInout"));
    if (item->msSource == "singlePingying") // 理论上基础的几个表内容都不能删除，这里只不准删除单字的表
    {
        qDebug("Can't delete item in singlePingying!");
        return false;
    }
    bool ok = query.exec(QString("DELETE FROM %1 WHERE id = %2;")
                         .arg(item->msSource)
                         .arg(item->miID));
    if (!ok)
    {
        qDebug("error: %s", query.lastError().text().toUtf8().data());
    }
    return ok;
}

QList<XYTranslateItem *> XYDatabaseOperation::findData(const QString &key, const QString &number, const QString &table, bool *haveFind, int max)
{
    static bool comein = false;
    QList<XYTranslateItem *> list;
    if (haveFind)
    {
        *haveFind = false;
    }
    if (comein)
    {
        return list;
    }
    else
    {
        comein = true;
    }

    QString table_fact = table;
    if (table_fact == "basePintying" && !key.isEmpty())
    {
        table_fact += QString("_%1").arg(key.at(0).toUpper());
    }
    QString field1, field2;
    if (table.toLower().contains("english"))
    {
        field1 = "english";
        field2 = "translate";
    }
    else
    {
        field1 = "pingying";
        field2 = "chinese";
    }
    QSqlQuery query(QSqlDatabase::database("XYInout"));
    bool ok = query.exec(QString("SELECT id, %1, %2, extra, times, stick FROM %3 "
                       "WHERE %4 like \"%5\" AND extra like \"%6\" "
                       "ORDER BY times DESC LIMIT 0,%7;")
               .arg(field1)
               .arg(field2)
               .arg(table_fact)
               .arg(field1)
               .arg(key)
               .arg(number)
               .arg(max));

    if (!ok)
    {
        qDebug("error: %s", query.lastError().text().toUtf8().data());
    }

    while (query.next())
    {
        list.append(new XYTranslateItem(table,
                                        query.value(2).toString(),
                                        query.value(1).toString(),
                                        query.value(3).toString(),
                                        query.value(0).toInt(),
                                        query.value(4).toInt(),
                                        query.value(5).toBool()));
    }

    comein = false;
    if (haveFind)
    {
        *haveFind = true;
    }
    return list;
}

