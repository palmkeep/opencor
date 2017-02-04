/*******************************************************************************

Copyright The University of Auckland

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

*******************************************************************************/

//==============================================================================
// PMR Workspaces window widget
//==============================================================================

#pragma once

//==============================================================================

#include "pmrworkspace.h"
#include "pmrworkspaceswindowitem.h"
#include "widget.h"

//==============================================================================

#include <QMap>
#include <QStandardItemModel>

//==============================================================================

class QMenu;

//==============================================================================

namespace OpenCOR {

//==============================================================================

namespace Core {
    class TreeViewWidget;
    class UserMessageWidget;
}   // namespace Core

//==============================================================================

namespace PMRSupport {
    class PmrWebService;
}   // namespace PMRSupport

//==============================================================================

namespace PMRWorkspacesWindow {

//==============================================================================

class PmrWorkspacesWindowWindow;

//==============================================================================

class PmrWorkspacesWindowWidget : public Core::Widget
{
    Q_OBJECT

public:
    explicit PmrWorkspacesWindowWidget(PMRSupport::PmrWebService *pPmrWebService,
                                       PmrWorkspacesWindowWindow *pParent);
    ~PmrWorkspacesWindowWidget();

    virtual void retranslateUi();

    virtual void loadSettings(QSettings *pSettings);
    virtual void saveSettings(QSettings *pSettings) const;

protected:
    virtual void keyPressEvent(QKeyEvent *pEvent);
    virtual QSize sizeHint() const;

private:
    PMRSupport::PmrWebService *mPmrWebService;

    QMap<QString, QString> mClonedWorkspaceFolderUrls;
    QMap<QString, QPair<QString, bool>> mWorkspaceUrlFoldersOwned;

    bool mInitialized;

    QString mErrorMessage;
    bool mAuthenticated;

    int mRow;

    QTimer *mTimer;

    QMenu *mContextMenu;

    QAction *mParentNewAction;
    QAction *mParentReloadAction;

    QAction *mNewAction;
    QAction *mViewInPmrAction;
    QAction *mViewOncomputerAction;
    QAction *mCopyUrlAction;
    QAction *mCopyPathAction;
    QAction *mCloneAction;
    QAction *mCommitAction;
    QAction *mPullAction;
    QAction *mPullAndPushAction;
    QAction *mStageAction;
    QAction *mUnstageAction;
    QAction *mReloadAction;
    QAction *mAboutAction;

    Core::UserMessageWidget *mUserMessageWidget;

    QStandardItemModel *mTreeViewModel;
    Core::TreeViewWidget *mTreeViewWidget;

    QIcon mCollapsedWorkspaceIcon;
    QIcon mExpandedWorkspaceIcon;

    QIcon mUnstagedCollapsedWorkspaceIcon;
    QIcon mUnstagedExpandedWorkspaceIcon;

    QIcon mConflictCollapsedWorkspaceIcon;
    QIcon mConflictExpandedWorkspaceIcon;

    QIcon mCollapsedOwnedWorkspaceIcon;
    QIcon mExpandedOwnedWorkspaceIcon;

    QIcon mUnstagedCollapsedOwnedWorkspaceIcon;
    QIcon mUnstagedExpandedOwnedWorkspaceIcon;

    QIcon mConflictCollapsedOwnedWorkspaceIcon;
    QIcon mConflictExpandedOwnedWorkspaceIcon;

    QIcon mFileIcon;

    QIcon mIaFileIcon;
    QIcon mIdFileIcon;
    QIcon mImFileIcon;
    QIcon mIrFileIcon;
    QIcon mItFileIcon;

    QIcon mWaFileIcon;
    QIcon mWcFileIcon;
    QIcon mWdFileIcon;
    QIcon mWeFileIcon;
    QIcon mWmFileIcon;
    QIcon mWqFileIcon;
    QIcon mWrFileIcon;
    QIcon mWtFileIcon;

    void updateGui();

    PmrWorkspacesWindowItem * currentItem() const;
    PmrWorkspacesWindowItem * workspaceItem(PMRSupport::PmrWorkspace *pWorkspace) const;

    void retrieveWorkspaceIcons(PMRSupport::PmrWorkspace *pWorkspace,
                                QIcon &pCollapsedIcon, QIcon &pExpandedIcon);

    PmrWorkspacesWindowItems retrieveItems(PmrWorkspacesWindowItem *pItem) const;
    void deleteItems(PmrWorkspacesWindowItem *pItem,
                     PmrWorkspacesWindowItems &pItems);

    void addWorkspace(PMRSupport::PmrWorkspace *pWorkspace);
    PmrWorkspacesWindowItems populateWorkspace(PMRSupport::PmrWorkspace *pWorkspace,
                                               PmrWorkspacesWindowItem *pFolderItem,
                                               PMRSupport::PmrWorkspaceFileNode *pFileNode);
    void refreshWorkspace(PMRSupport::PmrWorkspace *pWorkspace,
                          const bool &pSortAndResize = true);

    void duplicateCloneMessage(const QString &pUrl, const QString &pPath1,
                               const QString &pPath2);

    void stageUnstage(const bool &pStage);

    void sortAndResizeTreeViewToContents();

signals:
    void information(const QString &pMessage);
    void warning(const QString &pMessage);

    void openFileRequested(const QString &pFileName);
    void openFilesRequested(const QStringList &pFileNames);

public slots:
    void initialize(const PMRSupport::PmrWorkspaces &pWorkspaces,
                    const QString &pErrorMessage = QString(),
                    const bool &pAuthenticated = true);

private slots:
    void showCustomContextMenu(const QPoint &pPosition) const;
    void itemDoubleClicked(const QModelIndex &pIndex);
    void itemExpanded(const QModelIndex &pIndex);
    void itemCollapsed(const QModelIndex &pIndex);

    void resizeTreeViewToContents();

    void refreshWorkspaces(const PMRSupport::PmrWorkspaces &pWorkspaces = PMRSupport::PmrWorkspaces());

    void workspaceCloned(PMRSupport::PmrWorkspace *pWorkspace);
    void workspaceUncloned(PMRSupport::PmrWorkspace *pWorkspace);
    void workspaceSynchronized(PMRSupport::PmrWorkspace *pWorkspace);

    void viewInPmr();
    void viewOnComputer();
    void copyUrl();
    void copyPath();
    void clone();
    void commit();
    void pull();
    void pullAndPush();
    void stage();
    void unstage();
    void about();
};

//==============================================================================

}   // namespace PMRWorkspacesWindow
}   // namespace OpenCOR

//==============================================================================
// End of file
//==============================================================================