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

#include <QtDebug>
#include <QStringList>

#include "package.h"

#define DEBUG

// FIXME: this should be in PackageList to have access from Package, 
// Package class should store PackageList pointer as parent
QString Package::baseURL = "http://heanet.dl.sourceforge.net/sourceforge/gnuwin32/";

Package::Package()
{}

QString Package::getFileName(Package::Type contentType)
{
    QList<packageDescr>::iterator it = m_packages.begin();
    for( ; it != m_packages.end(); ++it) {
        if(contentType == (*it).contentType)
            return (*it).fileName;
    }
    return QString();
}

QString Package::getURL(Package::Type contentType)
{
    QList<packageDescr>::iterator it = m_packages.begin();
    for( ; it != m_packages.end(); ++it) {
        if(contentType == (*it).contentType)
            return (*it).path;
    }
    return QString();
}

void Package::add(const QString &path, const QByteArray &contentType, bool bInstalled)
{
#ifdef DEBUG
    qDebug() << __FUNCTION__ << " " << path << " " << contentType << " " << bInstalled;
#endif
    QByteArray ct = contentType.toLower();
    if(ct == "bin")
        add(path, BIN, bInstalled);
    else
    if(ct == "lib")
        add(path, LIB, bInstalled);
    else
    if(ct == "doc")
        add(path, DOC, bInstalled);
    else
    if(ct == "src")
        add(path, SRC, bInstalled);
}

void Package::add(const QString &path, Package::Type contentType, bool bInstalled)
{
    packageDescr desc;
    int idx;
#ifdef DEBUG
    qDebug() << __FUNCTION__ << " " << path << " " << contentType << " " << bInstalled;
#endif
    QList<packageDescr>::iterator it = m_packages.begin();
    desc.path = path;

    idx = path.lastIndexOf('/');
    if(idx == -1) {
        qDebug("Invalid");    // FIXME
        return;
    }
    desc.fileName = path.mid(idx + 1);

    idx = desc.fileName.lastIndexOf('.');
    if(idx == -1) {
        qDebug("Invalid");    // FIXME
    }
    desc.packageType = desc.fileName.mid(idx + 1);
    
    desc.contentType = contentType;
    desc.bInstalled  = bInstalled;
#ifdef DEBUG
    qDebug() << __FUNCTION__ << desc.contentType << desc.bInstalled;
#endif    
    for( ; it != m_packages.end(); ++it) {
        if(desc.contentType == (*it).contentType &&
           desc.packageType == (*it).packageType) {
            qDebug("Already added");
            return;
        }
    }

    m_packages.append(desc);
}

QString Package::toString(bool installed, const QString &delim)
{
    QString result = m_name + delim + m_version;
    QString availableTypes = getTypeAsString(installed);
    if (availableTypes != "" && !installed )
        result += "   ( found =" + getTypeAsString() + ")";
    else if (availableTypes != "" && installed )
        result += "   ( installed =" + getTypeAsString() + ")";
    return result;
}

QString Package::getTypeAsString(bool requiredIsInstalled, const QString &delim)
{
    QString types;

    QList<packageDescr>::iterator it = m_packages.begin();
    for( ; it != m_packages.end(); ++it) {
        if(requiredIsInstalled && !(*it).bInstalled)
            continue;

        switch((*it).contentType) {
            case BIN:
                types += "bin" + delim;
                break;
            case LIB:
                types += "lib" + delim;
                break;
            case DOC:
                types += "doc" + delim;
                break;
            case SRC:
                types += "src" + delim;
                break;
        };
    }
    return types;
}

bool Package::isInstalled(Package::Type contentType)
{
    QList<packageDescr>::iterator it = m_packages.begin();
    for( ; it != m_packages.end(); ++it) {
        if(contentType == (*it).contentType)
            return (*it).bInstalled;
    }
    return false;
}

void Package::setInstalled(Package::Type contentType)
{
    QList<packageDescr>::iterator it = m_packages.begin();
    for( ; it != m_packages.end(); ++it) {
        if(contentType == (*it).contentType)
            (*it).bInstalled = true;
    }
}

bool Package::write(QTextStream &out)
{
#ifdef DEBUG
    qDebug() << __FUNCTION__ << m_name << "\t" << m_version << "\t" << getTypeAsString(true,"\t") << "\n";
#endif
    out << m_name << "\t" << m_version 
// FIXME enable when read could parse this
#if 0
    << "\t" 
    << getFileName(BIN) << ":"
    << getFileName(LIB) << ":"
    << getFileName(DOC) << ":"
    << getFileName(SRC) << ":"
    << getTypeAsString(true," ") 
#endif
    << "\n";
    return true;
}

bool Package::read(QTextStream &in)
{
    QString line = in.readLine();
    while (line.startsWith("#")) {
        line = in.readLine();
    }
    if (in.atEnd())
        return false;
    QStringList parts = line.split("\t");
    setName(parts.at(0));
    setVersion(parts.at(1));
    // FIXME: this is a hack 
    add(baseURL+parts.at(0)+"-"+parts.at(1)+"-bin.zip",BIN);
    add(baseURL+parts.at(0)+"-"+parts.at(1)+"-lib.zip",LIB);
    add(baseURL+parts.at(0)+"-"+parts.at(1)+"-doc.zip",DOC);
    add(baseURL+parts.at(0)+"-"+parts.at(1)+"-src.zip",SRC);
    qDebug() << __FUNCTION__ << " " << toString() 
// FIXME: see write()
#if 0
    << "type found " << parts.at(2) 
#endif
    ;
    return true;
}

void Package::logOutput()
{}





