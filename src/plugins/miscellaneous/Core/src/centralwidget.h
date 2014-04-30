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
// Central widget
//==============================================================================

#ifndef CENTRALWIDGET_H
#define CENTRALWIDGET_H

//==============================================================================

#include "file.h"
#include "fileinterface.h"
#include "guiinterface.h"
#include "widget.h"

//==============================================================================

#include <QDir>
#include <QMap>
#include <QTabBar>

//==============================================================================

namespace Ui {
    class CentralWidget;
}

//==============================================================================

class QDialog;
class QLabel;
class QLineEdit;
class QStackedWidget;

//==============================================================================

namespace OpenCOR {

//==============================================================================

class Plugin;

//==============================================================================

namespace Core {

//==============================================================================

typedef QMap<int, Plugin *> CentralWidgetViewPlugins;
typedef QMap<int, GuiViewSettings *> CentralWidgetViewSettings;

//==============================================================================

class CentralWidget;
class UserMessageWidget;

//==============================================================================

class CentralWidgetMode
{
public:
    explicit CentralWidgetMode(CentralWidget *pOwner);
    ~CentralWidgetMode();

    bool isEnabled() const;
    void setEnabled(const bool &pEnabled);

    QTabBar * viewTabs() const;

    CentralWidgetViewPlugins * viewPlugins() const;
    CentralWidgetViewSettings * viewSettings() const;

private:
    bool mEnabled;

    QTabBar *mViewTabs;

    CentralWidgetViewPlugins *mViewPlugins;
    CentralWidgetViewSettings *mViewSettings;
};

//==============================================================================

class CentralWidget : public Widget
{
    Q_OBJECT

public:
    explicit CentralWidget(QMainWindow *pMainWindow);
    ~CentralWidget();

    virtual void loadSettings(QSettings *pSettings);
    virtual void saveSettings(QSettings *pSettings) const;

    void settingsLoaded(const Plugins &pLoadedPlugins);

    virtual void retranslateUi();

    void setSupportedFileTypes(const FileTypes &pSupportedFileTypes);

    void addView(Plugin *pPlugin, GuiViewSettings *pSettings);

    QTabBar * newTabBar(const QTabBar::Shape &pShape,
                        const bool &pMovable = false,
                        const bool &pTabsClosable = false);

    QString currentFileName() const;

    void openFile(const QString &pFileName,
                  const File::Type &pType = File::Local,
                  const QString &pUrl = QString());
    void openFiles(const QStringList &pFileNames);

    void openRemoteFile(const QString &pUrl, const bool &pShowWarning = true);

    bool canClose();

protected:
    virtual void dragEnterEvent(QDragEnterEvent *pEvent);
    virtual void dragMoveEvent(QDragMoveEvent *pEvent);
    virtual void dropEvent(QDropEvent *pEvent);

private:
    enum State {
        Starting,
        Idling,
        UpdatingGui,
        Stopping
    };

    QMainWindow *mMainWindow;

    Ui::CentralWidget *mGui;

    State mState;

    Plugins mLoadedPlugins;

    QTabBar *mModeTabs;
    QTabBar *mFileTabs;

    QMap<int, GuiViewSettings::Mode> mModeTabIndexModes;
    QMap<GuiViewSettings::Mode, int> mModeModeTabIndexes;

    QMap<QString, int> mFileModeTabIndexes;
    QMap<QString, QMap<int, int>> mFileModeViewTabIndexes;

    FileTypes mSupportedFileTypes;

    QStringList mFileNames;

    QStackedWidget *mContents;

    QWidget *mLogoView;

    UserMessageWidget *mNoViewMsg;

    QMap<GuiViewSettings::Mode, CentralWidgetMode *> mModes;

    QDialog *mRemoteFileDialog;
    QLabel *mRemoteFileDialogUrlLabel;
    QLineEdit *mRemoteFileDialogUrlValue;

    QMap<QString, QString> mRemoteLocalFileNames;

    Plugin * viewPlugin(const int &pIndex) const;
    Plugin * viewPlugin(const QString &pFileName) const;

    void updateNoViewMsg();

    bool saveFile(const int &pIndex, const bool &pNeedNewFileName = false);

    bool canCloseFile(const int &pIndex);

    void updateFileTab(const int &pIndex);

    void updateStatusBarWidgets(QList<QWidget *> pWidgets);

Q_SIGNALS:
    void guiUpdated(Plugin *pViewPlugin, const QString &pFileName);

    void canSave(const bool &pEnabled);
    void canSaveAs(const bool &pEnabled);
    void canSaveAll(const bool &pEnabled);

    void atLeastOneFile(const bool &pAtLeastOneFile);
    void atLeastTwoFiles(const bool &pAtLeastTwoFiles);

private Q_SLOTS:
    void updateGui();

    void openFile();

    void doOpenRemoteFile();
    void cancelOpenRemoteFile();
    void openRemoteFile();

    void reloadFile(const int &pIndex = -1);

    void duplicateFile();

    void toggleLockedFile();

    void fileChanged(const QString &pFileName);
    void fileDeleted(const QString &pFileName);

    void updateModifiedSettings();

    void filePermissionsChanged(const QString &pFileName);
    void fileModified(const QString &pFileName, const bool &pModified);

    void fileReloaded(const QString &pFileName);

    void fileCreated(const QString &pFileName, const QString &pUrl);
    void fileDuplicated(const QString &pFileName);
    void fileRenamed(const QString &pOldFileName, const QString &pNewFileName);

    void saveFile();
    void saveFileAs();
    void saveAllFiles();

    void previousFile();
    void nextFile();

    bool closeFile(const int &pIndex = -1, const bool &pForceClosing = false);
    void closeAllFiles(const bool &pForceClosing = false);

    void moveFile(const int &pFromIndex, const int &pToIndex);

    void updateFileTabIcon(const QString &pViewName, const QString &pFileName,
                           const QIcon &pIcon);
    void updateFileTabIcons();
};

//==============================================================================

}   // namespace Core
}   // namespace OpenCOR

//==============================================================================

#endif

//==============================================================================
// End of file
//==============================================================================
