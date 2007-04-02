/****************************************************************************
**
** Copyright (C) 2006-2007 Ralf Habacker. All rights reserved.
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


#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QBuffer>
#include <QTextStream>

#include "packager.h"
#include "quazip.h" 
#include "quazipfile.h" 
#include "misc.h"
#include "md5.h"

#ifndef QZIP_BUFFER
# define QZIP_BUFFER (256 * 1024)
#endif

Packager::Packager(const QString &packageName, const QString &packageVersion, const QString &packageNotes)
: m_name(packageName), m_version(packageVersion), m_notes(packageNotes)
{
       qDebug() << m_name;
       qDebug() << m_version;
       qDebug() << m_notes;
       m_verbose = true;
}

/**
  create a zip file from a list of files
  @param zipFile archive file name 
  @param filesRootDir root dir of file list 
  @param files list of files based on filesRootDir 
  @param destRootDir root dir prefixed to any file of the files list in the archive (must have a trailing '/')
  @param memFiles list of in memory files 
*/
bool Packager::createZipFile(const QString &zipFile, const QString &filesRootDir, const QStringList &files, const QList<MemFile> &memFiles, const QString &destRootDir )
{
    QuaZip zip(zipFile + ".zip");
    if(!zip.open(QuaZip::mdCreate)) {
       qWarning("createZipFile(): zip.open(): %d", zip.getZipError());
       return false;
    }
       
    QFile inFile;
    QuaZipFile outFile(&zip);

    for (int l = 0; l < files.size(); l++)
    {
       inFile.setFileName(filesRootDir + '/' + files[l]);
   
       if(!inFile.open(QIODevice::ReadOnly)) 
       {
           qWarning("createZipFile(): inFile.open(): %s", inFile.errorString().toLocal8Bit().constData());
           return false;
       }
       QuaZipNewInfo a(destRootDir + files.at(l), inFile.fileName());
	   if(!outFile.open(QIODevice::WriteOnly, a)) 
       {
           qWarning("createZipFile(): outFile.open(): %d", outFile.getZipError());
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
          qWarning("createZipFile(): outFile.putChar(): %d", outFile.getZipError());
          return false;
       }
       outFile.close();
       if(outFile.getZipError()!=UNZ_OK) 
       {
          qWarning("createZipFile(): outFile.close(): %d", outFile.getZipError());
          return false;
       }
       inFile.close();
    }

    QList<MemFile>::ConstIterator it = memFiles.constBegin();
    for( ; it != memFiles.constEnd(); ++it) {
       if(!outFile.open(QIODevice::WriteOnly, QuaZipNewInfo((*it).filename))) 
       {
           qWarning("createZipFile(): outFile.open(): %d", outFile.getZipError());
           return false;
       }
       outFile.write((*it).data, (*it).data.size());
       if(outFile.getZipError()!=UNZ_OK) 
       {
          qWarning("createZipFile(): outFile.putChar(): %d", outFile.getZipError());
          return false;
       }
       outFile.close();
       if(outFile.getZipError()!=UNZ_OK) 
       {
          qWarning("createZipFile(): outFile.close(): %d", outFile.getZipError());
          return false;
       }
    }
    
    zip.close();
    if(zip.getZipError()!=0) {
        qWarning("createZipFile(): zip.close(): %d", zip.getZipError());
        return false;
        if(outFile.getZipError()!=UNZ_OK) 
        {
            qWarning("createZipFile(): outFile.putChar(): %d", outFile.getZipError());
            return false;
        }
        outFile.close();
        if(outFile.getZipError()!=UNZ_OK) 
        {
            qWarning("createZipFile(): outFile.close(): %d", outFile.getZipError());
            return false;
        }
    }
    return true;
} 

bool Packager::generatePackageFileList(QStringList &fileList, Packager::Type type, const QString &root)
{
    QString dir = root.isEmpty() ? m_rootDir : root;
	QString exclude;
    fileList.clear();
    if (m_name.startsWith("qt") || m_name.startsWith("q++") || m_name.startsWith("q.."))
        switch (type) {
            case BIN:
                generateFileList(fileList, dir, "bin", "*.dll *.manifest qdbus.exe qdbusviewer.exe", "*d.dll *d4.dll");
                generateFileList(fileList, dir, "lib", " *d.manifest");
                generateFileList(fileList, dir, "plugins", "*.dll","*d.dll *d4.dll *d1.dll");
                generateFileList(fileList, dir, "translations",  "*.qm");
                return true;
            case LIB:
                generateFileList(fileList, dir, "bin",   "*.exe *.bat *d4.dll", "assistant.exe qtdemo.exe qdbus.exe dbus-viewer.exe");
                generateFileList(fileList, dir, "plugins", "*d.dll *d4.dll *d1.dll");
                generateFileList(fileList, dir, ".",     ".qmake.cache");
                generateFileList(fileList, dir, "mkspecs",  "qconfig.pri");
                if (m_name.endsWith("mingw")) 
				{
                    generateFileList(fileList, dir, "mkspecs/win32-g++", "*.*");
	                generateFileList(fileList, dir, "lib",     "*.a");
				}
				else 
				{
                    generateFileList(fileList, dir, "mkspecs/win32-msvc.net", "*.*");
	                generateFileList(fileList, dir, "lib",     "*.lib");
				}
                generateFileList(fileList, dir, "include", "*","*_p.h");
                generateFileList(fileList, dir, "src/corelib", "*.h", "*_p.h");
                generateFileList(fileList, dir, "src/gui", "*.h", "*_p.h");
                generateFileList(fileList, dir, "src/qt3support", "*.h", "*_p.h");
                generateFileList(fileList, dir, "src/xml", "*.h", "*_p.h");
                generateFileList(fileList, dir, "src/network", "*.h", "*_p.h");
                generateFileList(fileList, dir, "src/svg", "*.h", "*_p.h");
                generateFileList(fileList, dir, "src/opengl", "*.h", "*_p.h");
                return true;
            case DOC:
                generateFileList(fileList, dir, "bin", "qtdemo.exe assistant.exe");
                generateFileList(fileList, dir, "doc", "*.*");
                return true;
            case SRC:
                // not supported yet
                //generateFileList(fileList, dir, "src", "*.*");
                return true;
            case NONE:
                generateFileList(fileList, dir, "", "*.*", "manifest");
                return true;
            default:
                break;
        }
    else    
        switch (type) {
            case BIN:
                generateFileList(fileList, dir, "bin",   "*.exe *.bat");
                generateFileList(fileList, dir, "bin",   "*.dll", "*d.dll");
                generateFileList(fileList, dir, "lib",   "*.dll", "*d.dll");
                generateFileList(fileList, dir, "share", "*.*");
                generateFileList(fileList, dir, "data",  "*.*");
                generateFileList(fileList, dir, "etc",   "*.*");
                return true;
            case LIB:
                generateFileList(fileList, dir, "bin",     "*d.dll");
                generateFileList(fileList, dir, "lib",     "*d.dll");
                generateFileList(fileList, dir, "lib",     "*.lib");    // msvc libs (static & import libs)
                generateFileList(fileList, dir, "lib",     "*.a");      // gcc (static) libs
                generateFileList(fileList, dir, "include", "*.*");
                return true;
            case DOC:
                generateFileList(fileList, dir, "doc", "*.*");
                generateFileList(fileList, dir, "man", "*.*");
                return true;
            case SRC:
				exclude = m_srcExcludes + " .svn CVS";
                // * and not *.* because *.* does not find "foo" (filename without extension) - Qt-bug?
                generateFileList(fileList, m_srcRoot.isEmpty() ? dir : m_srcRoot, "src", "*",exclude);
                return true;
            case NONE:
                generateFileList(fileList, dir, "", "*.*", "manifest");
                return true;
            default:
                break;
       }
   return false;
}

bool Packager::createManifestFiles(const QString &rootDir, QStringList &fileList, Packager::Type type, QList<MemFile> &manifestFiles)
{
    QString fileNameBase = getBaseName(type); 
    QString descr;
    MemFile mf;
    
    manifestFiles.clear();

    switch(type) {
        case Packager::BIN:
            descr = "Binaries";
            break;
        case Packager::LIB:
            descr = "developer files";
            break;
        case Packager::DOC:
            descr = "documentation";
            break;
        case Packager::SRC:
            descr = "source code";
            break;
        case Packager::NONE:
            descr = "complete package";
            break;
        default:
            break;
    } 

    QBuffer b(&mf.data);
    b.open(QIODevice::WriteOnly);
    QTextStream out(&b);
    QStringList::ConstIterator it = fileList.begin();
    QStringList::ConstIterator end = fileList.end();
    QStringList toRemove;
    for( ; it != end; ++it ) {
        QString fn = *it;
        QFile f(rootDir + '/' + fn);
        if(!f.open(QIODevice::ReadOnly)) {
            qWarning("Can't open %s - removing from filelist!", qPrintable(fn));
            toRemove += fn;
            continue;
        }
        QByteArray ba = f.readAll();    // mmmmh
        QString md5Hash = qtMD5(ba);
        QByteArray fnUtf8 = (*it).toUtf8();
        fnUtf8.replace(' ', "\\ "); // escape ' '
        out << fnUtf8 << ' ' << md5Hash << '\n';
    }
    out << '\n'
        << "manifest/" + fileNameBase + ".mft" << '\n'
        << "manifest/" + fileNameBase + ".ver" << '\n';
    b.close();
    mf.filename = "manifest/" + fileNameBase + ".mft";
    manifestFiles += mf;

    mf.data.clear();

    b.setBuffer(&mf.data);
    b.open(QIODevice::WriteOnly);
    out.setDevice(&b);
    out << m_name + ' ' + m_version + ' ' + descr << '\n'
        << m_name + ": " + m_notes + '\n';
    b.close();
    mf.filename = "manifest/" + fileNameBase + ".ver";
    manifestFiles += mf;
    
    return true;    
}

bool Packager::makePackage(const QString &root, const QString &destdir, bool bComplete)
{
    // generate file lists
    // create manifest files 
    // create zip file
    m_rootDir = root;
    QStringList fileList; 
    QList<MemFile> manifestFiles;
	QString _destdir = destdir;
	if (!destdir.isEmpty() && !destdir.endsWith("/") && !destdir.endsWith("\\"))
		_destdir += "/"; 
    if (m_verbose)
        qDebug() << "creating bin package" << destdir + getBaseName(Packager::BIN); 
    generatePackageFileList(fileList, Packager::BIN);
    createManifestFiles(m_rootDir, fileList, Packager::BIN, manifestFiles);    
    if (fileList.size() > 0)
        createZipFile(_destdir + getBaseName(Packager::BIN), m_rootDir, fileList, manifestFiles);
    else
        qDebug() << "no binary files found!";

    if (m_verbose)
        qDebug() << "creating lib package" << getBaseName(Packager::LIB); 
    generatePackageFileList(fileList, Packager::LIB);
    createManifestFiles(m_rootDir, fileList, Packager::LIB, manifestFiles);
    if (fileList.size() > 0)
        createZipFile(_destdir + getBaseName(Packager::LIB), m_rootDir, fileList, manifestFiles);

    if (m_verbose)
        qDebug() << "creating doc package" << getBaseName(Packager::DOC); 
    generatePackageFileList(fileList, Packager::DOC);
    createManifestFiles(m_rootDir, fileList, Packager::DOC, manifestFiles);
    if (fileList.size() > 0)
        createZipFile(_destdir + getBaseName(Packager::DOC), m_rootDir, fileList, manifestFiles);

    if (m_verbose)
        qDebug() << "creating src package" << getBaseName(Packager::SRC); 
    generatePackageFileList(fileList, Packager::SRC);
    QString s = m_srcRoot.isEmpty() ? m_rootDir : m_srcRoot;
    createManifestFiles(s, fileList, Packager::SRC, manifestFiles);
    if (fileList.size() > 0)
        createZipFile(_destdir + getBaseName(Packager::SRC), s, fileList, manifestFiles, "src/" + m_name + "-" + m_version + "/");

    if(bComplete) {
        if (m_verbose)
            qDebug() << "creating complete package" << getBaseName(Packager::NONE); 
        generatePackageFileList(fileList, Packager::NONE);
        createManifestFiles(m_rootDir, fileList, Packager::NONE, manifestFiles);
        if (fileList.size() > 0)
            createZipFile(_destdir + getBaseName(Packager::NONE), m_rootDir, fileList, manifestFiles);
    }
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

bool Packager::stripFiles(const QString &dir)
{
    QStringList fileList; 
    generateFileList(fileList,dir,"bin","*.exe *.dll","*d.dll *d4.dll");
    for (int i = 0; i < fileList.size(); i++) 
    {
        QFileInfo fi(dir + "/" + fileList.at(i));
        // FIXME: add file in use detection, isWritable() returns not the required state
        // if no windows related functions is available at least parsing the output for 
        // the string "strip: unable to rename" indicates this condition
#if 0
        if (!fi.isWritable())
        {
            qDebug() << "file " << fi.absoluteFilePath() << " is in use"; 
            exit(1);
        }
#endif
        qDebug() << "strip -s " + fi.absoluteFilePath(); 
        QProcess::execute("strip -s " + fi.absoluteFilePath());
    }
    return true;
}
// create debug files for mingw
//  see http://www.daemon-systems.org/man/strip.1.html

bool Packager::createDebugFiles(const QString &dir)
{
    QStringList fileList; 
    generateFileList(fileList,dir,"bin","*.exe *.dll");
    generateFileList(fileList,dir,"lib","*.exe *.dll");
    for (int i = 0; i < fileList.size(); i++) 
    {
        QFileInfo fi(dir + '/' + fileList.at(i));
        // FIXME: add file in use detection, isWritable() returns not the required state
        // if no windows related functions is available at least parsing the output for 
        // the string "strip: unable to rename" indicates this condition
#if 0
        if (!fi.isWritable())
        {
            qDebug() << "file " << fi.absoluteFilePath() << " is in use"; 
            exit(1);
        }
#endif
        QProcess::execute("objcopy --only-keep-debug " + fi.absoluteFilePath() + " " + fi.absoluteFilePath() + ".dbg");
        QProcess::execute("objcopy --strip-debug " + fi.absoluteFilePath());
        QProcess::execute("objcopy --add-gnu-debuglink=" + fi.absoluteFilePath() + ".dbg " + fi.absoluteFilePath());
    }
    return true;
}
