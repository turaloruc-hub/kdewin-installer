/****************************************************************************
**
** Copyright (C) 2006 Ralf Habacker. All rights reserved.
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


#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QBuffer>

#include "packager.h"
#include "quazip.h" 
#include "quazipfile.h" 


// FIXME: generate manifest files 
// FIXME: do not display full path only beginning from given dir

#ifndef QZIP_BUFFER
# define QZIP_BUFFER (256 * 1024)
#endif


Packager::Packager(const QString &packageName, const QString &packageVersion, const QString &packageNotes)
: m_name(packageName), m_version(packageVersion), m_notes(packageNotes)
{
}

bool Packager::createZipFile(const QString &baseName, const QStringList &files, const QString &root, const QList<MemFile> &memFiles)
{
    QuaZip zip(baseName + ".zip");
    if(!zip.open(QuaZip::mdCreate)) {
       qWarning("testCreate(): zip.open(): %d", zip.getZipError());
       return false;
    }
       
    QFile inFile;
    QuaZipFile outFile(&zip);
       
    for (int l = 0; l < files.size(); l++)
    {
       inFile.setFileName(root + '/' + files[l]);
   
       if(!inFile.open(QIODevice::ReadOnly)) 
       {
           qWarning("testCreate(): inFile.open(): %s", inFile.errorString().toLocal8Bit().constData());
           return false;
       }
       if(!outFile.open(QIODevice::WriteOnly, QuaZipNewInfo(files.at(l), inFile.fileName()))) 
       {
           qWarning("testCreate(): outFile.open(): %d", outFile.getZipError());
           return false;
       }
       // copy data
       // FIXME: check for not that huge filesize ?
       qint64 iBytesRead;
       QByteArray ba;
       ba.resize(QZIP_BUFFER);

       while((iBytesRead = inFile.read(ba.data(), QZIP_BUFFER)) > 0)
          outFile.write(ba.data(), iBytesRead);

       if(outFile.getZipError()!=UNZ_OK) 
       {
          qWarning("testCreate(): outFile.putChar(): %d", outFile.getZipError());
          return false;
       }
       outFile.close();
       if(outFile.getZipError()!=UNZ_OK) 
       {
          qWarning("testCreate(): outFile.close(): %d", outFile.getZipError());
          return false;
       }
       inFile.close();
    }
    QList<MemFile>::ConstIterator it = memFiles.constBegin();
    for( ; it != memFiles.constEnd(); ++it) {
       if(!outFile.open(QIODevice::WriteOnly, QuaZipNewInfo((*it).filename))) 
       {
           qWarning("testCreate(): outFile.open(): %d", outFile.getZipError());
           return false;
       }
       outFile.write((*it).data, (*it).data.size());
       if(outFile.getZipError()!=UNZ_OK) 
       {
          qWarning("testCreate(): outFile.putChar(): %d", outFile.getZipError());
          return false;
       }
       outFile.close();
       if(outFile.getZipError()!=UNZ_OK) 
       {
          qWarning("testCreate(): outFile.close(): %d", outFile.getZipError());
          return false;
       }
    }
    
    zip.close();
    if(zip.getZipError()!=0) {
        qWarning("testCreate(): zip.close(): %d", zip.getZipError());
        return false;
        if(outFile.getZipError()!=UNZ_OK) 
        {
            qWarning("testCreate(): outFile.putChar(): %d", outFile.getZipError());
            return false;
        }
        outFile.close();
        if(outFile.getZipError()!=UNZ_OK) 
        {
            qWarning("testCreate(): outFile.close(): %d", outFile.getZipError());
            return false;
        }
    }
    return true;
} 

bool Packager::generateFileList(QStringList &fileList, const QString &root, const QString &subdir, const QString &filter, const QString &exclude)
{
   // create a QListQRegExp
   QStringList sl = exclude.split(' ');
   QList<QRegExp> rxList;

   if(!sl.contains("*.bak"))
       sl += "*.bak";

   QStringList::ConstIterator it = sl.constBegin();
   for( ; it != sl.constEnd(); ++it) {
       QRegExp rx(*it);
       rx.setPatternSyntax(QRegExp::Wildcard);

       rxList += rx;
   }

   return generateFileList(fileList, root, subdir, filter, rxList);
}

bool Packager::generateFileList(QStringList &fileList, const QString &root, const QString &subdir, const QString &filter, const QList<QRegExp> &excludeList)
{
   QDir d;
   if(subdir.isEmpty())
       d = QDir(root);
   else
       d = QDir(root + '/' + subdir);
   if (!d.exists()) {
       qDebug() << "Can't read directory" << QDir::convertSeparators(d.absolutePath());
       return false;
   }
   d.setFilter(QDir::NoDotAndDotDot | QDir::AllEntries | QDir::AllDirs);
   d.setNameFilters(filter.split(' '));
   d.setSorting(QDir::Name);

   QFileInfoList list = d.entryInfoList();
   QFileInfo fi;
     
   for (int i = 0; i < list.size(); i++) {
       const QFileInfo &fi = list[i];
       QString fn = fi.fileName();

       bool bFound = false;
       QList<QRegExp>::ConstIterator it = excludeList.constBegin();
       for( ; it != excludeList.constEnd(); ++it) {
           if((*it).exactMatch(fn)) {
               bFound = true;
               break;
           }
           if (bFound)
               break;
       }
       if (bFound)
           continue;

       if (fi.isDir()) {
           if(!subdir.isEmpty())
               fn = subdir + '/' + fn;
           generateFileList(fileList, root, fn, filter, excludeList);
       }
       else 
           fileList.append(subdir + '/' + fn);
   }
   return true;
}

bool Packager::generatePackageFileList(QStringList &fileList, const QString &dir, Packager::Type type)
{
    fileList.clear();
    switch (type) {
        case BIN:
            generateFileList(fileList, dir, "bin",   "*.exe *.dll");
            generateFileList(fileList, dir, "lib",   "*.dll");
            generateFileList(fileList, dir, "share", "*.*");
            generateFileList(fileList, dir, "data",  "*.*");
            generateFileList(fileList, dir, "etc",   "*.*");
            return true;
        case LIB:
            generateFileList(fileList, dir, "lib",     "*.dll.a");
            generateFileList(fileList, dir, "include", "*.*");
            return true;
        case DOC:
            generateFileList(fileList, dir, "doc", "*.dll.a");
            generateFileList(fileList, dir, "man", "*.*");
            return true;
        case SRC:
            generateFileList(fileList, dir, "src", "*.*");
            return true;
        default:
            break;
   }
   return false;
}

bool Packager::createManifestFiles(QStringList &fileList, Packager::Type type, QList<MemFile> &manifestFiles)
{
    QString fileNameBase = getBaseName(type); 
    QString notes;
    MemFile mf;
    
    manifestFiles.clear();

    switch(type) {
        case Packager::BIN:
            notes = "Binaries";
            break;
        case Packager::LIB:
            notes = "developer files";
            break;
        case Packager::DOC:
            notes = "documentation";
            break;
        case Packager::SRC:
            notes = "source code";
            break;
        default:
            break;
    } 

    QBuffer b(&mf.data);
    b.open(QIODevice::WriteOnly);
    QTextStream out(&b);
    out << fileList.join("\n")
        << "manifest/" + fileNameBase + ".mft" << "manifest/" + fileNameBase + ".ver";
    b.close();
    mf.filename = "manifest/" + fileNameBase + ".mft";
    manifestFiles += mf;

    mf.data.clear();

    b.setBuffer(&mf.data);
    b.open(QIODevice::WriteOnly);
    out.setDevice(&b);
    out << m_name + ' ' + m_version + ": source code\n" << "\n"
        << m_name + ": " + m_notes + '\n';
    b.close();
    mf.filename = "manifest/" + fileNameBase + ".ver";
    manifestFiles += mf;
    
    return true;    
}

bool Packager::makePackage(const QString &dir, const QString &destdir)
{
    // generate file lists
    // create manifest files 
    // create zip file 

    QStringList fileList; 
    QList<MemFile> manifestFiles;
    
    generatePackageFileList(fileList, dir, Packager::BIN);
    createManifestFiles(fileList, Packager::BIN, manifestFiles);
    if (fileList.size() > 0)
        createZipFile(getBaseName(Packager::BIN), fileList, dir, manifestFiles);
    else
        qDebug() << "no binary files found!";

    generatePackageFileList(fileList, dir, Packager::LIB);
    createManifestFiles(fileList, Packager::LIB, manifestFiles);
    if (fileList.size() > 0)
        createZipFile(getBaseName(Packager::LIB), fileList, dir, manifestFiles);

    generatePackageFileList(fileList, dir, Packager::DOC);
    createManifestFiles(fileList, Packager::DOC, manifestFiles);
    if (fileList.size() > 0)
        createZipFile(getBaseName(Packager::DOC), fileList, dir, manifestFiles);

    generatePackageFileList(fileList, dir, Packager::SRC);
    createManifestFiles(fileList, Packager::SRC, manifestFiles);
    if (fileList.size() > 0)
        createZipFile(getBaseName(Packager::SRC), fileList, dir, manifestFiles);

    return true;
}

QString Packager::getBaseName(Packager::Type type)
{
    QString name = m_name + '-' + m_version;

    switch(type) {
        case BIN:
            name += "-bin";
            break;
        case LIB:
            name += "-lib";
            break;
        case DOC:
            name += "-doc";
            break;
        case SRC:
            name += "-src";
            break;
        default:
            break;
    };
    return name;
}

