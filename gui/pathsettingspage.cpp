/****************************************************************************
**
** Copyright (C) 2008 Ralf Habacker <ralf.habacker@freenet.de> 
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

#include <QDir>
#include <QString>
#include <QFileDialog>

#include "database.h"
#include "pathsettingspage.h"

PathSettingsPage::PathSettingsPage() : InstallWizardPage(0)
{
    ui.setupUi(this);
    setTitle(windowTitle());
    setSubTitle(statusTip());

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(statusLabel,1,Qt::AlignBottom);
    setLayout(layout);
    connect( ui.rootPathSelect,SIGNAL(clicked()),this,SLOT(rootPathSelectClicked()) );
}

void PathSettingsPage::initializePage()
{
    Settings &s = Settings::getInstance();
    ui.createStartMenuEntries->setEnabled(false);
    ui.installModeEndUser->setChecked(!s.isDeveloperMode() ? Qt::Checked : Qt::Unchecked);
    ui.installModeDeveloper->setChecked(s.isDeveloperMode() ? Qt::Checked : Qt::Unchecked);
    ui.rootPathEdit->setText(QDir::convertSeparators(s.installDir()));
//    ui.tempPathEdit->setText(QDir::convertSeparators(s.downloadDir()));

    // logical grouping isn't available in the designer yet :-P
    QButtonGroup *groupA = new QButtonGroup(this);
    groupA->addButton(ui.compilerUnspecified);
    groupA->addButton(ui.compilerMinGW);
    groupA->addButton(ui.compilerMSVC);

    if (Database::isAnyPackageInstalled(s.installDir()))
    {
        ui.compilerUnspecified->setEnabled(false);
        ui.compilerMinGW->setEnabled(false);
        ui.compilerMSVC->setEnabled(false);
    }
    else
    {
        ui.compilerUnspecified->setEnabled(true);
        ui.compilerMinGW->setEnabled(true);
        ui.compilerMSVC->setEnabled(true);
    }

    switch (s.compilerType()) {
        case Settings::unspecified: ui.compilerUnspecified->setChecked(true); break;
        case Settings::MinGW: ui.compilerMinGW->setChecked(true); break;
        case Settings::MSVC: ui.compilerMSVC->setChecked(true); break;
        default: ui.compilerMinGW->setChecked(true); break;
    }

    QButtonGroup *groupB = new QButtonGroup(this);
    groupB->addButton(ui.installModeDeveloper);
    groupB->addButton(ui.installModeEndUser);

    if (Database::isAnyPackageInstalled(s.installDir()))
    {
        ui.installModeDeveloper->setEnabled(false);
        ui.installModeEndUser->setEnabled(false);
    }
    else
    {
        ui.installModeDeveloper->setEnabled(true);
        ui.installModeEndUser->setEnabled(true);
    }

    if (s.isDeveloperMode())
        ui.installModeDeveloper->setChecked(true);
    else
        ui.installModeEndUser->setChecked(true);

    setPixmap(QWizard::WatermarkPixmap, QPixmap());
}

bool PathSettingsPage::isComplete()
{
    return !ui.rootPathEdit->text().isEmpty();
}

int PathSettingsPage::nextId() const
{
    return InstallWizard::internetSettingsPage;
}

bool PathSettingsPage::validatePage()
{
    Settings &s = Settings::getInstance();
    s.setCreateStartMenuEntries(ui.createStartMenuEntries->checkState() == Qt::Checked ? true : false);
    s.setInstallDir(ui.rootPathEdit->text());

    s.setDeveloperMode(ui.installModeDeveloper->isChecked());

    if (ui.compilerUnspecified->isChecked())
        s.setCompilerType(Settings::unspecified);
    if (ui.compilerMinGW->isChecked())
        s.setCompilerType(Settings::MinGW);
    if (ui.compilerMSVC->isChecked())
        s.setCompilerType(Settings::MSVC);
    return true;
}

void PathSettingsPage::rootPathSelectClicked()
{
    QString fileName = QFileDialog::getExistingDirectory(this,
                       tr("Select Root Installation Directory"),
                       "",
                       QFileDialog::ShowDirsOnly| QFileDialog::DontResolveSymlinks);
    if(!fileName.isEmpty())
        ui.rootPathEdit->setText(QDir::toNativeSeparators(fileName));
}


#include "pathsettingspage.moc"
