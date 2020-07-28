﻿/*
 * Copyright 2018-2020 Qter(qsaker@qq.com). All rights reserved.
 *
 * The file is encoding with utf-8 (with BOM). It is a part of QtSwissArmyKnife
 * project(https://www.qsak.pro). The project is an open source project. You can
 * get the source of the project from: "https://github.com/qsak/QtSwissArmyKnife"
 * or "https://gitee.com/qsak/QtSwissArmyKnife". Also, you can join in the QQ
 * group which number is 952218522 to have a communication.
 */
#include <QDebug>
#include <QDateTime>

#include "SAKGlobal.hh"
#include "SAKDebugPage.hh"
#include "SAKDataStruct.hh"
#include "SAKOtherTimingSentItem.hh"

#include "ui_SAKOtherTimingSentItem.h"

SAKOtherTimingSentItem::SAKOtherTimingSentItem(SAKDebugPage *debugPage, QWidget *parent)
    :QWidget(parent)
    ,debugPage(debugPage)
    ,ui(new Ui::SAKTimingSendingItemWidget)
{
    initUi();
    id = QDateTime::currentMSecsSinceEpoch();
}

SAKOtherTimingSentItem::SAKOtherTimingSentItem(SAKDebugPage *debugPage,
                                                       quint64 id,
                                                       quint32 interval,
                                                       quint32 format,
                                                       QString comment,
                                                       QString data,
                                                       QWidget *parent)
    :QWidget(parent)
    ,debugPage(debugPage)
    ,id(id)
    ,ui(new Ui::SAKTimingSendingItemWidget)
{
    initUi();

    timingTimeLineEdit->setText(QString::number(interval));
    textFormatComboBox->setCurrentIndex(format);
    remarkLineEdit->setText(comment);
    inputDataTextEdit->setText(data);
}

SAKOtherTimingSentItem::~SAKOtherTimingSentItem()
{
    delete ui;
}

quint64 SAKOtherTimingSentItem::parameterID()
{
    return id;
}

quint32 SAKOtherTimingSentItem::parameterInterval()
{
    return timingTimeLineEdit->text().toUInt();
}

quint32 SAKOtherTimingSentItem::parameterFormat()
{
    return textFormatComboBox->currentIndex();
}

QString SAKOtherTimingSentItem::parameterComment()
{
    return remarkLineEdit->text();
}

QString SAKOtherTimingSentItem::parameterData()
{
    return inputDataTextEdit->toPlainText();
}

void SAKOtherTimingSentItem::write()
{
    QString data = inputDataTextEdit->toPlainText();

    if (!data.isEmpty()){
        int textFormat = this->textFormatComboBox->currentData().toInt();
        debugPage->writeRawData(data, textFormat);
    }
}

void SAKOtherTimingSentItem::initUi()
{
    ui->setupUi(this);

    timingCheckBox = ui->timingCheckBox;
    timingTimeLineEdit = ui->timingTimeLineEdit;
    textFormatComboBox = ui->textFormatComboBox;
    remarkLineEdit = ui->remarkLineEdit;
    inputDataTextEdit = ui->inputDataTextEdit;
    updatePushButton = ui->updatePushButton;

    writeTimer.setInterval(timingTimeLineEdit->text().toInt());
    connect(&writeTimer, &QTimer::timeout, this, &SAKOtherTimingSentItem::write);

    SAKGlobal::initInputTextFormatComboBox(textFormatComboBox);
}

void SAKOtherTimingSentItem::on_timingCheckBox_clicked()
{
    if (timingCheckBox){
        timingCheckBox->isChecked() ? writeTimer.start() : writeTimer.stop();
    }
}

void SAKOtherTimingSentItem::on_timingTimeLineEdit_textChanged(const QString &text)
{
    int interval = text.toInt();
    writeTimer.setInterval(interval == 0 ? 1000 : interval);
}

void SAKOtherTimingSentItem::on_updatePushButton_clicked()
{
    QString tableName = SAKDataStruct::timingSendingTableName(debugPage->pageType());
    SAKDataStruct::SAKStructTimingSendingItem sendingItem;
    sendingItem.id = parameterID();
    sendingItem.data = parameterData();
    sendingItem.format = parameterFormat();
    sendingItem.comment = parameterComment();
    sendingItem.interval = parameterInterval();
//    SAKDebugPageDatabaseInterface *databaseInterface = SAKDebugPageDatabaseInterface::instance();
//    databaseInterface->updateTimingSendingItem(tableName, sendingItem);
}