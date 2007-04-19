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

#include <QtDebug>
#include <QDir>
#include <QTreeWidget>
#include <QFlags>

#include "installerenginegui.h"

//#include "downloader.h"
//#include "installer.h"
//#include "installerprogress.h"
#include "downloaderprogress.h"
#include "package.h"
#include "packagelist.h"
#include "globalconfig.h"
//#include "database.h"


InstallerEngineGui::InstallerEngineGui(DownloaderProgress *progressBar,InstallerProgress *instProgressBar)
: InstallerEngine(progressBar,instProgressBar)
{
    m_installMode = Single;
}

int typeToColumn(Package::Type type)
{
    switch (type)
    {
        case Package::BIN : return 3;
        case Package::LIB : return 4;
        case Package::DOC : return 5;
        case Package::SRC : return 6;
        default: return 2;
    }    
}

Package::Type columnToType(int column)
{
    switch (column)
    {
        case 3: return Package::BIN;
        case 4: return Package::LIB; 
        case 5: return Package::DOC; 
        case 6: return Package::SRC; 
        case 2: return Package::ALL;
        default : return Package::NONE;
    }    
}

enum iconType {_install, _autoinstall,_keepinstalled, _update, _remove, _nothing, _disable}; 

void setIcon(QTreeWidgetItem &item, Package::Type type, iconType action)
{
    static QIcon *ai;
    static QIcon *ii;
    static QIcon *ki;
    static QIcon *ni;
    static QIcon *id;
    static QIcon *dl;
    static QIcon *up;
 
    if (!ii) 
    {
        ai = new QIcon(":/images/autoinstall.xpm");
        ii = new QIcon(":/images/install.xpm");
        ki = new QIcon(":/images/keepinstalled.xpm");
        ni = new QIcon(":/images/noinst.xpm");
        id = new QIcon(":/images/install_disabled.xpm");
        dl = new QIcon(":/images/del.xpm");
        up = new QIcon(":/images/update.xpm");
    }

    int column = typeToColumn(type);
    switch(action)
    {
        case _autoinstall: item.setIcon(column,*ai); return;
        case _install: item.setIcon(column,*ii); return;
        case _keepinstalled: item.setIcon(column,*ki); return;
        case _remove:  item.setIcon(column,*dl); return;
        case _update:  item.setIcon(column,*up); return;
        case _nothing: item.setIcon(column,*ni); return;
        case _disable: item.setIcon(column,*id); return;
    }
    // FIXME: does not work, don't know how to set the icon size 
    // item.icon(column).setIconSize(QSize(22,22));
}

enum actionType { _initial, _next, _deps, _sync}; 
enum stateType { _Install, _Update, _Remove, _Nothing}; 

static void setState(QTreeWidgetItem &item, const Package *pkg, int column, actionType action, int syncColumn=0)
{
    Package::Type type = columnToType(column);

    if(!pkg)
        return;
    if (type != Package::ALL && !pkg->hasType(type)) 
    {
        setIcon(item,type,_disable);
        return; 
    }

    switch (action)
    {
        case _initial: 
            if (pkg->isInstalled(type))
            {
                setIcon(item,type,_keepinstalled );
                item.setData(column,Qt::UserRole,_Nothing);
            }
            else
            {
                setIcon(item,type,_nothing);
                item.setData(column,Qt::UserRole,_Nothing);
            }
            break;
        
        // enter next state depending on current state 
        case _next: 
        {
            stateType state = (stateType)item.data(column,Qt::UserRole).toInt();

            switch(state)
            {
                case _Nothing:  
                    if (pkg->isInstalled(type) || 
                        (type == Package::ALL 
                         && (!pkg->hasType(Package::BIN) || pkg->hasType(Package::BIN) && pkg->isInstalled(Package::BIN)) 
                         && (!pkg->hasType(Package::LIB) || pkg->hasType(Package::LIB) && pkg->isInstalled(Package::LIB)) 
                         && (!pkg->hasType(Package::DOC) || pkg->hasType(Package::DOC) && pkg->isInstalled(Package::DOC))))
                    { 
                        setIcon(item,type,_remove);
                        item.setData(column,Qt::UserRole,_Remove);
                    }
                    else
                    {
                        setIcon(item,type,_install);
                        item.setData(column,Qt::UserRole,_Install);
                    }
                    break;

                case _Remove:  
                    if (pkg->isInstalled(type))
                    {
                        setIcon(item,type,_keepinstalled );
                        item.setData(column,Qt::UserRole,_Nothing);
                    }
                    else
                    {
                        item.setData(column,Qt::UserRole,_Nothing);
                        setIcon(item,type,_nothing);  
                    }
                    break;

                case _Install:  
                    if (pkg->isInstalled(type))
                    {
                        setIcon(item,type,_install);
                        item.setData(column,Qt::UserRole,_Install);
                    }
                    else
                    {
                        setIcon(item,type,_nothing);  
                        item.setData(column,Qt::UserRole,_Nothing);
                    }
                    break;
            }
            break;
        }
        // handle dependeny selecting 
        case _deps: 
        {
            if (pkg->isInstalled(type))
            {
            }
            else
            {
                // FIXME: should be _autoinstall, but then the main package is using this icon too 
                setIcon(item,type,_install);
                item.setData(column,Qt::UserRole,_Install);
            }
            break;
        }
        case _sync: 
            stateType state = (stateType)item.data(syncColumn,Qt::UserRole).toInt();
            switch(state) 
            {
                case _Install: 
                    if (!pkg->isInstalled(type))
                    {
                        setIcon(item,type,_install);
                        item.setData(column,Qt::UserRole,_Install);
                    }
                    break;
                case _Nothing: 
                    if (!pkg->isInstalled(type))
                    {
                        setIcon(item,type,_nothing);
                        item.setData(column,Qt::UserRole,_Nothing);
                    }
                    break;
                case _Remove:
                    if (pkg->isInstalled(type))
                    {
                        setIcon(item,type,_remove);
                        item.setData(column,Qt::UserRole,_Remove);
                    }
                    else 
                    {
                        setIcon(item,type,_nothing);
                        item.setData(column,Qt::UserRole,_Nothing);
                    }
                    break;
            }                       
    }        
}

bool isMarkedForInstall(QTreeWidgetItem &item,Package::Type type)
{
    int column = typeToColumn(type);
    stateType state = (stateType)item.data(column,Qt::UserRole).toInt();
    return state == _Install;
}

bool isMarkedForRemoval(QTreeWidgetItem &item,Package::Type type)
{
    int column = typeToColumn(type);
    stateType state = (stateType)item.data(column,Qt::UserRole).toInt();
    return state == _Remove;
}

void InstallerEngineGui::setPageSelectorWidgetData(QTreeWidget *tree)
{
    tree->clear();
    QStringList labels;
    QList<QTreeWidgetItem *> items;
    // QTreeWidgetItem *item;
    QString allToolTip("select this checkbox to install/remove/update binaries, headers, import libraries and docs of this package");
    QString binToolTip("select this checkbox to install/remove/update the binaries of this package");
    QString libToolTip("select this checkbox to install/remove/update header and import libraries of this package");
    QString docToolTip("select this checkbox to install/remove/update the documentation of this package");
    QString srcToolTip("select this checkbox to install/remove/update the source of this package");

    labels
    << tr("Package")
    << tr("Version");
    switch (m_installMode) 
    {
        case Developer: 
            labels
            << ""
            << tr("bin/lib/doc")
            << ""
            << "";
            break;

        case EndUser: 
            labels
            << ""
            << tr("bin/doc")
            << ""
            << "";
            break;

        case Single: 
            labels
            << tr("all")
            << tr("bin")
            << tr("lib")
            << tr("doc");
            break;
    }
    labels
    << tr("src")
    << tr("package notes")
    << tr("news");

    tree->setColumnCount(9);
    tree->setHeaderLabels(labels);
    // see http://lists.trolltech.com/qt-interest/2006-06/thread00441-0.html
    // and Task Tracker Entry 106731
    //tree->setAlignment(Center);

    // adding top level items
    QList<QTreeWidgetItem *> categoryList;

#ifdef DEBUG
    qDebug() << "adding categories size:" << m_globalConfig->sites()->size();
#endif

    QList <PackageList *>::ConstIterator k = m_packageListList.constBegin();
    for ( ; k != m_packageListList.constEnd(); ++k)
    {
        if ((*k)->packageList().size() == 0)
            continue;
        QStringList names;
        names << (*k)->Name();
        names << "";
        names << "";
        names << "";
        names << "";
        names << "";
        names << "";
        names << (*k)->notes();
        QTreeWidgetItem *category = new QTreeWidgetItem((QTreeWidget*)0, names);
        category->setToolTip(0,(*k)->notes());
        categoryList.append(category);

        // adding sub items
        QList<Package*>::ConstIterator i = (*k)->packageList().constBegin();
        for ( ; i != (*k)->packageList().constEnd(); ++i)
        {
            Package *pkg = *i;
            QStringList data;
            data << pkg->name()
                 << pkg->version()
                 << QString();
            QTreeWidgetItem *item = new QTreeWidgetItem(category, data);
            if (m_installMode == Single)
                setState(*item,pkg,2,_initial);

            setState(*item,pkg,3,_initial);
            if (m_installMode != Developer )
            {
                setState(*item,pkg,4,_initial);
                setState(*item,pkg,5,_initial);
            }
            setState(*item,pkg,6,_initial);
            item->setText(7, pkg->notes());
            // FIXME 
            //item->setText(8, m_globalConfig->news()->value(pkg->name()+"-"+pkg->version()));
            item->setToolTip ( 2, allToolTip);
            item->setToolTip ( 3, binToolTip);
            item->setToolTip ( 4, libToolTip);
            item->setToolTip ( 5, docToolTip);
            item->setToolTip ( 6, srcToolTip);
        }
    }
    tree->insertTopLevelItems(0,categoryList);
    tree->expandAll();
    tree->sortItems(0,Qt::AscendingOrder);
}
extern QTreeWidget *tree;

void InstallerEngineGui::itemClickedPackageSelectorPage(QTreeWidgetItem *item, int column)
{
    if (column < 2)
        return;

    Package *pkg = getPackageByName(item->text(0),item->text(1));
    if (!pkg)
        return;
    if (m_installMode == Single && column == 2)
    {
       setState(*item,pkg,2,_next);
       setState(*item,pkg,3,_sync,2);
       setState(*item,pkg,4,_sync,2);
       setState(*item,pkg,5,_sync,2);
       setState(*item,pkg,6,_sync,2);
    }
    else if (m_installMode == Developer && column == 3)
    {
        setState(*item,pkg,column,_next);
        setState(*item,pkg,column+1,_next);
        setState(*item,pkg,column+1,_next);
    }
    else if (m_installMode == EndUser && column == 3)
    {
        setState(*item,pkg,column,_next);
        // lib excluded
        setState(*item,pkg,column+2,_next);
    }
    else
        setState(*item,pkg,column,_next);

    // select depending packages in case all or bin is selected

    if (column == 2 || column == 3) 
    {
        const QStringList &deps = pkg->deps();

        qDebug() << deps.join(" ");    
        for (int i = 0; i < deps.size(); ++i)
        {  
            QString dep = deps.at(i);
            QList<QTreeWidgetItem *> items = tree->findItems(deps.at(i),Qt::MatchFixedString | Qt::MatchRecursive);
            qDebug() << items.size();
            for (int j = 0; j < items.size(); ++j) 
            {
                   qDebug() << items.at(j);
                QTreeWidgetItem * depItem = static_cast<QTreeWidgetItem*>(items[j]);
                /// the dependency is only for bin package and one way to switch on
                Package *depPkg = getPackageByName(dep);

                if (m_installMode == Developer)
                {
                    setState(*depItem,depPkg,3,_deps);
                    setState(*depItem,depPkg,4,_deps);
                    setState(*depItem,depPkg,5,_deps);
                }
                else if (m_installMode == EndUser)
                {
                    setState(*depItem,depPkg,3,_deps);
                    // lib is excluded
                    setState(*depItem,depPkg,5,_deps);
                }
                else if (m_installMode == Single)
                {    
                    if (column == 2)
                    {
                        setState(*depItem,depPkg,3,_deps);
                        setState(*depItem,depPkg,4,_deps);
                        setState(*depItem,depPkg,5,_deps);
                    }
                    else
                        setState(*depItem,depPkg,column,_deps);
                }
            }
        }    
    }

}

bool InstallerEngineGui::downloadPackages(QTreeWidget *tree, const QString &category)
{
    for (int i = 0; i < tree->topLevelItemCount(); i++)
    {
        QTreeWidgetItem *item = static_cast<QTreeWidgetItem*>(tree->topLevelItem(i));
#ifdef DEBUG
        qDebug() << __FUNCTION__ << " " << item->text(0);
#endif
        if (category.isEmpty() || item->text(0) == category)
        {
            PackageList *packageList = getPackageListByName(item->text(0));
            if (!packageList)
            {
                qDebug() << __FUNCTION__ << " packagelist for " << item->text(0) << " not found";
                continue;
            }
            for (int j = 0; j < item->childCount(); j++)
            {
                QTreeWidgetItem *child = static_cast<QTreeWidgetItem*>(item->child(j));
//              qDebug("%s %s %d",child->text(0).toAscii().data(),child->text(1).toAscii().data(),child->checkState(2));
                bool all = child->checkState(2) == Qt::Checked;
                Package *pkg = packageList->getPackage(child->text(0),child->text(1).toAscii());
                if (!pkg)
                    continue;
                if (all | isMarkedForInstall(*child,Package::BIN))
                    pkg->downloadItem(m_downloader, Package::BIN);
                if (all | isMarkedForInstall(*child,Package::LIB))
                    pkg->downloadItem(m_downloader, Package::LIB);
                if (all | isMarkedForInstall(*child,Package::DOC))
                    pkg->downloadItem(m_downloader, Package::DOC);
                if (all | isMarkedForInstall(*child,Package::SRC))
                    pkg->downloadItem(m_downloader, Package::SRC);
            }
        }
    }
    return true;
}

bool InstallerEngineGui::removePackages(QTreeWidget *tree, const QString &category)
{
    for (int i = 0; i < tree->topLevelItemCount(); i++)
    {
        QTreeWidgetItem *item = static_cast<QTreeWidgetItem*>(tree->topLevelItem(i));
        if (category.isEmpty() || item->text(0) == category)
        {
            PackageList *packageList = getPackageListByName(item->text(0));
            if (!packageList)
            {
                qDebug() << __FUNCTION__ << " packagelist for " << item->text(0) << " not found";
                continue;
            }
            for (int j = 0; j < item->childCount(); j++)
            {
                QTreeWidgetItem *child = static_cast<QTreeWidgetItem*>(item->child(j));
                bool all = false; //child->checkState(2) == Qt::Checked;
                Package *pkg = packageList->getPackage(child->text(0),child->text(1).toAscii());
                if (!pkg)
                    continue;
                if (all | isMarkedForRemoval(*child,Package::BIN))
                    pkg->removeItem(m_installer, Package::BIN);
                if (all | isMarkedForRemoval(*child,Package::LIB))
                    pkg->removeItem(m_installer, Package::LIB);
                if (all | isMarkedForRemoval(*child,Package::DOC))
                    pkg->removeItem(m_installer, Package::DOC);
                if (all | isMarkedForRemoval(*child,Package::SRC))
                    pkg->removeItem(m_installer, Package::SRC);
            }
        }
    }
    return true;
}

bool InstallerEngineGui::installPackages(QTreeWidget *tree,const QString &_category)
{
    for (int i = 0; i < tree->topLevelItemCount(); i++)
    {
        QTreeWidgetItem *item = static_cast<QTreeWidgetItem*>(tree->topLevelItem(i));
        QString category = item->text(0); 
#ifdef DEBUG
        qDebug() << __FUNCTION__ << " " << category;
#endif
        if (_category.isEmpty() || category == _category)
        {
            PackageList *packageList = getPackageListByName(category);
            if (!packageList)
            {
                qDebug() << __FUNCTION__ << " packagelist for " << category << " not found";
                continue;
            }
            for (int j = 0; j < item->childCount(); j++)
            {
                QTreeWidgetItem *child = static_cast<QTreeWidgetItem*>(item->child(j));
//                qDebug("%s %s %d",child->text(0).toAscii().data(),child->text(1).toAscii().data(),child->checkState(2));
                bool all = child->checkState(2) == Qt::Checked;
                QString pkgName = child->text(0);
                Package *pkg = packageList->getPackage(pkgName,child->text(1).toAscii());
                if (!pkg)
                    continue;
                if (all | isMarkedForInstall(*child,Package::BIN))
                    pkg->installItem(m_installer, Package::BIN);
                if (all | isMarkedForInstall(*child,Package::LIB))
                    pkg->installItem(m_installer, Package::LIB);
                if (all | isMarkedForInstall(*child,Package::DOC))
                    pkg->installItem(m_installer, Package::DOC);
                if (all | isMarkedForInstall(*child,Package::SRC))
                    pkg->installItem(m_installer, Package::SRC);
            }
        }
    }
    return true;
}

void InstallerEngineGui::modeSwitchClicked(int mode)
{
}
