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

#include "corecliutils.h"
#include "coreguiutils.h"
#include "i18ninterface.h"
#include "pmrwebservice.h"
#include "pmrworkspacemanager.h"
#include "pmrworkspaceswindowcommitdialog.h"
#include "pmrworkspaceswindowwidget.h"
#include "pmrworkspaceswindowwindow.h"
#include "treeviewwidget.h"
#include "usermessagewidget.h"

//==============================================================================

#include "ui_pmrworkspaceswindowwindow.h"

//==============================================================================

#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QDir>
#include <QFileIconProvider>
#include <QKeyEvent>
#include <QLayout>
#include <QMainWindow>
#include <QMenu>
#include <QSettings>
#include <QTimer>

//==============================================================================

#include "git2/remote.h"
#include "git2/repository.h"

//==============================================================================

namespace OpenCOR {
namespace PMRWorkspacesWindow {

//==============================================================================

PmrWorkspacesWindowWidget::PmrWorkspacesWindowWidget(PMRSupport::PmrWebService *pPmrWebService,
                                                     PmrWorkspacesWindowWindow *pParent) :
    Core::Widget(pParent),
    mPmrWebService(pPmrWebService),
    mClonedWorkspaceFolderUrls(QMap<QString, QString>()),
    mWorkspaceUrlFoldersOwned(QMap<QString, QPair<QString, bool>>()),
    mInitialized(false),
    mErrorMessage(QString()),
    mAuthenticated(false)
{
    // Create and customise some objects

    mUserMessageWidget = new Core::UserMessageWidget(this);

    mUserMessageWidget->setScale(0.85);

    mTreeViewModel = new QStandardItemModel(this);
    mTreeViewWidget = new Core::TreeViewWidget(this);

    mTreeViewWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    mTreeViewWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mTreeViewWidget->setHeaderHidden(true);
    mTreeViewWidget->setModel(mTreeViewModel);
    mTreeViewWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    mTreeViewWidget->setVisible(false);

    connect(mTreeViewWidget, SIGNAL(customContextMenuRequested(const QPoint &)),
            this, SLOT(showCustomContextMenu(const QPoint &)));

    connect(mTreeViewWidget, SIGNAL(doubleClicked(const QModelIndex &)),
            this, SLOT(itemDoubleClicked(const QModelIndex &)));

    connect(mTreeViewWidget, SIGNAL(expanded(const QModelIndex &)),
            this, SLOT(resizeTreeViewToContents()));
    connect(mTreeViewWidget, SIGNAL(expanded(const QModelIndex &)),
            this, SLOT(itemExpanded(const QModelIndex &)));

    connect(mTreeViewWidget, SIGNAL(collapsed(const QModelIndex &)),
            this, SLOT(resizeTreeViewToContents()));
    connect(mTreeViewWidget, SIGNAL(collapsed(const QModelIndex &)),
            this, SLOT(itemCollapsed(const QModelIndex &)));

    // Create our various non-owned workspace icons

    static const QIcon UnstagedIcon = QIcon(":/PMRWorkspacesWindow/wQ.png");
    static const QIcon ConflictIcon = QIcon(":/PMRWorkspacesWindow/wE.png");

    mCollapsedWorkspaceIcon = QApplication::style()->standardIcon(QStyle::SP_DirClosedIcon);
    mExpandedWorkspaceIcon = QApplication::style()->standardIcon(QStyle::SP_DirOpenIcon);

    int folderIconSize = mCollapsedWorkspaceIcon.availableSizes().first().width();
    int overlayIconSize = 0.57*folderIconSize;

    mUnstagedCollapsedWorkspaceIcon = Core::overlayedIcon(mCollapsedWorkspaceIcon, UnstagedIcon,
                                                          folderIconSize, folderIconSize,
                                                          0, 0, overlayIconSize, overlayIconSize);
    mUnstagedExpandedWorkspaceIcon = Core::overlayedIcon(mExpandedWorkspaceIcon, UnstagedIcon,
                                                         folderIconSize, folderIconSize,
                                                         0, 0, overlayIconSize, overlayIconSize);

    mConflictCollapsedWorkspaceIcon = Core::overlayedIcon(mCollapsedWorkspaceIcon, ConflictIcon,
                                                          folderIconSize, folderIconSize,
                                                          0, 0, overlayIconSize, overlayIconSize);
    mConflictExpandedWorkspaceIcon = Core::overlayedIcon(mExpandedWorkspaceIcon, ConflictIcon,
                                                         folderIconSize, folderIconSize,
                                                         0, 0, overlayIconSize, overlayIconSize);

    // Create our various owned workspace icons

    static const QIcon FavoriteIcon = QIcon(":/oxygen/places/favorites.png");

    int overlayIconPos = folderIconSize-overlayIconSize;

    mCollapsedOwnedWorkspaceIcon = Core::overlayedIcon(mCollapsedWorkspaceIcon, FavoriteIcon,
                                                       folderIconSize, folderIconSize,
                                                       overlayIconPos, overlayIconPos,
                                                       overlayIconSize, overlayIconSize);
    mExpandedOwnedWorkspaceIcon = Core::overlayedIcon(mExpandedWorkspaceIcon, FavoriteIcon,
                                                      folderIconSize, folderIconSize,
                                                      overlayIconPos, overlayIconPos,
                                                      overlayIconSize, overlayIconSize);

    mUnstagedCollapsedOwnedWorkspaceIcon = Core::overlayedIcon(mCollapsedOwnedWorkspaceIcon, UnstagedIcon,
                                                               folderIconSize, folderIconSize,
                                                               0, 0, overlayIconSize, overlayIconSize);
    mUnstagedExpandedOwnedWorkspaceIcon = Core::overlayedIcon(mExpandedOwnedWorkspaceIcon, UnstagedIcon,
                                                              folderIconSize, folderIconSize,
                                                              0, 0, overlayIconSize, overlayIconSize);

    mConflictCollapsedOwnedWorkspaceIcon = Core::overlayedIcon(mCollapsedOwnedWorkspaceIcon, ConflictIcon,
                                                               folderIconSize, folderIconSize,
                                                               0, 0, overlayIconSize, overlayIconSize);
    mConflictExpandedOwnedWorkspaceIcon = Core::overlayedIcon(mExpandedOwnedWorkspaceIcon, ConflictIcon,
                                                              folderIconSize, folderIconSize,
                                                              0, 0, overlayIconSize, overlayIconSize);

    // Create our various file icons

    mFileIcon = QFileIconProvider().icon(QFileIconProvider::File);

    int fileIconSize = mFileIcon.availableSizes().first().width();

    overlayIconSize = 0.57*fileIconSize;

    mIaFileIcon = Core::overlayedIcon(mFileIcon, QIcon(":/PMRWorkspacesWindow/iA.png"),
                                      fileIconSize, fileIconSize,
                                      0, 0, overlayIconSize, overlayIconSize);
    mIdFileIcon = Core::overlayedIcon(mFileIcon, QIcon(":/PMRWorkspacesWindow/iD.png"),
                                      fileIconSize, fileIconSize,
                                      0, 0, overlayIconSize, overlayIconSize);
    mImFileIcon = Core::overlayedIcon(mFileIcon, QIcon(":/PMRWorkspacesWindow/iM.png"),
                                      fileIconSize, fileIconSize,
                                      0, 0, overlayIconSize, overlayIconSize);
    mIrFileIcon = Core::overlayedIcon(mFileIcon, QIcon(":/PMRWorkspacesWindow/iR.png"),
                                      fileIconSize, fileIconSize,
                                      0, 0, overlayIconSize, overlayIconSize);
    mItFileIcon = Core::overlayedIcon(mFileIcon, QIcon(":/PMRWorkspacesWindow/iT.png"),
                                      fileIconSize, fileIconSize,
                                      0, 0, overlayIconSize, overlayIconSize);

    mWaFileIcon = Core::overlayedIcon(mFileIcon, QIcon(":/PMRWorkspacesWindow/wA.png"),
                                      fileIconSize, fileIconSize,
                                      0, 0, overlayIconSize, overlayIconSize);
    mWcFileIcon = Core::overlayedIcon(mFileIcon, QIcon(":/PMRWorkspacesWindow/wC.png"),
                                      fileIconSize, fileIconSize,
                                      0, 0, overlayIconSize, overlayIconSize);
    mWdFileIcon = Core::overlayedIcon(mFileIcon, QIcon(":/PMRWorkspacesWindow/wD.png"),
                                      fileIconSize, fileIconSize,
                                      0, 0, overlayIconSize, overlayIconSize);
    mWeFileIcon = Core::overlayedIcon(mFileIcon, QIcon(":/PMRWorkspacesWindow/wE.png"),
                                      fileIconSize, fileIconSize,
                                      0, 0, overlayIconSize, overlayIconSize);
    mWmFileIcon = Core::overlayedIcon(mFileIcon, QIcon(":/PMRWorkspacesWindow/wM.png"),
                                      fileIconSize, fileIconSize,
                                      0, 0, overlayIconSize, overlayIconSize);
    mWqFileIcon = Core::overlayedIcon(mFileIcon, QIcon(":/PMRWorkspacesWindow/wQ.png"),
                                      fileIconSize, fileIconSize,
                                      0, 0, overlayIconSize, overlayIconSize);
    mWrFileIcon = Core::overlayedIcon(mFileIcon, QIcon(":/PMRWorkspacesWindow/wR.png"),
                                      fileIconSize, fileIconSize,
                                      0, 0, overlayIconSize, overlayIconSize);
    mWtFileIcon = Core::overlayedIcon(mFileIcon, QIcon(":/PMRWorkspacesWindow/wT.png"),
                                      fileIconSize, fileIconSize,
                                      0, 0, overlayIconSize, overlayIconSize);

    // Populate ourselves

    createLayout();

    layout()->addWidget(mUserMessageWidget);
    layout()->addWidget(mTreeViewWidget);

    // Connection to handle a response from our workspace manager

    PMRSupport::PmrWorkspaceManager *workspaceManager = PMRSupport::PmrWorkspaceManager::instance();

    connect(workspaceManager, SIGNAL(workspaceCloned(PMRSupport::PmrWorkspace *)),
            this, SLOT(workspaceCloned(PMRSupport::PmrWorkspace *)));
    connect(workspaceManager, SIGNAL(workspaceUncloned(PMRSupport::PmrWorkspace *)),
            this, SLOT(workspaceUncloned(PMRSupport::PmrWorkspace *)));
    connect(workspaceManager, SIGNAL(workspaceSynchronized(PMRSupport::PmrWorkspace *)),
            this, SLOT(workspaceSynchronized(PMRSupport::PmrWorkspace *)));

    // Create and start a timer for refreshing our workspaces

    mTimer = new QTimer(this);

    connect(mTimer, SIGNAL(timeout()),
            this, SLOT(refreshWorkspaces()));

    mTimer->start(1000);

    // Create and populate our context menu

    static const QIcon PullIcon = QIcon(":/PMRWorkspacesWindow/pull.png");

    mContextMenu = new QMenu(this);

    mParentNewAction = pParent->gui()->actionNew;
    mParentReloadAction = pParent->gui()->actionReload;

    mNewAction = Core::newAction(mParentNewAction->icon(),
                                 this);
    mViewInPmrAction = Core::newAction(QIcon(":/oxygen/categories/applications-internet.png"),
                                       this);
    mViewOncomputerAction = Core::newAction(QIcon(":/oxygen/devices/computer.png"),
                                            this);
    mCopyUrlAction = Core::newAction(QIcon(":/oxygen/actions/edit-copy.png"),
                                     this);
    mCopyPathAction = Core::newAction(QIcon(":/oxygen/actions/edit-copy.png"),
                                      this);
    mCloneAction = Core::newAction(Core::overlayedIcon(mCollapsedWorkspaceIcon, PullIcon,
                                                       folderIconSize, folderIconSize,
                                                       overlayIconPos, overlayIconPos,
                                                       overlayIconSize, overlayIconSize),
                                   this);
    mCommitAction = Core::newAction(QIcon(":/oxygen/actions/view-task.png"),
                                    this);
    mPullAction = Core::newAction(PullIcon,
                                  this);
    mPullAndPushAction = Core::newAction(QIcon(":/PMRWorkspacesWindow/pullAndPush.png"),
                                         this);
    mStageAction = Core::newAction(QIcon(":/oxygen/actions/dialog-ok-apply.png"),
                                   this);
    mUnstageAction = Core::newAction(QIcon(":/oxygen/actions/dialog-cancel.png"),
                                     this);
    mReloadAction = Core::newAction(mParentReloadAction->icon(),
                                    this);
    mAboutAction = Core::newAction(QIcon(":/oxygen/actions/help-about.png"),
                                   this);

    connect(mNewAction, SIGNAL(triggered(bool)),
            mParentNewAction, SIGNAL(triggered(bool)));
    connect(mViewInPmrAction, SIGNAL(triggered(bool)),
            this, SLOT(viewInPmr()));
    connect(mViewOncomputerAction, SIGNAL(triggered(bool)),
            this, SLOT(viewOnComputer()));
    connect(mCopyUrlAction, SIGNAL(triggered(bool)),
            this, SLOT(copyUrl()));
    connect(mCopyPathAction, SIGNAL(triggered(bool)),
            this, SLOT(copyPath()));
    connect(mCloneAction, SIGNAL(triggered(bool)),
            this, SLOT(clone()));
    connect(mCommitAction, SIGNAL(triggered(bool)),
            this, SLOT(commit()));
    connect(mPullAction, SIGNAL(triggered(bool)),
            this, SLOT(pull()));
    connect(mPullAndPushAction, SIGNAL(triggered(bool)),
            this, SLOT(pullAndPush()));
    connect(mStageAction, SIGNAL(triggered(bool)),
            this, SLOT(stage()));
    connect(mUnstageAction, SIGNAL(triggered(bool)),
            this, SLOT(unstage()));
    connect(mReloadAction, SIGNAL(triggered(bool)),
            mParentReloadAction, SIGNAL(triggered(bool)));
    connect(mAboutAction, SIGNAL(triggered(bool)),
            this, SLOT(about()));

    mContextMenu->addAction(mNewAction);
    mContextMenu->addSeparator();
    mContextMenu->addAction(mViewInPmrAction);
    mContextMenu->addAction(mViewOncomputerAction);
    mContextMenu->addSeparator();
    mContextMenu->addAction(mCopyUrlAction);
    mContextMenu->addAction(mCopyPathAction);
    mContextMenu->addSeparator();
    mContextMenu->addAction(mCloneAction);
    mContextMenu->addSeparator();
    mContextMenu->addAction(mCommitAction);
    mContextMenu->addSeparator();
    mContextMenu->addAction(mPullAction);
    mContextMenu->addAction(mPullAndPushAction);
    mContextMenu->addSeparator();
    mContextMenu->addAction(mStageAction);
    mContextMenu->addAction(mUnstageAction);
    mContextMenu->addSeparator();
    mContextMenu->addAction(mReloadAction);
    mContextMenu->addSeparator();
    mContextMenu->addAction(mAboutAction);

    // Make our tree view widget our focus proxy

    setFocusProxy(mTreeViewWidget);
}

//==============================================================================

PmrWorkspacesWindowWidget::~PmrWorkspacesWindowWidget()
{
    // Delete some internal objects

    delete mTimer;
}

//==============================================================================

void PmrWorkspacesWindowWidget::retranslateUi()
{
    // Retranslate our actions
    // Note: the stage and unstage actions are translated prior to showing the
    //       context menu since they can be used for one or several files...

    I18nInterface::retranslateAction(mNewAction, mParentNewAction->text(),
                                     mParentNewAction->statusTip());
    I18nInterface::retranslateAction(mViewInPmrAction, tr("View In PMR"),
                                     tr("View in PMR"));
    I18nInterface::retranslateAction(mViewOncomputerAction, tr("View On Computer"),
                                     tr("View on computer"));
    I18nInterface::retranslateAction(mCopyUrlAction, tr("Copy URL"),
                                     tr("Copy the URL to the clipboard"));
    I18nInterface::retranslateAction(mCopyPathAction, tr("Copy Path"),
                                     tr("Copy the path to the clipboard"));
    I18nInterface::retranslateAction(mCloneAction, tr("Clone..."),
                                     tr("Clone the current workspace"));
    I18nInterface::retranslateAction(mCommitAction, tr("Commit..."),
                                     tr("Commit staged changes"));
    I18nInterface::retranslateAction(mPullAction, tr("Pull"),
                                     tr("Pull changes from PMR"));
    I18nInterface::retranslateAction(mPullAndPushAction, tr("Pull And Push"),
                                     tr("Pull and push changes from/to PMR"));
    I18nInterface::retranslateAction(mReloadAction, mParentReloadAction->text(),
                                     mParentReloadAction->statusTip());
    I18nInterface::retranslateAction(mAboutAction, tr("About..."),
                                     tr("Some information about the current workspace"));

    // Retranslate the rest of our GUI by updating it, if we have been
    // initialised

    if (mInitialized)
        updateGui();
}

//==============================================================================

static const auto SettingsClonedWorkspaceFolders = QStringLiteral("ClonedWorkspaceFolders");

//==============================================================================

void PmrWorkspacesWindowWidget::loadSettings(QSettings *pSettings)
{
    // Retrieve and keep track of some information about the previously cloned
    // workspace folders

    foreach (const QString &clonedWorkspaceFolder,
             pSettings->value(SettingsClonedWorkspaceFolders).toStringList()) {
        // Retrieve the URL (i.e. remote.origin.url) of the cloned workspace
        // folder

        QString clonedWorkspaceUrl = QString();
        QByteArray folderByteArray = clonedWorkspaceFolder.toUtf8();
        git_repository *gitRepository = 0;

        if (!git_repository_open(&gitRepository, folderByteArray.constData())) {
            git_strarray remotes;

            if (!git_remote_list(&remotes, gitRepository)) {
                for (size_t i = 0; i < remotes.count; ++i) {
                    char *name = remotes.strings[i];

                    if (!strcmp(name, "origin")) {
                        git_remote *remote = 0;

                        if (!git_remote_lookup(&remote, gitRepository, name)) {
                            const char *remoteUrl = git_remote_url(remote);

                            if (remoteUrl)
                                clonedWorkspaceUrl = QString(remoteUrl);
                        }
                    }
                }
            }

            git_repository_free(gitRepository);
        }

        // Keep track of the URL of the cloned workspace folder, if there is one
        // and it's not already tracked

        if (!clonedWorkspaceUrl.isEmpty()) {
            if (mWorkspaceUrlFoldersOwned.contains(clonedWorkspaceUrl)) {
                duplicateCloneMessage(clonedWorkspaceUrl, mWorkspaceUrlFoldersOwned.value(clonedWorkspaceUrl).first, clonedWorkspaceFolder);
            } else {
                mClonedWorkspaceFolderUrls.insert(clonedWorkspaceFolder, clonedWorkspaceUrl);
                mWorkspaceUrlFoldersOwned.insert(clonedWorkspaceUrl, QPair<QString, bool>(clonedWorkspaceFolder, false));
            }
        }
    }
}

//==============================================================================

void PmrWorkspacesWindowWidget::saveSettings(QSettings *pSettings) const
{
    // Keep track of the names of folders containing cloned workspaces

    pSettings->setValue(SettingsClonedWorkspaceFolders, QVariant(mClonedWorkspaceFolderUrls.keys()));
}

//==============================================================================

void PmrWorkspacesWindowWidget::keyPressEvent(QKeyEvent *pEvent)
{
    // Default handling of the event

    Core::Widget::keyPressEvent(pEvent);

    // Retrieve all the files that are currently selected
    // Note: if there is a folder among the selected items, then ignore
    //       everything...

    QStringList fileNames = QStringList();
    QModelIndexList selectedItems = mTreeViewWidget->selectedIndexes();

    for (int i = 0, iMax = selectedItems.count(); i < iMax; ++i) {
        PmrWorkspacesWindowItem *item = static_cast<PmrWorkspacesWindowItem *>(mTreeViewModel->itemFromIndex(selectedItems[i]));

        if (item->type() == PmrWorkspacesWindowItem::File) {
            fileNames << item->fileName();
        } else {
            fileNames = QStringList();

            break;
        }
    }

    // Let people know about a key having been pressed with the view of opening
    // one or several files

    if (   fileNames.count()
        && ((pEvent->key() == Qt::Key_Enter) || (pEvent->key() == Qt::Key_Return))) {
        // There are some files that are selected and we want to open them, so
        // let people know about it

        emit openFilesRequested(fileNames);
    }
}

//==============================================================================

QSize PmrWorkspacesWindowWidget::sizeHint() const
{
    // Suggest a default size for our PMR workspaces widget
    // Note: this is critical if we want a docked widget, with a PMR workspaces
    //       widget on it, to have a decent size when docked to the main
    //       window...

    return defaultSize(0.15);
}

//==============================================================================

void PmrWorkspacesWindowWidget::updateGui()
{
    // Determine the message to be displayed, if any

    if (!mAuthenticated) {
        mUserMessageWidget->setIconMessage(":/oxygen/actions/help-hint.png",
                                           tr("Authenticate yourself..."),
                                           tr("Click on the top-right button."));
    } else if (mErrorMessage.isEmpty()) {
        if (!PMRSupport::PmrWorkspaceManager::instance()->count()) {
            mUserMessageWidget->setIconMessage(":/oxygen/actions/help-about.png",
                                               tr("No workspaces were found..."));
        } else {
            mUserMessageWidget->resetMessage();
        }
    } else {
        mUserMessageWidget->setIconMessage(":/oxygen/emblems/emblem-important.png",
                                           Core::formatMessage(mErrorMessage, false, true));
    }

    // Show/hide our user message widget and our tree view widget

    mUserMessageWidget->setVisible(!mUserMessageWidget->text().isEmpty());
    mTreeViewWidget->setVisible(mUserMessageWidget->text().isEmpty());
}

//==============================================================================

void PmrWorkspacesWindowWidget::initialize(const PMRSupport::PmrWorkspaces &pWorkspaces,
                                           const QString &pErrorMessage,
                                           const bool &pAuthenticated)
{
    // Initialise / keep track of some properties

    PMRSupport::PmrWorkspaceManager *workspaceManager = PMRSupport::PmrWorkspaceManager::instance();

    workspaceManager->clearWorkspaces();

    mErrorMessage = pErrorMessage;
    mAuthenticated = pAuthenticated;

    if (pErrorMessage.isEmpty() && pAuthenticated) {
        // Reconcile the URLs of my-workspaces (on PMR) with those from our
        // workspace folders (in doing so, folders/URLs that don't correspond to
        // an actual PMR workspace are pruned from the relevant maps)

        // First, clear the owned flag from the list of URLs with workspace
        // folders

        QMutableMapIterator<QString, QPair<QString, bool>> urlsIterator(mWorkspaceUrlFoldersOwned);

        while (urlsIterator.hasNext()) {
            urlsIterator.next();

            urlsIterator.setValue(QPair<QString, bool>(urlsIterator.value().first, false));
        }

        foreach (PMRSupport::PmrWorkspace *workspace, pWorkspaces) {
            // Remember our workspace, so we can find it by URL

            QString url = workspace->url();

            workspaceManager->addWorkspace(workspace);

            // Check if we know about its folder and, if so, flag it as ours

            if (mWorkspaceUrlFoldersOwned.contains(url)) {
                QString path = mWorkspaceUrlFoldersOwned.value(url).first;

                mWorkspaceUrlFoldersOwned.insert(url, QPair<QString, bool>(path, true));

                workspace->open(path);
            }
        }

        // Now check the workspace folders that aren't from my-workspaces (on
        // PMR)

        urlsIterator.toFront();

        while (urlsIterator.hasNext()) {
            urlsIterator.next();

            if (!urlsIterator.value().second) {
                QString url = urlsIterator.key();
                PMRSupport::PmrWorkspace *workspace = mPmrWebService->workspace(url);

                if (workspace) {
                    // The workspace is known, so ask our workspace manager to
                    // track it, and then open it

                    workspaceManager->addWorkspace(workspace);

                    workspace->open(urlsIterator.value().first);
                } else {
                    // The workspace is not known, so forget about it

                    mClonedWorkspaceFolderUrls.remove(urlsIterator.value().first);

                    urlsIterator.remove();
                }
            }
        }
    }

    // Populate our tree view widget with our different workspaces

    mTreeViewModel->clear();

    foreach (PMRSupport::PmrWorkspace *workspace, workspaceManager->workspaces())
        addWorkspace(workspace);

    updateGui();

    mInitialized = true;
}

//==============================================================================

void PmrWorkspacesWindowWidget::resizeTreeViewToContents()
{
    // Resize our tree view widget so that all of its contents is visible

    mTreeViewWidget->resizeColumnToContents(0);
}

//==============================================================================

PmrWorkspacesWindowItem * PmrWorkspacesWindowWidget::currentItem() const
{
    // Return our current item

    return static_cast<PmrWorkspacesWindowItem *>(mTreeViewModel->itemFromIndex(mTreeViewWidget->currentIndex()));
}

//==============================================================================

PmrWorkspacesWindowItem * PmrWorkspacesWindowWidget::workspaceItem(PMRSupport::PmrWorkspace *pWorkspace) const
{
    // Return the item, if any, corresponding to the given workspace

    for (int i = 0, iMax = mTreeViewModel->invisibleRootItem()->rowCount(); i < iMax; ++i) {
        PmrWorkspacesWindowItem *item = static_cast<PmrWorkspacesWindowItem *>(mTreeViewModel->invisibleRootItem()->child(i));

        if (item->workspace() == pWorkspace)
            return item;
    }

    return 0;
}

//==============================================================================

void PmrWorkspacesWindowWidget::retrieveWorkspaceIcons(PMRSupport::PmrWorkspace *pWorkspace,
                                                       QIcon &pCollapsedIcon,
                                                       QIcon &pExpandedIcon)
{
    // Retrieve the icons to use for the given workspace

    PMRSupport::PmrWorkspace::WorkspaceStatus workspaceStatus = pWorkspace->gitWorkspaceStatus();

    if (workspaceStatus & PMRSupport::PmrWorkspace::StatusUnstaged) {
        pCollapsedIcon = pWorkspace->isOwned()?mUnstagedCollapsedOwnedWorkspaceIcon:mUnstagedCollapsedWorkspaceIcon;
        pExpandedIcon = pWorkspace->isOwned()?mUnstagedExpandedOwnedWorkspaceIcon:mUnstagedExpandedWorkspaceIcon;
    } else if (workspaceStatus & PMRSupport::PmrWorkspace::StatusConflict) {
        pCollapsedIcon = pWorkspace->isOwned()?mConflictCollapsedOwnedWorkspaceIcon:mConflictCollapsedWorkspaceIcon;
        pExpandedIcon = pWorkspace->isOwned()?mConflictExpandedOwnedWorkspaceIcon:mConflictExpandedWorkspaceIcon;
    } else {
        pCollapsedIcon = pWorkspace->isOwned()?mCollapsedOwnedWorkspaceIcon:mCollapsedWorkspaceIcon;
        pExpandedIcon = pWorkspace->isOwned()?mExpandedOwnedWorkspaceIcon:mExpandedWorkspaceIcon;
    }
}

//==============================================================================

PmrWorkspacesWindowItems PmrWorkspacesWindowWidget::retrieveItems(PmrWorkspacesWindowItem *pItem) const
{
    // Return the child items of the given item

    PmrWorkspacesWindowItems res = PmrWorkspacesWindowItems();

    for (int i = 0, iMax = pItem->rowCount(); i < iMax; ++i) {
        PmrWorkspacesWindowItem *item = static_cast<PmrWorkspacesWindowItem *>(pItem->child(i));

        res << item;

        if (item->hasChildren())
            res << retrieveItems(item);
    }

    return res;
}

//==============================================================================

void PmrWorkspacesWindowWidget::deleteItems(PmrWorkspacesWindowItem *pItem,
                                            PmrWorkspacesWindowItems &pItems)
{
    // Recursively delete the given items, unless there is none left

    if (pItems.isEmpty())
        return;

    if (pItem->hasChildren()) {
        // Note: we delete the child items in reverse order since deletion is
        //       done using their row number...

        for (int i = pItem->rowCount()-1; i >= 0; --i)
            deleteItems(static_cast<PmrWorkspacesWindowItem *>(pItem->child(i)), pItems);

        // Remove the folder item, if it is now empty, or workspace, if it's not
        // owned and the folder where it was cloned has been deleted

        if (!pItem->hasChildren()) {
            if (pItem->parent())
                pItem->parent()->removeRow(pItem->row());
            else if (!pItem->workspace()->isOwned())
                mTreeViewModel->invisibleRootItem()->removeRow(pItem->row());

            pItems.removeOne(pItem);
        }
    } else if (pItems.contains(pItem)) {
        pItem->parent()->removeRow(pItem->row());

        pItems.removeOne(pItem);
    }
}

//==============================================================================

void PmrWorkspacesWindowWidget::addWorkspace(PMRSupport::PmrWorkspace *pWorkspace)
{
    // Add the given workspace to our tree view widget

    QIcon collapsedIcon;
    QIcon expandedIcon;

    retrieveWorkspaceIcons(pWorkspace, collapsedIcon, expandedIcon);

    PmrWorkspacesWindowItem *item = new PmrWorkspacesWindowItem(pWorkspace->isOwned()?
                                                                    PmrWorkspacesWindowItem::OwnedWorkspace:
                                                                    PmrWorkspacesWindowItem::Workspace,
                                                                pWorkspace,
                                                                collapsedIcon, expandedIcon);

    mTreeViewModel->invisibleRootItem()->appendRow(item);

    populateWorkspace(pWorkspace, item, pWorkspace->rootFileNode());

    // Make sure that everything is properly sorted and that all of the contents
    // of our tree view widget is visible

    sortAndResizeTreeViewToContents();
}

//==============================================================================

PmrWorkspacesWindowItems PmrWorkspacesWindowWidget::populateWorkspace(PMRSupport::PmrWorkspace *pWorkspace,
                                                                      PmrWorkspacesWindowItem *pFolderItem,
                                                                      PMRSupport::PmrWorkspaceFileNode *pFileNode)
{
    // Populate the given folder item with its children, which are referenced in
    // the given file node

    PmrWorkspacesWindowItems res = PmrWorkspacesWindowItems();

    foreach(PMRSupport::PmrWorkspaceFileNode *fileNode, pFileNode->children()) {
        // Check whether we already know about the file node

        PmrWorkspacesWindowItem *newItem = 0;

        for (int i = 0, iMax = pFolderItem->rowCount(); i < iMax; ++i) {
            PmrWorkspacesWindowItem *item = static_cast<PmrWorkspacesWindowItem *>(pFolderItem->child(i));

            if (item->fileNode() == fileNode) {
                newItem = item;

                break;
            }
        }

        // Add a new item for the file node or use the one that already exists
        // for it

        if (fileNode->hasChildren()) {
            PmrWorkspacesWindowItem *folderItem = newItem?
                                                      newItem:
                                                      new PmrWorkspacesWindowItem(PmrWorkspacesWindowItem::Folder,
                                                                                  pWorkspace,
                                                                                  fileNode,
                                                                                  mCollapsedWorkspaceIcon,
                                                                                  mExpandedWorkspaceIcon);

            if (!newItem)
                pFolderItem->appendRow(folderItem);

            res << folderItem
                << populateWorkspace(pWorkspace, folderItem, fileNode);
        } else {
            // We are dealing with a file, so retrieve its status and use the
            // corresponding icon for it, if needed
            // Note: it may happen (e.g. when deleting a folder) that the Git
            //       status is not up to date, hence we need to check for the
            //       I and W values not to be '\0' (which would be the case for
            //       a folder that doesn't have any files anymore)...

            PMRSupport::CharPair status = fileNode->status();

            if ((status.first == '\0') && (status.second == '\0'))
                continue;

            QIcon icon = mFileIcon;

            if (status.first != ' ') {
                if (status.first == 'A')
                    icon = mIaFileIcon;
                else if (status.first == 'D')
                    icon = mIdFileIcon;
                else if (status.first == 'M')
                    icon = mImFileIcon;
                else if (status.first == 'R')
                    icon = mIrFileIcon;
                else if (status.first == 'T')
                    icon = mItFileIcon;
            } else if (status.second != ' ') {
                if (status.second == 'A')
                    icon = mWaFileIcon;
                else if (status.second == 'C')
                    icon = mWcFileIcon;
                else if (status.second == 'D')
                    icon = mWdFileIcon;
                else if (status.second == 'E')
                    icon = mWeFileIcon;
                else if (status.second == 'M')
                    icon = mWmFileIcon;
                else if (status.second == 'Q')
                    icon = mWqFileIcon;
                else if (status.second == 'R')
                    icon = mWrFileIcon;
                else if (status.second == 'T')
                    icon = mWtFileIcon;
            }

            if (newItem) {
                // We already have an item, so just update its icon

                newItem->setIcon(icon);
            } else {
                // We don't already have an item, so create one and add it

                newItem = new PmrWorkspacesWindowItem(PmrWorkspacesWindowItem::File,
                                                      pWorkspace, fileNode, icon);

                pFolderItem->appendRow(newItem);
            }

            res << newItem;
        }
    }

    return res;
}

//==============================================================================

void PmrWorkspacesWindowWidget::sortAndResizeTreeViewToContents()
{
    // Sort the contents of our tree view widget and make sure that all of its
    // the contents is visible

    mTreeViewModel->sort(0);

    resizeTreeViewToContents();
}

//==============================================================================

void PmrWorkspacesWindowWidget::refreshWorkspace(PMRSupport::PmrWorkspace *pWorkspace,
                                                 const bool &pSortAndResize)
{
    // Refresh the status of the given workspace

    pWorkspace->refreshStatus();

    // Retrieve the item for the given workspace, if any

    PmrWorkspacesWindowItem *item = workspaceItem(pWorkspace);

    if (item) {
        // There is an item for the given workspace, retrieve and use the
        // (potentially new) icons to use with the item

        QIcon collapsedIcon;
        QIcon expandedIcon;

        retrieveWorkspaceIcons(pWorkspace, collapsedIcon, expandedIcon);

        item->setCollapsedIcon(collapsedIcon);
        item->setExpandedIcon(expandedIcon);

        item->setIcon(mTreeViewWidget->isExpanded(item->index())?expandedIcon:collapsedIcon);

        // Keep track of existing items

        PmrWorkspacesWindowItems oldItems = PmrWorkspacesWindowItems() << retrieveItems(item);

        // Populate the given workspace

        PmrWorkspacesWindowItems newItems = populateWorkspace(pWorkspace, item,
                                                              pWorkspace->rootFileNode());

        // Delete old unused items

        PmrWorkspacesWindowItems oldItemsToDelete = PmrWorkspacesWindowItems();

        foreach (PmrWorkspacesWindowItem *oldItem, oldItems) {
            if (!newItems.contains(oldItem))
                oldItemsToDelete << oldItem;
        }

        if (!oldItemsToDelete.isEmpty())
            deleteItems(item, oldItemsToDelete);

        // Make sure that everything is properly sorted and that all of the
        // contents of our tree view widget is visible

        if (pSortAndResize)
            sortAndResizeTreeViewToContents();
    }
}

//==============================================================================

void PmrWorkspacesWindowWidget::showCustomContextMenu(const QPoint &pPosition) const
{
    // Customise our context menu and show it

    PmrWorkspacesWindowItem *item = static_cast<PmrWorkspacesWindowItem *>(mTreeViewModel->itemFromIndex(mTreeViewWidget->indexAt(pPosition)));
    QModelIndexList selectedItems = mTreeViewWidget->selectedIndexes();
    bool ownedWorkspaceItem = item && (item->type() == PmrWorkspacesWindowItem::OwnedWorkspace);
    bool workspaceItem =    ownedWorkspaceItem
                         || (item && (item->type() == PmrWorkspacesWindowItem::Workspace));
    bool workspaceItems = true;

    for (int i = 0, iMax = selectedItems.count(); i < iMax; ++i) {
        PmrWorkspacesWindowItem *item = static_cast<PmrWorkspacesWindowItem *>(mTreeViewModel->itemFromIndex(selectedItems[i]));

        if (   (item->type() != PmrWorkspacesWindowItem::OwnedWorkspace)
            && (item->type() != PmrWorkspacesWindowItem::Workspace)) {
            workspaceItems = false;

            break;
        }
    }

    PMRSupport::PmrWorkspace::WorkspaceStatus workspaceStatus = workspaceItem?
                                                                    item->workspace()->gitWorkspaceStatus():
                                                                    PMRSupport::PmrWorkspace::StatusUnknown;
    int nbOfStagedFiles = 0;
    int nbOfUnstagedFiles = 0;

    for (int i = 0, iMax = selectedItems.count(); i < iMax; ++i) {
        PmrWorkspacesWindowItem *item = static_cast<PmrWorkspacesWindowItem *>(mTreeViewModel->itemFromIndex(selectedItems[i]));
        PMRSupport::PmrWorkspaceFileNode *fileNode = item->fileNode();
        bool stagedFile = fileNode?(fileNode->status().first != ' '):false;
        bool unstagedFile = fileNode?(fileNode->status().second != ' '):false;

        if (   (item->type() == PmrWorkspacesWindowItem::File)
            && (stagedFile || unstagedFile)) {
            nbOfStagedFiles += stagedFile;
            nbOfUnstagedFiles += unstagedFile;
        } else {
            nbOfStagedFiles = 0;
            nbOfUnstagedFiles = 0;

            break;
        }
    }

    bool onlyOneItem = selectedItems.count() == 1;

    I18nInterface::retranslateAction(mStageAction, tr("Stage"),
                                     onlyOneItem?
                                         tr("Stage the file for commit"):
                                         tr("Stage the files for commit"));
    I18nInterface::retranslateAction(mUnstageAction, tr("Unstage"),
                                     onlyOneItem?
                                         tr("Unstage the file from commit"):
                                         tr("Unstage the files from commit"));

    mNewAction->setVisible(!item);
    mViewInPmrAction->setVisible(workspaceItems);
    mViewOncomputerAction->setVisible(onlyOneItem && workspaceItem);
    mCopyUrlAction->setVisible(onlyOneItem && workspaceItem);
    mCopyPathAction->setVisible(onlyOneItem && workspaceItem);
    mCloneAction->setVisible(onlyOneItem && ownedWorkspaceItem);
    mCommitAction->setVisible(onlyOneItem && workspaceItem);
    mPullAction->setVisible(onlyOneItem && workspaceItem);
    mPullAndPushAction->setVisible(onlyOneItem && ownedWorkspaceItem);
    mStageAction->setVisible(nbOfUnstagedFiles);
    mUnstageAction->setVisible(nbOfStagedFiles);
    mAboutAction->setVisible(onlyOneItem && workspaceItem);

    bool clonedWorkspaceItem =     workspaceItem && item
                               && !mWorkspaceUrlFoldersOwned.value(item->url()).first.isEmpty();

    mViewOncomputerAction->setEnabled(clonedWorkspaceItem);
    mCopyPathAction->setEnabled(clonedWorkspaceItem);
    mCloneAction->setEnabled(!clonedWorkspaceItem);
    mCommitAction->setEnabled(workspaceStatus & PMRSupport::PmrWorkspace::StatusCommit);
    mPullAction->setEnabled(clonedWorkspaceItem);
    mPullAndPushAction->setEnabled(workspaceStatus & PMRSupport::PmrWorkspace::StatusAhead);

    mContextMenu->exec(QCursor::pos());
}

//==============================================================================

void PmrWorkspacesWindowWidget::itemDoubleClicked(const QModelIndex &pIndex)
{
    // Ask for a file to be opened

    PmrWorkspacesWindowItem *item = static_cast<PmrWorkspacesWindowItem *>(mTreeViewModel->itemFromIndex(pIndex));

    if (item->type() == PmrWorkspacesWindowItem::File)
        emit openFileRequested(item->fileName());
}

//==============================================================================

void PmrWorkspacesWindowWidget::itemExpanded(const QModelIndex &pIndex)
{
    // Update the icon of the item, if its type is that an owned workspace

    PmrWorkspacesWindowItem *item = static_cast<PmrWorkspacesWindowItem *>(mTreeViewModel->itemFromIndex(pIndex));

    item->setIcon(item->expandedIcon());
}

//==============================================================================

void PmrWorkspacesWindowWidget::itemCollapsed(const QModelIndex &pIndex)
{
    // Update the icon of the item, if its type is that an owned workspace

    PmrWorkspacesWindowItem *item = static_cast<PmrWorkspacesWindowItem *>(mTreeViewModel->itemFromIndex(pIndex));

    item->setIcon(item->collapsedIcon());
}

//==============================================================================

void PmrWorkspacesWindowWidget::duplicateCloneMessage(const QString &pUrl,
                                                      const QString &pPath1,
                                                      const QString &pPath2)
{
    // Let people know that the given workspace has been cloned twice, but only
    // if it doesn't occur when initialising ourselves
    // Note: indeed, someone handling the warning() signal might decide to show
    //       a message box to warn the user. Such a thing is fine when
    //       everything is initialised, but not when initialising ourselves
    //       (since the message box would show up from nowhere)...

    if (mInitialized)
        emit warning(QString("Workspace '%1' is cloned into both '%2' and '%3'").arg(pUrl, pPath1, pPath2));
}

//==============================================================================

void PmrWorkspacesWindowWidget::refreshWorkspaces(const PMRSupport::PmrWorkspaces &pWorkspaces)
{
    // Refresh our workspaces

    PMRSupport::PmrWorkspaces workspaces = pWorkspaces.isEmpty()?
                                               PMRSupport::PmrWorkspaceManager::instance()->workspaces():
                                               pWorkspaces;
    int workspacesCount = workspaces.count();
    int workspaceNb = 0;

    foreach (PMRSupport::PmrWorkspace *workspace, workspaces)
        refreshWorkspace(workspace, ++workspaceNb == workspacesCount);
}

//==============================================================================

void PmrWorkspacesWindowWidget::workspaceCloned(PMRSupport::PmrWorkspace *pWorkspace)
{
    // The given workspace has been cloned, so update ourselves accordingly

    QString url = pWorkspace->url();

    if (!mWorkspaceUrlFoldersOwned.contains(url)) {
        // We don't know about the workspace, so keep track of it and open it

        QString folder = pWorkspace->path();

        mClonedWorkspaceFolderUrls.insert(folder, url);
        mWorkspaceUrlFoldersOwned.insert(url, QPair<QString, bool>(folder, false));

        // Populate the workspace, if we own it and we have an entry for it
        // (i.e. we just cloned a workspace that we own), or add the workspace,
        // if we don't own it or just created a new one (that we own)

        PmrWorkspacesWindowItem *item = workspaceItem(pWorkspace);

        if (pWorkspace->isOwned() && item)
            populateWorkspace(pWorkspace, item, pWorkspace->rootFileNode());
        else
            addWorkspace(pWorkspace);
    } else {
        // We already know about the workspace, which means that we have already
        // cloned it and that we need to let the user know about it
        // Note: this should never happen, but better be safe than sorry...

        QPair<QString, bool> folderOwned = mWorkspaceUrlFoldersOwned.value(url);

        duplicateCloneMessage(url, folderOwned.first, pWorkspace->path());
    }
}

//==============================================================================

void PmrWorkspacesWindowWidget::workspaceUncloned(PMRSupport::PmrWorkspace *pWorkspace)
{
    // The given workspace has been uncloned, so update ourselves accordingly

    QString url = pWorkspace->url();

    mClonedWorkspaceFolderUrls.remove(mWorkspaceUrlFoldersOwned.value(url).first);
    mWorkspaceUrlFoldersOwned.remove(url);
}

//==============================================================================

void PmrWorkspacesWindowWidget::workspaceSynchronized(PMRSupport::PmrWorkspace *pWorkspace)
{
    // The workspace has been synchronised, so refresh it

    refreshWorkspace(pWorkspace);
}

//==============================================================================

void PmrWorkspacesWindowWidget::viewInPmr()
{
    // Show the selected items in PMR

    QModelIndexList selectedItems = mTreeViewWidget->selectedIndexes();

    for (int i = 0, iMax = selectedItems.count(); i < iMax; ++i)
        QDesktopServices::openUrl(static_cast<PmrWorkspacesWindowItem *>(mTreeViewModel->itemFromIndex(selectedItems[i]))->url());
}

//==============================================================================

void PmrWorkspacesWindowWidget::viewOnComputer()
{
    // Show the selected items on the user's computer

    QModelIndexList selectedItems = mTreeViewWidget->selectedIndexes();

    for (int i = 0, iMax = selectedItems.count(); i < iMax; ++i)
        QDesktopServices::openUrl(QUrl::fromLocalFile(static_cast<PmrWorkspacesWindowItem *>(mTreeViewModel->itemFromIndex(selectedItems[i]))->path()));
}

//==============================================================================

void PmrWorkspacesWindowWidget::copyUrl()
{
    // Copy the current item's URL to the clipboard

    QApplication::clipboard()->setText(currentItem()->url());
}

//==============================================================================

void PmrWorkspacesWindowWidget::copyPath()
{
    // Copy the current item's path to the clipboard

    QApplication::clipboard()->setText(currentItem()->path());
}

//==============================================================================

void PmrWorkspacesWindowWidget::clone()
{
    // Clone the owned workspace

    QString dirName = PMRSupport::PmrWebService::getEmptyDirectory();

    if (!dirName.isEmpty()) {
        // Create, if needed, the folder where the workspace will be cloned

        QDir workspaceFolder = QDir(dirName);

        if (!workspaceFolder.exists())
            workspaceFolder.mkpath(".");

        // Ask our PMR web service to effectively clone our owned workspace

        mPmrWebService->requestWorkspaceClone(PMRSupport::PmrWorkspaceManager::instance()->workspace(currentItem()->url()),
                                              dirName);
    }
}

//==============================================================================

void PmrWorkspacesWindowWidget::commit()
{
    // Commit the current workspace's staged changes

    PMRSupport::PmrWorkspace *workspace = currentItem()->workspace();

    if (workspace->isMerging()) {
        workspace->commitMerge();
    } else {
        PmrWorkspacesWindowCommitDialog commitDialog(workspace->stagedFiles(),
                                                     Core::mainWindow());

        if (commitDialog.exec() == QDialog::Accepted)
            workspace->commit(commitDialog.message());
    }

    refreshWorkspace(workspace);
}

//==============================================================================

void PmrWorkspacesWindowWidget::pull()
{
    // Synchronise the current workspace with PMR

    mPmrWebService->requestWorkspaceSynchronize(currentItem()->workspace(), false);
}

//==============================================================================

void PmrWorkspacesWindowWidget::pullAndPush()
{
    // Synchronise the current workspace with PMR and push its changes to it

    mPmrWebService->requestWorkspaceSynchronize(currentItem()->workspace(), true);
}

//==============================================================================

void PmrWorkspacesWindowWidget::stageUnstage(const bool &pStage)
{
    // Stage/unstage the current file(s)

    PMRSupport::PmrWorkspaces workspaces = PMRSupport::PmrWorkspaces();
    QModelIndexList selectedItems = mTreeViewWidget->selectedIndexes();

    for (int i = 0, iMax = selectedItems.count(); i < iMax; ++i) {
        PmrWorkspacesWindowItem *item = static_cast<PmrWorkspacesWindowItem *>(mTreeViewModel->itemFromIndex(selectedItems[i]));
        PMRSupport::PmrWorkspace *workspace = item->workspace();

        workspace->stageFile(item->fileName(), pStage);

        if (!workspaces.contains(workspace))
            workspaces << workspace;
    }

    // Refresh the workspaces that (may) have changed status

    refreshWorkspaces(workspaces);
}

//==============================================================================

void PmrWorkspacesWindowWidget::stage()
{
    // Stage the current file(s)

    stageUnstage(true);
}

//==============================================================================

void PmrWorkspacesWindowWidget::unstage()
{
    // Stage the current file(s)

    stageUnstage(false);
}

//==============================================================================

void PmrWorkspacesWindowWidget::about()
{
    // Let people know that we want to show some information about the current
    // workspace

    static const QString Entry = "    <tr>\n"
                                 "        <td style=\"font-weight: bold;\">%1</td>\n"
                                 "        <td>%2</td>\n"
                                 "    </tr>\n";

    PMRSupport::PmrWorkspace *workspace = PMRSupport::PmrWorkspaceManager::instance()->workspace(currentItem()->url());
    QString message = QString("<p style=\"font-weight: bold;\">\n"
                              "    %1\n"
                              "</p>\n").arg(workspace->name());

    if (!workspace->description().isEmpty()) {
        message += QString("\n"
                           "<p>\n"
                           "    %1\n"
                           "</p>\n").arg(workspace->description());
    }

    message += "\n"
               "<br/>\n"
               "\n"
               "<table>\n";

    if (!workspace->owner().isEmpty())
        message += Entry.arg(tr("Owner:"), workspace->owner());

    message += Entry.arg(tr("PMR:"))
                    .arg(QString("<a href=\"%1\">%1</a>").arg(workspace->url()));

    if (workspace->isLocal()) {
        message += Entry.arg(tr("Path:"))
                        .arg(QString("<a href=\"%1\">%2</a>").arg(QUrl::fromLocalFile(workspace->path()).url())
                                                             .arg(workspace->path()));
    }

    message += "</table>\n";

    emit information(message);
}

//==============================================================================

}   // namespace PMRWorkspacesWindow
}   // namespace OpenCOR

//==============================================================================
// End of file
//==============================================================================