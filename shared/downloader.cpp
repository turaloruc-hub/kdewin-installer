/****************************************************************************
**
** Copyright (C) 2005 Ralf Habacker. All rights reserved.
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

#include <QtNetwork>
#include <QtCore>

#include "packagelist.h"
#include "package.h"
#include "downloader.h"
#include "downloaderprogress.h"

Downloader::Downloader(bool _blocking, DownloaderProgress *_progress)
 : m_progress(_progress), m_ioDevice(0), m_file(0), m_httpGetId(~0U),
   m_httpRequestAborted(false), m_blocking(_blocking), m_eventLoop(0)
{
//  if(!m_progress)
//    m_progress = new DownloaderProgress(this);
  init();
}

void Downloader::init()
{
	m_http = new QHttp(this);
	
	connect(m_http, SIGNAL(requestFinished(int, bool)),this, SLOT(httpRequestFinished(int, bool)));
	connect(m_http, SIGNAL(dataReadProgress(int, int)),this, SLOT(updateDataReadProgress(int, int)));
	connect(m_http, SIGNAL(responseHeaderReceived(const QHttpResponseHeader &)),this, SLOT(readResponseHeader(const QHttpResponseHeader &)));
	connect(m_http, SIGNAL(done(bool)),this, SLOT(allDone(bool)));
	connect(m_http, SIGNAL(stateChanged(int)),this, SLOT(stateChanged(int)));
}

Downloader::~Downloader() 
{
	delete m_progress;
	delete m_http;
	delete m_eventLoop;
  delete m_file;
}

void Downloader::setError(const QString &text)
{
	qDebug() << text; 
}

bool Downloader::start(const QString &_url, const QString &_fileName)
{
  QString fileName = _fileName;

  if(_fileName.isEmpty()) {
    QFileInfo fi(QUrl(_url).path());
    fileName = fi.fileName();
  }
  m_file = new QFile(fileName);
  if (!m_file->open(QIODevice::WriteOnly)) {
    setError(tr("Unable to open file %1: %2.").arg(_fileName).arg(m_file->errorString()));
    delete m_file;
    m_file = 0;
    return false;
  }
  m_progress->setTitle(tr("Downloading %1 to %2").arg(_url).arg(m_file->fileName()));
  return startInternal(_url, m_file);
}

bool Downloader::start(const QString &_url, QByteArray &ba)
{
  m_progress->setTitle(tr("Downloading %1").arg(_url));
  QBuffer *buf = new QBuffer(&ba);

  if(!buf->open(QIODevice::WriteOnly)) {
    setError(tr("Internal error!"));
    return false;
  }
  return startInternal(_url, buf);
}

bool Downloader::startInternal(const QString &_url, QIODevice *ioDev)
{
  QUrl url(_url);
  m_ioDevice = ioDev;

  m_http->setHost(url.host(), url.port() != -1 ? url.port() : 80);
  if (!url.userName().isEmpty())
    m_http->setUser(url.userName(), url.password());

  QByteArray query = url.encodedQuery(); 
  m_httpRequestAborted = false;
  m_httpGetId = m_http->get(url.path() + (!query.isEmpty() ? "?" + url.encodedQuery() : ""), m_ioDevice);

 	m_progress->show();
	if (m_blocking) {
		m_eventLoop = new QEventLoop(); 
		m_eventLoop->exec();
		delete m_eventLoop;
	}		
	return true;
}

void Downloader::cancel()
{
	m_progress->setStatus(tr("Download canceled."));
	m_httpRequestAborted = true;
	m_http->abort();
}

void Downloader::httpRequestFinished(int requestId, bool error)
{
	if (m_httpRequestAborted) {
		if (m_file) {
			m_file->close();
			m_file->remove();
			delete m_file;
			m_file = 0;
      m_ioDevice = 0;
		} else
    if (m_ioDevice) {
      m_ioDevice->close();
      delete m_ioDevice;
      m_ioDevice = 0;
    }
	
		if (m_progress)
			m_progress->hide();
    return;
	}
	
	if (requestId != m_httpGetId)
    return;
	
  if (m_progress)
    m_progress->hide();
  m_ioDevice->close();
	
	if (error) {
    m_file->remove();
    setError(tr("Download failed: %1.").arg(m_http->errorString()));
	} else {
		m_progress->setStatus(tr("download ready"));
	}
	delete m_ioDevice;
	m_ioDevice = 0;
	m_file = 0;
}

void Downloader::readResponseHeader(const QHttpResponseHeader &responseHeader)
{
	if (responseHeader.statusCode() != 200) {
		setError(tr("Download failed: %1.").arg(responseHeader.reasonPhrase()));
		m_httpRequestAborted = true;
		m_progress->hide();
		m_http->abort();
		return;
	}
}

void Downloader::updateDataReadProgress(int bytesRead, int totalBytes)
{
	if (m_httpRequestAborted)
	    return;
	
	if (m_progress) {
	  m_progress->setMaximum(totalBytes);
	  m_progress->setValue(bytesRead);
	}
}

void Downloader::allDone(bool error)
{
	emit done(error);
	if (m_blocking)
		m_eventLoop->quit();
}


void Downloader::stateChanged(int state)
{
	QString stateLabel;
	switch (state) {
		case QHttp::Unconnected  : stateLabel = "Unconnected"; break;
		case QHttp::HostLookup   : stateLabel = "HostLookup "; break;
		case QHttp::Connecting   : stateLabel = "Connecting "; break;
		case QHttp::Sending      : stateLabel = "Sending    "; break;
		case QHttp::Reading      : stateLabel = "Reading    "; break;
		case QHttp::Connected    : stateLabel = "Connected  "; break;
		case QHttp::Closing      : stateLabel = "Closing    "; break;
	}
	m_progress->setStatus(stateLabel);
}

#include "downloader.moc"
