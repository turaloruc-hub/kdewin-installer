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

#include "downloadsettingspage.h"

#include <QFileDialog>
#include <QMessageBox>

DownloadSettingsPage::DownloadSettingsPage() : InstallWizardPage(0)
{
    ui.setupUi(this);
    setTitle(windowTitle());
    setSubTitle(statusTip());

    connect( ui.tempPathSelect,SIGNAL(clicked()),this,SLOT(tempPathSelectClicked()) );
}

void DownloadSettingsPage::initializePage()
{
    Settings &s = Settings::instance();
    ui.tempPathEdit->setText(s.downloadDir());
}

bool DownloadSettingsPage::validatePage()
{
    Settings &s = Settings::instance();
    QFileInfo fi(ui.tempPathEdit->text());
    if (!fi.isWritable())
    {
        QMessageBox::critical(this,
                              tr("Error"),
                              tr("You do not have write permissions on the selected download directory."),
                              QMessageBox::Ok);
        return false;
    }
    s.setDownloadDir(ui.tempPathEdit->text());
    return true;
}

bool DownloadSettingsPage::isComplete()
{
    return !ui.tempPathEdit->text().isEmpty();
}

void DownloadSettingsPage::tempPathSelectClicked()
{
    QString fileName = QFileDialog::getExistingDirectory(this,
                       tr("Select Package Download Directory"),
                       "",
                       QFileDialog::ShowDirsOnly| QFileDialog::DontResolveSymlinks);
    if(!fileName.isEmpty())
        ui.tempPathEdit->setText(QDir::toNativeSeparators(fileName));
}
