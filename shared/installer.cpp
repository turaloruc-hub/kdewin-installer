/****************************************************************************
**
** Copyright (C) 2005-2007 Ralf Habacker. All rights reserved.
**
** This file is part of the KDE installer for windows
**
** This library is free software; you can redistribute it and/or
** modify it under the terms of the GNU Library General Public
** License version 2 as published by the Free Software Foundation.
**
** This library is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
** Library General Public License for more details.
**
** You should have received a copy of the GNU Library General Public License
** along with this library; see the file COPYING.LIB.  If not, write to
** the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
** Boston, MA 02110-1301, USA.
**
****************************************************************************/

#include <QtCore>
#include <QtDebug>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

#include "config.h"
#include "installer.h"
#include "packagelist.h"
#include "installerprogress.h"

#include "quazip.h"
#include "quazipfile.h"
#include "tarfilter.h"
#include "bzip2filter.h"

#ifdef SEVENZIP_UNPACK_SUPPORT
 #include "qua7zip.h"
 using namespace qua7zip;
#endif

//#define DEBUG
#ifdef Q_CC_MSVC
# define __PRETTY_FUNCTION__ __FUNCTION__
#endif

Installer::Installer(InstallerProgress *_progress)
        : QObject(), m_progress(_progress), m_type(Installer::Standard)
{
    m_root = ".";
//  packageList = _packageList;
//  if(packageList)
//  {
//      packageList->m_root = m_root;
//  }
//  connect (packageList,SIGNAL(configLoaded()),this,SLOT(updatePackageList()));
}

Installer::~Installer()
{}

void Installer::setRoot(const QString &root)
{
    m_root = root;
//    packageList->m_root = m_root = root;
//    QDir().mkdir(m_root);
}

bool Installer::isEnabled()
{
    return true;
}

bool Installer::loadConfig()
{
#ifdef DEBUG
    qDebug() << __PRETTY_FUNCTION__;
#endif
    return true;
}

void Installer::updatePackageList()
{
#ifdef DEBUG
    qDebug() << __PRETTY_FUNCTION__;
#endif

    loadConfig();
}

#ifndef QUNZIP_BUFFER
# define QUNZIP_BUFFER (256 * 1024)
#endif
bool Installer::createManifestFile(const QString &destpath, const QString &zipFile)
{
  // write manifest file if not exist or is corrupted
  QFileInfo f(zipFile);
  QFileInfo a(destpath +"/manifest/"+ f.fileName().replace(".zip",".mft"));
  if (a.exists() && a.isFile() && a.size() > 0)
    return true;
    // in some gnuwin32 packages the manifest file is a directory
  if (a.isDir())
  {
    QDir dd;
    dd.rmdir(a.absoluteFilePath());
  }
  m_files << "manifest/"+ f.fileName().replace(".zip",".mft");
  m_files << "manifest/"+ f.fileName().replace(".zip",".ver");
  QFile fo(a.absoluteFilePath());
  if (!fo.open(QIODevice::WriteOnly | QIODevice::Text))
    return false;
  QTextStream so(&fo);
  so << m_files.join("\n");

    // write .ver file if not exist
  QFileInfo b(destpath +"/manifest/"+ f.fileName().replace(".zip",".ver"));
  if (b.exists() && b.isFile() && b.size() > 0)
    return true;
  QFile verFile(b.absoluteFilePath());
  if (!verFile.open(QIODevice::WriteOnly | QIODevice::Text))
    return false;
  QTextStream vFo(&verFile);
    //  @TODO fill ver file with usefull values
  vFo << "dummy x.y.z Content description\n";
  vFo << "dummy:";

  return true;
}

bool Installer::unbz2File(const QString &destpath, const QString &zipFile, const StringHash &pathRelocations)
{
  QDir path(destpath);

  if(!path.exists())
  {
    setError(tr("Internal Error - Path %1 does not exist").arg(path.absolutePath()));
    return false;
  }

    // z.setFileNameCodec("Windows-1252"); // important!

  QFile file(zipFile);
  if(!file.open(QIODevice::ReadOnly)) {
    setError(tr("Error opening %1 for reading").arg(zipFile));
    return false;
  }
  BZip2IODevice bzip2(&file);
  if(!bzip2.open(QIODevice::ReadOnly)) {
    setError(tr("Error opening %1 for reading (%2)").arg(zipFile).arg(bzip2.errorString()));
    return false;
  }
  m_files.clear();

  // FIXME: progress not possible
  //       -> use file.size() and bzip2.pos() ??
  forever
  {
    TarFilter tf(&bzip2);
    TarFilter::FileInformations tarFileInfo;
    while(tf.getData(tarFileInfo)) {
      // relocate path names
      QString name = tarFileInfo.fileName;
      for(StringHash::const_iterator i = pathRelocations.constBegin(); i != pathRelocations.constEnd(); i++)
      {
        name = name.replace(i.key(),i.value());
      }
      m_files << name;
      QString outPath = path.filePath(name);
      QFileInfo fi(outPath);

          // is it's a subdir ?
      if(tarFileInfo.fileType == TarFilter::directory)
      {
        if(fi.exists())
        {
          if(!fi.isDir())
          {
            setError(tr("Can not create directory %1").arg(fi.absoluteFilePath()));
            return false;
          }
          continue;
        }
        if(!path.mkpath(fi.absoluteFilePath()))
        {
          setError(tr("Can not create directory %1").arg(fi.absolutePath()));
          return false;
        }
        continue;
      }
      // some archives does not have directory entries
      else
      {
        if(!path.exists(fi.absolutePath()))
        {
          if (!path.mkpath(fi.absolutePath()))
          {
            setError(tr("Can not create directory %1").arg(fi.absolutePath()));
            return false;
          }
        }
        continue;
      }

      if(tarFileInfo.fileType != TarFilter::regular2 && tarFileInfo.fileType != TarFilter::regular2) {
        setError(tr("Can not unpack %1 - unsupported filetype %1").arg(tarFileInfo.fileName).arg(tarFileInfo.fileType));
        continue;
      }

          // create new file
      QFile newFile(fi.absoluteFilePath());
      if(!newFile.open(QIODevice::WriteOnly))
      {
        setError(tr("Can not creating file %1").arg(fi.absoluteFilePath()));
        return false;
      }

      if (m_progress)
        m_progress->setTitle(tr("Installing %1").arg(newFile.fileName()));

      if(!tf.getData(&newFile)) {
        setError(tr("Can't write to file %1 (%2)").arg(fi.absoluteFilePath()).arg(newFile.errorString()));
        return false;
      }
      newFile.close();
    }
  }

  bzip2.close();
  return true;
}

bool Installer::untgzFile(const QString &destpath, const QString &zipFile, const StringHash &pathRelocations)
{
  QDir path(destpath);
  QuaZip z(zipFile);

  if(!z.open(QuaZip::mdUnzip))
  {
    setError(tr("Can not open %1").arg(zipFile));
    return false;
  }

  if(!path.exists())
  {
    setError(tr("Internal Error - Path %1 does not exist").arg(path.absolutePath()));
    return false;
  }

    // z.setFileNameCodec("Windows-1252"); // important!

  QuaZipFile file(&z);
  QuaZipFileInfo info;
  m_files.clear();
  if (m_progress)
  {
    m_progress->setMaximum(z.getEntriesCount());
    m_progress->show();
  }

  for(bool bOk = z.goToFirstFile(); bOk; bOk = z.goToNextFile())
  {
    // get zipfile information
    if(!z.getCurrentFileInfo(&info))
    {
      setError(tr("Can not get file information from tgz file %1").arg(zipFile));
      return false;
    }
    if(!file.open(QIODevice::ReadOnly))
    {
      setError(tr("Can not open file %1 from zip file %2").arg(info.name).arg(zipFile));
      return false;
    }
    if(file.getZipError() != UNZ_OK)
    {
      setError(tr("Error reading zip file %1").arg(zipFile));
      return false;
    }
    TarFilter tf(&file);
    TarFilter::FileInformations tarFileInfo;
    QByteArray ba;
    while(tf.getData(tarFileInfo, ba)) {
      // relocate path names
      QString name = tarFileInfo.fileName;
      for(StringHash::const_iterator i = pathRelocations.constBegin(); i != pathRelocations.constEnd(); i++)
      {
        name = name.replace(i.key(),i.value());
      }
      m_files << name;
      QString outPath = path.filePath(name);
      QFileInfo fi(outPath);

          // is it's a subdir ?
      if(tarFileInfo.fileType == TarFilter::directory)
      {
        if(fi.exists())
        {
          if(!fi.isDir())
          {
            setError(tr("Can not create directory %1").arg(fi.absoluteFilePath()));
            return false;
          }
          continue;
        }
        if(!path.mkpath(fi.absoluteFilePath()))
        {
          setError(tr("Can not create directory %1").arg(fi.absolutePath()));
          return false;
        }
        continue;
      }
      // some archives does not have directory entries
      else
      {
        if(!path.exists(fi.absolutePath()))
        {
          if (!path.mkpath(fi.absolutePath()))
          {
            setError(tr("Can not create directory %1").arg(fi.absolutePath()));
            return false;
          }
        }
        continue;
      }

      if(tarFileInfo.fileType != TarFilter::regular2 && tarFileInfo.fileType != TarFilter::regular2) {
        setError(tr("Can not unpack %1 - unsupported filetype %1").arg(tarFileInfo.fileName).arg(tarFileInfo.fileType));
        continue;
      }

          // create new file
      QFile newFile(fi.absoluteFilePath());
      if(!newFile.open(QIODevice::WriteOnly))
      {
        setError(tr("Can not creating file %1").arg(fi.absoluteFilePath()));
        return false;
      }

      if (m_progress)
        m_progress->setTitle(tr("Installing %1").arg(newFile.fileName()));

      qint64 iBytesRead;
      QByteArray ba;
      ba.resize(QUNZIP_BUFFER);

      while((iBytesRead = file.read(ba.data(), QUNZIP_BUFFER)) > 0)
        newFile.write(ba.data(), iBytesRead);

      newFile.close();
    }

    file.close();
    if(file.getZipError() != UNZ_OK)
    {
      setError(tr("Error reading zip file %1").arg(zipFile));
      return false;
    }
  }
  z.close();
  if (m_progress)
  {
    m_progress->hide();
  }

  if(z.getZipError() != UNZ_OK)
  {
    setError(tr("Error reading zip file %1").arg(zipFile));
    return false;
  }
  return true;
}

bool Installer::unzipFile(const QString &destpath, const QString &zipFile, const StringHash &pathRelocations)
{
  QDir path(destpath);
  QuaZip z(zipFile);

  if(!z.open(QuaZip::mdUnzip))
  {
    setError(tr("Can not open %1").arg(zipFile));
    return false;
  }

  if(!path.exists())
  {
    setError(tr("Internal Error - Path %1 does not exist").arg(path.absolutePath()));
    return false;
  }

    // z.setFileNameCodec("Windows-1252"); // important!

  QuaZipFile file(&z);
  QuaZipFileInfo info;
  m_files.clear();
  if (m_progress)
  {
    m_progress->setMaximum(z.getEntriesCount());
    m_progress->show();
  }

  for(bool bOk = z.goToFirstFile(); bOk; bOk = z.goToNextFile())
  {
        // get file information
    if(!z.getCurrentFileInfo(&info))
    {
      setError(tr("Can not get file information from zip file %1").arg(zipFile));
      return false;
    }
        // relocate path names
    QString name = info.name;
    for(StringHash::const_iterator i = pathRelocations.constBegin(); i != pathRelocations.constEnd(); i++)
    {
      name = name.replace(i.key(),i.value());
    }
    m_files << name;
    QString outPath = path.filePath(name);
    QFileInfo fi(outPath);

        // is it's a subdir ?
    if(info.compressedSize == 0 && info.uncompressedSize == 0)
    {
      if(fi.exists())
      {
        if(!fi.isDir())
        {
          setError(tr("Can not create directory %1").arg(fi.absoluteFilePath()));
          return false;
        }
        continue;
      }
      if(!path.mkpath(fi.absoluteFilePath()))
      {
        setError(tr("Can not create directory %1").arg(fi.absolutePath()));
        return false;
      }
      continue;
    }
        // some archives does not have directory entries
    else
    {
      if(!path.exists(fi.absolutePath()))
      {
        if (!path.mkpath(fi.absolutePath()))
        {
          setError(tr("Can not create directory %1").arg(fi.absolutePath()));
          return false;
        }
      }
    }
        // open file
    if(!file.open(QIODevice::ReadOnly))
    {
      setError(tr("Can not open file %1 from zip file %2").arg(info.name).arg(zipFile));
      return false;
    }
    if(file.getZipError() != UNZ_OK)
    {
      setError(tr("Error reading zip file %1").arg(zipFile));
      return false;
    }

        // create new file
    QFile newFile(fi.absoluteFilePath());
    if(!newFile.open(QIODevice::WriteOnly))
    {
      setError(tr("Can not creating file %1").arg(fi.absoluteFilePath()));
      return false;
    }

    if (m_progress)
    {
      m_progress->setTitle(tr("Installing %1").arg(newFile.fileName()));
    }
        // copy data
        // FIXME: check for not that huge filesize ?
    qint64 iBytesRead;
    QByteArray ba;
    ba.resize(QUNZIP_BUFFER);

    while((iBytesRead = file.read(ba.data(), QUNZIP_BUFFER)) > 0)
      newFile.write(ba.data(), iBytesRead);

    file.close();
    newFile.close();

    if(file.getZipError() != UNZ_OK)
    {
      setError(tr("Error reading zip file %1").arg(zipFile));
      return false;
    }
  }
  z.close();
  if (m_progress)
  {
    m_progress->hide();
  }

  if(z.getZipError() != UNZ_OK)
  {
    setError(tr("Error reading zip file %1").arg(zipFile));
    return false;
  }
    // write manifest file if not exist or is corrupted
  QFileInfo f(zipFile);
  QFileInfo a(destpath +"/manifest/"+ f.fileName().replace(".zip",".mft"));
  if (a.exists() && a.isFile() && a.size() > 0)
    return true;
    // in some gnuwin32 packages the manifest file is a directory
  if (a.isDir())
  {
    QDir dd;
    dd.rmdir(a.absoluteFilePath());
  }
  m_files << "manifest/"+ f.fileName().replace(".zip",".mft");
  m_files << "manifest/"+ f.fileName().replace(".zip",".ver");
  QFile fo(a.absoluteFilePath());
  if (!fo.open(QIODevice::WriteOnly | QIODevice::Text))
    return false;
  QTextStream so(&fo);
  so << m_files.join("\n");

    // write .ver file if not exist
  QFileInfo b(destpath +"/manifest/"+ f.fileName().replace(".zip",".ver"));
  if (b.exists() && b.isFile() && b.size() > 0)
    return true;
  QFile verFile(b.absoluteFilePath());
  if (!verFile.open(QIODevice::WriteOnly | QIODevice::Text))
    return false;
  QTextStream vFo(&verFile);
    //  @TODO fill ver file with usefull values
  vFo << "dummy x.y.z Content description\n";
  vFo << "dummy:";

  return true;
}

bool Installer::un7zipFile(const QString &destpath, const QString &zipFile)
{
#ifdef SEVENZIP_UNPACK_SUPPORT
    QDir path(destpath);
    Qua7zip z(zipFile);

    if(!z.open(Qua7zip::mdUnpack))
    {
        setError(tr("Can not open %1").arg(zipFile));
        return false;
    }

    if(!path.exists())
    {
        setError(tr("Internal Error - Path %1 does not exist").arg(path.absolutePath()));
        return false;
    }

    // z.setFileNameCodec("Windows-1252"); // important!

    Qua7zipFile file(&z);
    Qua7zipFileInfo info;
    if (m_progress)
    {
        m_progress->setMaximum(z.getEntriesCount());
        m_progress->show();
    }

    for(bool bOk = z.goToFirstFile(); bOk; bOk = z.goToNextFile())
    {
        // get file information
        if(!z.getCurrentFileInfo(info))
        {
            setError(tr("Can not get file information from zip file %1").arg(zipFile));
            return false;
        }
        QFileInfo fi(path.filePath(info.fileName));

        // is it's a subdir ?
        if(info.isDir)
        {
            if(fi.exists())
            {
                if(!fi.isDir())
                {
                    setError(tr("Can not create directory %1").arg(fi.absoluteFilePath()));
                    return false;
                }
                continue;
            }
            if(!path.mkpath(fi.absoluteFilePath()))
            {
                setError(tr("Can not create directory %1").arg(fi.absolutePath()));
                return false;
            }
            continue;
        }
        // some archives does not have directory entries
        else
        {
            if(!path.exists(fi.absolutePath()))
            {
                if (!path.mkpath(fi.absolutePath()))
                {
                    setError(tr("Can not create directory %1").arg(fi.absolutePath()));
                    return false;
                }
            }
        }
        // open file
        if(!file.open(QIODevice::ReadOnly))
        {
            setError(tr("Can not open file %1 from zip file %2").arg(info.fileName).arg(zipFile));
            return false;
        }
        if(file.get7zipError() != SZ_OK)
        {
            setError(tr("Error reading zip file %1").arg(zipFile));
            return false;
        }

        // create new file
        QFile newFile(fi.absoluteFilePath());
        if(!newFile.open(QIODevice::WriteOnly))
        {
            setError(tr("Can not creating file %1").arg(fi.absoluteFilePath()));
            return false;
        }

        if (m_progress)
        {
            m_progress->setTitle(tr("Installing %1").arg(newFile.fileName()));
        }
        // copy data
        // FIXME: check for not that huge filesize ?
        qint64 iBytesRead;
        QByteArray ba;
        ba.resize(QUNZIP_BUFFER);

        while((iBytesRead = file.read(ba.data(), QUNZIP_BUFFER)) > 0)
            newFile.write(ba.data(), iBytesRead);

        file.close();
        newFile.close();

        if(file.get7zipError() != SZ_OK)
        {
            setError(tr("Error reading zip file %1").arg(zipFile));
            return false;
        }
    }
    z.close();
    if(z.get7zipError() != SZ_OK)
    {
        setError(tr("Error reading zip file %1").arg(zipFile));
        return false;
    }
    return true;
#else
    return false;
#endif
}


void Installer::setError(const QString &str)
{
    qWarning(str.toLocal8Bit().data());
}

bool Installer::createQtConfigFile()
{
    QSettings qtConfig(m_root + "/bin/qt.conf",QSettings::IniFormat);
    qtConfig.setValue("Paths/Prefix",m_root);
    qtConfig.setValue("Translations","i18n");
    qtConfig.sync();
    return true;
}

bool Installer::install(const QString &fileName, const StringHash &pathRelocations)
{
    if (fileName.endsWith(".zip"))
    {
      if(!unzipFile(m_root, fileName, pathRelocations))
        return false;
    } else
    if (fileName.endsWith(".tar.gz") || fileName.endsWith(".tgz"))
    {
      if(!untgzFile(m_root, fileName, pathRelocations))
        return false;
    } else
    if (fileName.endsWith(".7z"))
    {
        if(!un7zipFile(m_root, fileName))
          return false;
    }
#ifdef Q_WS_WIN
    else // for all other formats use windows assignments
    {
        // fixme: use QProcess to determine if all worked fine?
        ShellExecuteW(0, L"open", (WCHAR*)fileName.utf16(), NULL, NULL, SW_SHOWNORMAL);
        return true;
    }
#endif
    createManifestFile(m_root, fileName);
    QFileInfo fi(fileName);
    if(fi.fileName().startsWith("qt"))
        createQtConfigFile();

    return true;
}

#include "installer.moc"
