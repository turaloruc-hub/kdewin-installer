/****************************************************************************
**
** Copyright (C) 2008-2010 Ralf Habacker <ralf.habacker@freenet.de> 
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

#ifndef USERCOMPILERMODEPAGE_H
#define USERCOMPILERMODEPAGE_H

#include "installwizard.h"

#include "ui_usercompilermodepage.h"

class UserCompilerModePage : public InstallWizardPage
{
    Q_OBJECT

public:
    UserCompilerModePage();

    void initializePage();
    bool isComplete();
    bool validatePage();
    
protected slots:
    void slotModeButtonClicked(int id);

protected:
    void setCompilerMode(bool);
    Ui::UserCompilerModePage ui;
};

#endif
