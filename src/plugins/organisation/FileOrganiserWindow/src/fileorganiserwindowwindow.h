/*******************************************************************************

Licensed to the OpenCOR team under one or more contributor license agreements.
See the NOTICE.txt file distributed with this work for additional information
regarding copyright ownership. The OpenCOR team licenses this file to you under
the Apache License, Version 2.0 (the "License"); you may not use this file
except in compliance with the License. You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed
under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
CONDITIONS OF ANY KIND, either express or implied. See the License for the
specific language governing permissions and limitations under the License.

*******************************************************************************/

//==============================================================================
// File organiser window
//==============================================================================

#ifndef FILEORGANISERWINDOWWINDOW_H
#define FILEORGANISERWINDOWWINDOW_H

//==============================================================================

#include "organisationwidget.h"

//==============================================================================

#include <QAbstractItemModel>

//==============================================================================

namespace Ui {
    class FileOrganiserWindowWindow;
}

//==============================================================================

class QMenu;

//==============================================================================

namespace OpenCOR {
namespace FileOrganiserWindow {

//==============================================================================

class FileOrganiserWindowWidget;

//==============================================================================

class FileOrganiserWindowWindow : public Core::OrganisationWidget
{
    Q_OBJECT

public:
    explicit FileOrganiserWindowWindow(QWidget *pParent);
    ~FileOrganiserWindowWindow();

    virtual void retranslateUi();

    virtual void loadSettings(QSettings *pSettings);
    virtual void saveSettings(QSettings *pSettings) const;

private:
    Ui::FileOrganiserWindowWindow *mGui;

    FileOrganiserWindowWidget *mFileOrganiserWidget;

    QMenu *mContextMenu;

private Q_SLOTS:
    void on_actionNew_triggered();
    void on_actionDelete_triggered();

    void showCustomContextMenu(const QPoint &pPosition) const;
    void itemDoubleClicked(const QModelIndex &itemIndex);
};

//==============================================================================

}   // namespace FileOrganiserWindow
}   // namespace OpenCOR

//==============================================================================

#endif

//==============================================================================
// End of file
//==============================================================================