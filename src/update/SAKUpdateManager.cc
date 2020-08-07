﻿/*
 * Copyright 2019-2020 Qter(qsaker@qq.com). All rights reserved.
 *
 * The file is encoded using "utf8 with bom", it is a part
 * of QtSwissArmyKnife project.
 *
 * QtSwissArmyKnife is licensed according to the terms in
 * the file LICENCE in the root of the source code directory.
 */
#include <QDebug>
#include <QLocale>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QListWidgetItem>
#include <QNetworkRequest>
#include <QDesktopServices>

#include "SAKSettings.hh"
#include "SAKApplication.hh"
#include "SAKUpdateManager.hh"
#include "SAKDownloadItemWidget.hh"

#include "ui_SAKUpdateManager.h"

static const char* checkForUpdateUrl = "https://api.github.com/repos/qsak/QtSwissArmyKnife/releases/latest";
SAKUpdateManager::SAKUpdateManager(QWidget *parent)
    :QDialog(parent)
    ,mUi (new Ui::SAKUpdateManager)
{
    mUi->setupUi(this);
    mCurrentVersionLabel = mUi->currentVersionLabel;
    mNewVersionLabel = mUi->newVersionLabel;
    mUpdateProgressLabel = mUi->updateProgressLabel;
    mUpdateProgressBar = mUi->updateProgressBar;
    mNoNewVersionTipLabel = mUi->noNewVersionTipLabel;
    mNewVersionCommentsGroupBox = mUi->newVersionCommentsGroupBox;
    mNewVersionCommentsTextBrowser = mUi->newVersionCommentsTextBrowser;
    mDownloadListListWidget = mUi->downloadListListWidget;
    mAutoCheckForUpdateCheckBox = mUi->autoCheckForUpdateCheckBox;
    mVisitWebPushButton = mUi->visitWebPushButton;
    mCheckForUpdatePushButton = mUi->checkForUpdatePushButton;
    mInfoLabel = mUi->infoLabel;

    mCurrentVersionLabel->setText(QApplication::applicationVersion());
    mNoNewVersionTipLabel->hide();

    mClearInfoTimer.setInterval(SAK_CLEAR_MESSAGE_INTERVAL);
    connect(&mClearInfoTimer, &QTimer::timeout, this, &SAKUpdateManager::clearInfo);

    // Read in setting information form settings file
    bool checked = SAKSettings::instance()->instance()->enableAutoCheckForUpdate();
    mAutoCheckForUpdateCheckBox->setChecked(checked);

    setModal(true);
}

SAKUpdateManager::~SAKUpdateManager()
{
    delete mUi;
}

void SAKUpdateManager::checkForUpdate()
{
    on_checkForUpdatePushButton_clicked();
}

bool SAKUpdateManager::enableAutoCheckedForUpdate()
{
    return SAKSettings::instance()->enableAutoCheckForUpdate();
}

void SAKUpdateManager::outputInfo(QString info, bool isError)
{
    if (isError){
        info = QString("<font color=red>%1</font>").arg(info);
    }else{
        info = QString("<font color=blue>%1</font>").arg(info);
    }

    mInfoLabel->setText(info);
    mClearInfoTimer.start();
}

void SAKUpdateManager::clearInfo()
{
    mClearInfoTimer.stop();
    mInfoLabel->clear();
}

void SAKUpdateManager::checkForUpdateFinished()
{
    if (mNetworkReply){
        if (mNetworkReply->error() == QNetworkReply::NoError){
            QByteArray data = mNetworkReply->readAll();
            mUpdateInfo = extractUpdateInfo(data);
            if (mUpdateInfo.isValid){
                if (isNewVersion(mUpdateInfo.name)){
                    mNewVersionLabel->setText(mUpdateInfo.name.remove("v"));
                    mNewVersionCommentsTextBrowser->setText(mUpdateInfo.body.replace(QString("\\r\\n"), QString("\r\n")));
                    setupDownloadList(mUpdateInfo);
                }else{
                    mNoNewVersionTipLabel->show();
                    mNewVersionLabel->setText(mUpdateInfo.name.remove("v"));
                }

                QApplication::beep();
                mUpdateProgressBar->setMinimum(0);
                mUpdateProgressBar->setMaximum(100);
                mUpdateProgressBar->setValue(100);
            }else{
                outputInfo(mUpdateInfo.errorString, true);
            }
        }else{
            QApplication::beep();            
            outputInfo(mNetworkReply->errorString(), true);
        }
    }

    mCheckForUpdatePushButton->setEnabled(true);

    delete mNetworkReply;
    mNetworkReply = Q_NULLPTR;
}

SAKUpdateManager::UpdateInfo SAKUpdateManager::extractUpdateInfo(QByteArray jsonObjectData)
{
    UpdateInfo updateInfo;
    updateInfo.isValid = false;

    if (jsonObjectData.isEmpty()){
        updateInfo.errorString = tr("Pull information from server failed!");
    }else{
        QJsonParseError jsonParseError;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonObjectData, &jsonParseError);

        // Data structure reference: resources/files/GitHubLatestReleasesDatastruct.json
        if (jsonParseError.error == QJsonParseError::NoError){
            updateInfo.isValid = true;
            QJsonObject jsonObj = jsonDoc.toVariant().toJsonObject();

            updateInfo.htmlUrl = jsonObj.value("html_url").toString();
            updateInfo.name = jsonObj.value("name").toString();
            updateInfo.browserDownloadUrl  = extractBrowserDownloadUrl(jsonObj.value("assets").toVariant().toJsonArray());
            updateInfo.body = jsonObj.value("body").toString();
            updateInfo.tarballUrl = jsonObj.value("tarball_url").toString();
            updateInfo.zipballUrl = jsonObj.value("zipball_url").toString();

            show();
        }else{
            updateInfo.errorString = jsonParseError.errorString();
        }
    }

    return updateInfo;
}

QStringList SAKUpdateManager::extractBrowserDownloadUrl(QJsonArray jsonArray)
{
    QStringList urlList;

    for(int i = 0; i < jsonArray.count(); i++){
        QJsonObject jsonObj = jsonArray.at(i).toVariant().toJsonObject();
        QString url = jsonObj.value("browser_download_url").toString();
        urlList.append(url);
    }

    return urlList;
}

bool SAKUpdateManager::isNewVersion(QString remoteVersion)
{
    auto versionStringToInt = [](QString versionString)->int{
        QStringList list = versionString.remove("v").split('.');

        int major = 0;
        int minor = 0;
        int patch = 0;
        if (list.count() == 3){
            major = list.at(0).toInt();
            minor = list.at(1).toInt();
            patch = list.at(2).toInt();
        }

        int ver = 0;
        ver = (major << 16) | (minor << 8) | patch;
        return ver;
    };

    QString localVersion = QApplication::applicationVersion();
    int remoteVer = versionStringToInt(remoteVersion);
    int localVer = versionStringToInt(localVersion);

    if (remoteVer > localVer){
        return true;
    }else{
        return false;
    }
}

void SAKUpdateManager::setupDownloadList(UpdateInfo info)
{
    clearDownloadList();

    // Binary for windows
    mDownloadListListWidget->addItem(QString("Windows"));
    appendPacketItem(info, QString(":/resources/images/Windows.png"), QString(".exe"));

    // Binary for linux
    mDownloadListListWidget->addItem(QString("Linux"));
    appendPacketItem(info, QString(":/resources/images/Linux.png"), QString(".run"));
    appendPacketItem(info, QString(":/resources/images/Linux.png"), QString(".Appimage"));

    // Binary for mac
    mDownloadListListWidget->addItem(QString("Apple"));
    appendPacketItem(info, QString(":/resources/images/Mac.png"), QString(".dmg"));

    // Binary for android
    mDownloadListListWidget->addItem(QString("Android"));
    appendPacketItem(info, QString(":/resources/images/Android.png"), QString(".pkg"));

    // Source - gz
    mDownloadListListWidget->addItem(tr("Source"));
    QListWidgetItem *item = new QListWidgetItem(QIcon(":/resources/images/Gz.png"), QString(""), mDownloadListListWidget);
    SAKDownloadItemWidget *itemWidget = new SAKDownloadItemWidget(info.tarballUrl, mDownloadListListWidget);
    item->setSizeHint(itemWidget->size());
    mDownloadListListWidget->setItemWidget(item, itemWidget);
    // source - zip
    item = new QListWidgetItem(QIcon(":/resources/images/Zip.png"), QString(""), mDownloadListListWidget);
    itemWidget = new SAKDownloadItemWidget(info.zipballUrl, mDownloadListListWidget);
    item->setSizeHint(itemWidget->size());
    mDownloadListListWidget->setItemWidget(item, itemWidget);
}

void SAKUpdateManager::clearDownloadList()
{
    while(mDownloadListListWidget->count()){
        QListWidgetItem *item = mDownloadListListWidget->item(0);
        mDownloadListListWidget->removeItemWidget(item);
        delete item;
    }
}

void SAKUpdateManager::appendPacketItem(UpdateInfo info, QString icon, QString key)
{
    for(auto var:info.browserDownloadUrl){
        if (var.contains(key)){
            QListWidgetItem *item = new QListWidgetItem(QIcon(icon), QString(""), mDownloadListListWidget);
            SAKDownloadItemWidget *itemWidget = new SAKDownloadItemWidget(var, mDownloadListListWidget);

            item->setSizeHint(itemWidget->size());
            mDownloadListListWidget->setItemWidget(item, itemWidget);
        }
    }
}

void SAKUpdateManager::on_autoCheckForUpdateCheckBox_clicked()
{
    SAKSettings::instance()->setEnableAutoCheckForUpdate(mAutoCheckForUpdateCheckBox->isChecked());
}

void SAKUpdateManager::on_visitWebPushButton_clicked()
{
    if (QLocale().country() == QLocale::China){
        QDesktopServices::openUrl(QUrl("https://gitee.com/qsak/QtSwissArmyKnife/releases"));
    }else{
        QDesktopServices::openUrl(QUrl("https://github.com/qsak/QtSwissArmyKnife/releases"));
    }
}

void SAKUpdateManager::on_checkForUpdatePushButton_clicked()
{
    mUpdateProgressBar->setMaximum(0);
    mUpdateProgressBar->setMaximum(0);
    mNoNewVersionTipLabel->hide();
    mCheckForUpdatePushButton->setEnabled(false);

    mNewVersionCommentsTextBrowser->clear();
    clearDownloadList();

    mNetworkReply = mNetworkAccessManager.get(QNetworkRequest(QUrl(checkForUpdateUrl)));
    connect(mNetworkReply, &QNetworkReply::finished, this, &SAKUpdateManager::checkForUpdateFinished);
}
