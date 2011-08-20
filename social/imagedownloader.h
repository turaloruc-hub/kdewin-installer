/****************************************************************************
**
** Copyright (C) 2011 Constantin Tudorica <tudalex@gmail.com>
** All rights reserved.
**
** This file is part of the KDE installer for windows
**
** This file may be used under the terms of the GNU General Public
** License version 2.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of
** this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
** http://www.trolltech.com/products/qt/opensource.html
**
** If you are unsure which license is appropriate for your use, please
** review the following information:
** http://www.trolltech.com/products/qt/licensing.html or contact the
** sales department at sales@trolltech.com.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#ifndef IMAGEDOWNLOADER_H
#define IMAGEDOWNLOADER_H

#include <QObject>
#include <QByteArray>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>

class ImageDownloader : public QObject
{
    Q_OBJECT
public:
    explicit ImageDownloader(QUrl imageUrl, QObject *parent = 0);
    virtual ~ImageDownloader();
    QByteArray dowloadedData();
signals:
    void downloaded();
public slots:
    void fileDownloaded();
    void error(QNetworkReply::NetworkError code);
private:
    QNetworkAccessManager m_WebCtrl;
    QByteArray m_DownloadedData;
    QNetworkReply *m_Reply;
};

#endif // IMAGEDOWNLOADER_H