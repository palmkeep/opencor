#include "docktoolbar.h"
#include "documentmanager.h"
#include "filebrowserwindow.h"
#include "filebrowserwidget.h"
#include "mainwindow.h"
#include "onefieldwindow.h"
#include "utils.h"

#include "ui_filebrowserwindow.h"

#include <QDir>
#include <QFileInfo>
#include <QMenu>
#include <QMessageBox>

static const QString SettingsFileBrowserWindow = "FileBrowserWindow";

FileBrowserWindow::FileBrowserWindow(MainWindow *pParent) :
    DockWidget(pParent),
    mUi(new Ui::FileBrowserWindow),
    mPrevFolder(),
    mKeepTrackOfPrevFolder(true)
{
    // Set up the UI

    mUi->setupUi(this);

    // Retrieve the document manager from our parent which is the main window

    mDocumentManager = pParent->documentManager();

    // Create a dropdown menu for the New action

    QMenu *actionNewMenu = new QMenu(this);

    actionNewMenu->addAction(mUi->actionNewFolder);
    actionNewMenu->addSeparator();
    actionNewMenu->addAction(mUi->actionNewCellML10File);
    actionNewMenu->addAction(mUi->actionNewCellML11File);

    mUi->actionNew->setMenu(actionNewMenu);

    // Create a toolbar with different buttons
    // Note: this sadly can't be done using the design mode, so...

    DockToolBar *toolbar = new DockToolBar(this);

    toolbar->addAction(mUi->actionHome);
    toolbar->addSeparator();
    toolbar->addAction(mUi->actionParent);
    toolbar->addSeparator();
    toolbar->addAction(mUi->actionPrevious);
    toolbar->addAction(mUi->actionNext);
    toolbar->addSeparator();
    toolbar->addAction(mUi->actionNew);
    toolbar->addAction(mUi->actionDelete);

    mUi->verticalLayout->addWidget(toolbar);

    // Create and add the file browser widget
    // Note: we let the widget know that we want our own custom context menu

    mFileBrowserWidget = new FileBrowserWidget(this);

    mUi->verticalLayout->addWidget(mFileBrowserWidget);

    // We want our own context menu for the file browser widget

    mFileBrowserWidget->setContextMenuPolicy(Qt::CustomContextMenu);

    // Prevent objects from being dropped on the file browser widget

    mFileBrowserWidget->setAcceptDrops(false);

    // Some connections

    connect(mFileBrowserWidget, SIGNAL(customContextMenuRequested(const QPoint &)),
            this, SLOT(customContextMenu(const QPoint &)));

    connect(mFileBrowserWidget->selectionModel(), SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)),
            this, SLOT(currentItemChanged(const QModelIndex &, const QModelIndex &)));
    connect(mFileBrowserWidget, SIGNAL(expanded(const QModelIndex &)),
            this, SLOT(needUpdateActions()));
    connect(mFileBrowserWidget, SIGNAL(collapsed(const QModelIndex &)),
            this, SLOT(needUpdateActions()));
}

FileBrowserWindow::~FileBrowserWindow()
{
    // Delete the UI

    delete mUi;
}

void FileBrowserWindow::updateActions()
{
    // Make sure that the various actions are properly enabled/disabled

    mUi->actionHome->setEnabled(   (mFileBrowserWidget->currentPath() != mFileBrowserWidget->homeFolder())
                                || (   (mFileBrowserWidget->currentPath() == mFileBrowserWidget->homeFolder())
                                    && !mFileBrowserWidget->isCurrentPathVisible()));

    mUi->actionParent->setEnabled(mFileBrowserWidget->currentPathParent() != "");

    mUi->actionPrevious->setEnabled(mPrevFolders.count());
    mUi->actionNext->setEnabled(mNextFolders.count());

    mUi->actionNew->setEnabled(mFileBrowserWidget->isCurrentPathDirWritable());
    mUi->actionDelete->setEnabled(mFileBrowserWidget->isCurrentPathWritable());
}

void FileBrowserWindow::retranslateUi()
{
    // Translate the whole window

    mUi->retranslateUi(this);

    // Make sure that the enabled state of the various actions is correct
    // (indeed, to translate everything messes things up in that respect,
    // so...)

    updateActions();

    // Retranslate the file browser widget

    mFileBrowserWidget->retranslateUi();
}

void FileBrowserWindow::loadSettings(const QSettings &pSettings,
                                     const QString &)
{
    // Retrieve the settings of the file browser widget
    // Note: we must make sure that we don't keep track of the previous folder

    mKeepTrackOfPrevFolder = false;

    mFileBrowserWidget->loadSettings(pSettings, SettingsFileBrowserWindow);

    mKeepTrackOfPrevFolder = true;

    // Initialise the previous folder information

    mPrevFolder = mFileBrowserWidget->currentPathDir();

    // Make sure that the current path is expanded
    // Note: this is important in case the current path is that of the C:\ drive
    //       or the root of the file system which during the loadSettings above
    //       won't trigger a directoryLoaded signal in the file browser widget

    if (!mFileBrowserWidget->isExpanded(mFileBrowserWidget->currentIndex()))
        mFileBrowserWidget->setExpanded(mFileBrowserWidget->currentIndex(), true);
}

void FileBrowserWindow::saveSettings(QSettings &pSettings, const QString &)
{
    // Keep track of the settings of the file browser widget

    mFileBrowserWidget->saveSettings(pSettings, SettingsFileBrowserWindow);
}

void FileBrowserWindow::customContextMenu(const QPoint &)
{
    // Create a custom context menu which items match the contents of our
    // toolbar

    QMenu menu;

    menu.addAction(mUi->actionHome);
    menu.addSeparator();
    menu.addAction(mUi->actionParent);
    menu.addSeparator();
    menu.addAction(mUi->actionPrevious);
    menu.addAction(mUi->actionNext);
    menu.addSeparator();
    menu.addAction(mUi->actionNew);
    menu.addAction(mUi->actionDelete);

    menu.exec(QCursor::pos());
}

void FileBrowserWindow::needUpdateActions()
{
    // Something related to the help widget requires the actions to be udpated

    updateActions();
}

void FileBrowserWindow::currentItemChanged(const QModelIndex &,
                                           const QModelIndex &)
{
    if (!mKeepTrackOfPrevFolder)
        // We don't want to keep track of the previous folder, so...

        return;

    // A new item has been selected, so we need to keep track of the old one in
    // case we want to go back to it

    // Retrieve the full path to the folder where the current item is located

    QString crtItemDir  = mFileBrowserWidget->currentPathDir();

    // Check whether there is a previous folder to keep track of
    // Note: to do so, we cannot rely on the previous item (i.e. the second
    //       parameter to this function), since both the current and previous
    //       items might be located in the same folder, so...

    if (crtItemDir != mPrevFolder) {
        // There is a previous folder to keep track of, so add it to our list of
        // previous folders and consider the current folder as our future
        // previous folder

        mPrevFolders.append(mPrevFolder);

        mPrevFolder = crtItemDir;

        // Reset the list of next folders since that list doesn't make sense
        // anymore

        mNextFolders.clear();
    }

    // Since a new item has been selected, we must update the actions

    updateActions();
}

void FileBrowserWindow::on_actionHome_triggered()
{
    // Go to the home folder (and ask for it to be expanded)

    mFileBrowserWidget->gotoHomeFolder(true);
}

void FileBrowserWindow::on_actionParent_triggered()
{
    // Go to the parent folder

    mFileBrowserWidget->gotoPath(mFileBrowserWidget->currentPathParent());

    // Going to the parent folder may have changed some actions, so...

    updateActions();
}

void FileBrowserWindow::on_actionPrevious_triggered()
{
    // Go to the previous folder and move the last item from our list of
    // previous folders to our list of next folders
    // Note: we must make sure that we don't keep track of the previous folder

    mNextFolders.append(mFileBrowserWidget->currentPath());

    mKeepTrackOfPrevFolder = false;

    mFileBrowserWidget->gotoPath(mPrevFolders.last());

    mKeepTrackOfPrevFolder = true;

    mPrevFolders.removeLast();

    // Going to the previous folder may have changed some actions, so...

    updateActions();
}

void FileBrowserWindow::on_actionNext_triggered()
{
    // Go to the next folder and move the last item from our list of next
    // folders to our list of previous folders
    // Note: we must make sure that we don't keep track of the previous folder

    mPrevFolders.append(mFileBrowserWidget->currentPath());

    mKeepTrackOfPrevFolder = false;

    mFileBrowserWidget->gotoPath(mNextFolders.last());

    mKeepTrackOfPrevFolder = true;

    mNextFolders.removeLast();

    // Going to the next folder may have changed some actions, so...

    updateActions();
}

void FileBrowserWindow::on_actionNew_triggered()
{
    // The default new action is about creating a new folder, so...

    on_actionNewFolder_triggered();
}

static QString FolderFileNameRegExp = "[^\\/:*?\"<>|]+";

void FileBrowserWindow::on_actionNewFolder_triggered()
{
    // Get the name of the new folder

    OneFieldWindow oneFieldWindow(tr("New Folder"), tr("Folder name:"),
                                  tr("Please provide a name for the new folder."),
                                  FolderFileNameRegExp, this);

    oneFieldWindow.exec();

    // Create the new folder in the current folder unless the user cancelled the
    // action

    if (oneFieldWindow.result() == QDialog::Accepted) {
        QString folderPath =  mFileBrowserWidget->currentPathDir()
                             +QDir::separator()+oneFieldWindow.fieldValue();

        if (QDir(folderPath).exists()) {
            // The folder already exists, so...

            QMessageBox::information(this, qApp->applicationName(),
                                     tr("Sorry, but the <strong>%1</strong> folder already exists.").arg(oneFieldWindow.fieldValue()));
        } else {
            // The folder doesn't already exist, so try to create it

            if (QDir(mFileBrowserWidget->currentPathDir()).mkdir(oneFieldWindow.fieldValue()))
                // The folder was created, so select it

                mFileBrowserWidget->gotoPath(folderPath);
            else
                // The folder couldn't be created, so...

                QMessageBox::information(this, qApp->applicationName(),
                                         tr("Sorry, but the <strong>%1</strong> folder could not be created.").arg(oneFieldWindow.fieldValue()));
        }
    }
}

void FileBrowserWindow::newCellmlFile(const CellmlVersion &pCellmlVersion)
{
    // Get the name of the new CellML file

    QString cellmlVersion = cellmlVersionString(pCellmlVersion);

    OneFieldWindow oneFieldWindow(tr("New CellML %1 File").arg(cellmlVersion), tr("Model name:"),
                                  tr("Please provide a name for the new CellML %1 model.").arg(cellmlVersion),
                                  FolderFileNameRegExp, this);

    oneFieldWindow.exec();

    // Create the new CellML file in the current folder unless the user
    // cancelled the action

    if (oneFieldWindow.result() == QDialog::Accepted) {
        QString cellmlFileName =  oneFieldWindow.fieldValue()
                                 +CellmlFileExtension;
        QString cellmlFilePath = mFileBrowserWidget->currentPathDir()
                                 +QDir::separator()+cellmlFileName;

        if (QFileInfo(cellmlFilePath).exists()) {
            // The CellML file already exists, so...

            QMessageBox::information(this, qApp->applicationName(),
                                     tr("Sorry, but the <strong>%1</strong> file already exists.").arg(cellmlFileName));
        } else {
            // The CellML file doesn't already exist, so try to create it

            if (::newCellmlFile(cellmlFilePath, oneFieldWindow.fieldValue(),
                                pCellmlVersion)) {
                // The CellML file was created, so select it

                mFileBrowserWidget->gotoPath(cellmlFilePath);

                // Have the new CellML file managed

                mDocumentManager->manage(cellmlFilePath);
            } else {
                // The CellML file couldn't be created, so...

                QMessageBox::information(this, qApp->applicationName(),
                                         tr("Sorry, but the <strong>%1</strong> file could not be created.").arg(cellmlFileName));
            }
        }
    }
}

void FileBrowserWindow::on_actionNewCellML10File_triggered()
{
    newCellmlFile(Cellml_1_0);
}

void FileBrowserWindow::on_actionNewCellML11File_triggered()
{
    newCellmlFile(Cellml_1_1);
}

void FileBrowserWindow::updateFolders(const QString &pFolderName,
                                      QStringList &pFolders)
{
    // Remove any instance of pFolderName in pFolders

    pFolders.removeAll(pFolderName);

    // Because of the above, we may have two or more consective identital
    // folders in the list, so we must reduce that to one

    if (pFolders.count() > 1) {
        QStringList newFolders;
        QString prevFolder = pFolders.at(0);

        newFolders.append(prevFolder);

        for (int i = 1; i < pFolders.count(); ++i) {
            QString crtFolder = pFolders.at(i);

            if (crtFolder != prevFolder) {
                // The current and previous folders are different, so we want to
                // keep track of it and add it to our new list

                newFolders.append(crtFolder);

                prevFolder = crtFolder;
            }
        }

        // Update the old list of folders with our new one

        pFolders = newFolders;
    }
}

void FileBrowserWindow::on_actionDelete_triggered()
{
    // Delete the current folder/file

    QString path = mFileBrowserWidget->currentPath();
    QFileInfo pathFileInfo = path;
    QString pathFileName = pathFileInfo.fileName();

    if (!pathFileName.isEmpty()) {
        // We are not at the root of the file system, so we can ask to delete
        // the folder/file

        bool isPathDir = pathFileInfo.isDir();

        if( QMessageBox::question(this, qApp->applicationName(),
                                  isPathDir?
                                      tr("Do you really want to delete the <strong>%1</strong> folder?").arg(pathFileName):
                                      tr("Do you really want to delete the <strong>%1</strong> file?").arg(pathFileName),
                                  QMessageBox::Yes|QMessageBox::No,
                                  QMessageBox::Yes) == QMessageBox::Yes ) {
            // The user really wants to delete the folder/file, so...

            if (removePath(path)) {
                // The folder/file has been removed, so now make sure that the
                // newly selected entry is visible

                mFileBrowserWidget->showCurrentPath();

                // In the case of a folder, we need to update our list of
                // previous and next folders

                if (isPathDir) {
                    // Update the lists of previous and next folders, as well as
                    // mPrevFolder

                    updateFolders(path, mPrevFolders);
                    updateFolders(path, mNextFolders);

                    mPrevFolder = mFileBrowserWidget->currentPathDir();

                    if (mPrevFolders.last() == mPrevFolder)
                        // After deleting, we have to reset mPrevFolder the path
                        // directory of the newly selected entry which means
                        // that the last folder in mPrevFolders cannot be that
                        // path directory, but it is here, so...

                        mPrevFolders.removeLast();

                    // Since we have updated the two lists, we must update the
                    // actions

                    updateActions();
                }
            } else {
                // The folder/file couldn't be removed, so let the user know
                // about it
                // Note: we should never reach this point in the case of a file,
                //       since the action is only triggerable if the file is
                //       writable (and therefore deletable), but still... better
                //       be safe than 'sorry'...

                QMessageBox::information(this, qApp->applicationName(),
                                         isPathDir?
                                             tr("Sorry, but the <strong>%1</strong> folder could not be deleted.").arg(pathFileName):
                                             tr("Sorry, but the <strong>%1</strong> file could not be deleted.").arg(pathFileName));
            }
        }
    }
}
