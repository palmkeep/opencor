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
// QScintillaWidget class
//==============================================================================

#include "filemanager.h"
#include "guiutils.h"
#include "qscintillawidget.h"

//==============================================================================

#include <QDragEnterEvent>
#include <QLabel>
#include <QMenu>
#include <QMimeData>

//==============================================================================

#include "Qsci/qscilexer.h"

//==============================================================================

namespace OpenCOR {
namespace QScintillaSupport {

//==============================================================================

QScintillaWidget::QScintillaWidget(const QString &pContents,
                                   const bool &pReadOnly,
                                   QsciLexer *pLexer, QWidget *pParent) :
    QsciScintilla(pParent),
    mCanUndo(false),
    mCanRedo(false),
    mCanSelectAll(false),
    mOverwriteMode(false)
{
    // Remove the frame around our Scintilla editor

    setFrameShape(QFrame::NoFrame);

    // Remove the margin number in our Scintilla editor

    setMarginWidth(SC_MARGIN_NUMBER, 0);

    // Associate a lexer to our Scintilla editor, should one be provided
    // Note: the default font family and size come from Qt Creator...

#if defined(Q_OS_WIN)
    mFont = QFont("Courier", 10);
#elif defined(Q_OS_LINUX)
    mFont = QFont("Monospace", 9);
#elif defined(Q_OS_MAC)
    mFont = QFont("Monaco", 12);
#else
    #error Unsupported platform
#endif

    if (pLexer) {
        // A lexer was provided, so specify its fonts and associate it with our
        // Scintilla editor

        pLexer->setFont(mFont);

        setLexer(pLexer);

        // Specify the type of tree folding to be used. Some lexers may indeed
        // use that feature, so...

        setFolding(QsciScintilla::BoxedTreeFoldStyle);
    } else {
        // No lexer was provided, so simply specify a default font family and
        // size for our Scintilla editor

        setFont(mFont);
    }

    // Set the contents of our Scintilla editor and its read-only property

    setContents(pContents);
    setReadOnly(pReadOnly);

    // Show the caret line

    setCaretLineVisible(true);

    // Initialise our colours by 'updating' them

    updateColors();

    // Clear some key mappings inherited from QsciScintilla
    // Note #1: indeed, QsciScintilla handles some shortcuts (e.g. Ctrl+L),
    //          which we don't want to see handled (e.g. Ctrl+L is used by
    //          QsciScintilla to delete the current line while OpenCOR uses it
    //          to (un)lock the current file), so...
    // Note #2: even though we are clearing those key mappings, we must also
    //          bypass QsciScintilla's handling of event() (see below). Indeed,
    //          not to do so would mean that if, for example, the user was to
    //          press Ctrl+L, then nothing would happen while we would have
    //          expected the current file to be (un)locked...

    SendScintilla(SCI_CLEARCMDKEY, (SCMOD_CTRL << 16)+'D');
    SendScintilla(SCI_CLEARCMDKEY, (SCMOD_CTRL << 16)+'L');
    SendScintilla(SCI_CLEARCMDKEY, (SCMOD_CTRL << 16)+(SCMOD_SHIFT << 16)+'L');

    // Empty context menu by default

    mContextMenu = new QMenu(this);

    // Create our two labels to show our cursor position and editing mode

    mCursorPositionWidget = new QLabel(this);
    mEditingModeWidget = new QLabel(this);

    // Keep track of the change to the UI

    connect(this, SIGNAL(SCN_UPDATEUI(int)),
            this, SLOT(updateUi()));

    // Keep track of changes to our editor that may affect our ability to select
    // all of its text
    // Note: we use the SCN_MODIFIED() signal rather than the textChanged()
    //       signal since the latter is only emitted when inserting or deleting
    //       some text...

    connect(this, SIGNAL(selectionChanged()),
            this, SLOT(checkCanSelectAll()));
    connect(this, SIGNAL(SCN_MODIFIED(int, int, const char *, int, int, int, int, int, int, int)),
            this, SLOT(checkCanSelectAll()));

    // Keep track of the change in the cursor position

    connect(this, SIGNAL(cursorPositionChanged(int, int)),
            this, SLOT(cursorPositionChanged(const int &, const int &)));
}

//==============================================================================

QScintillaWidget::~QScintillaWidget()
{
    // Delete some internal objects

    delete mContextMenu;
}

//==============================================================================

QMenu * QScintillaWidget::contextMenu() const
{
    // Return our context menu

    return mContextMenu;
}

//==============================================================================

void QScintillaWidget::setContextMenu(const QList<QAction *> &pContextMenuActions)
{
    // Set our context menu

    mContextMenu->clear();

    foreach (QAction *action, pContextMenuActions)
        mContextMenu->addAction(action);
}

//==============================================================================

int QScintillaWidget::currentPosition() const
{
    // Return our current position

    return SendScintilla(SCI_GETCURRENTPOS);
}

//==============================================================================

QString QScintillaWidget::contents() const
{
    // Return our contents

    return text();
}

//==============================================================================

void QScintillaWidget::setContents(const QString &pContents)
{
    // Set our contents

    setText(pContents);
}

//==============================================================================

int QScintillaWidget::contentsSize() const
{
    // Return the size of our contents

    return SendScintilla(SCI_GETLENGTH);
}

//==============================================================================

int QScintillaWidget::findTextInRange(const int &pStartRange,
                                      const int &pEndRange,
                                      const QString &pText,
                                      const bool &pCaseSensitive) const
{
    // Find and return the position, if any, of the given text within the given
    // range

    SendScintilla(SCI_SETSEARCHFLAGS, pCaseSensitive?SCFIND_MATCHCASE:0);

    SendScintilla(SCI_SETTARGETSTART, pStartRange);
    SendScintilla(SCI_SETTARGETEND, pEndRange);

    QByteArray text = pText.toUtf8();

    return SendScintilla(SCI_SEARCHINTARGET, text.length(), text.constData());
}

//==============================================================================

bool QScintillaWidget::isSelectAllAvailable() const
{
    // Return whether we can select all the text

    return mCanSelectAll;
}

//==============================================================================

void QScintillaWidget::resetUndoHistory()
{
    // Reset our undo history

    SendScintilla(SCI_EMPTYUNDOBUFFER);

    mCanUndo = false;
    mCanRedo = false;
}

//==============================================================================

QLabel * QScintillaWidget::cursorPositionWidget() const
{
    // Return our cursort position widget

    return mCursorPositionWidget;
}

//==============================================================================

QLabel * QScintillaWidget::editingModeWidget() const
{
    // Return our editing mode widget

    return mEditingModeWidget;
}

//==============================================================================

void QScintillaWidget::changeEvent(QEvent *pEvent)
{
    // Default handling of the event

    QsciScintilla::changeEvent(pEvent);

    // Check whether the palette has changed and if so then update our colors

    if (pEvent->type() == QEvent::PaletteChange)
        updateColors();
}

//==============================================================================

void QScintillaWidget::contextMenuEvent(QContextMenuEvent *pEvent)
{
    // Show our context menu or QsciScintilla's one, if we don't have one

    if (mContextMenu->isEmpty())
        QsciScintilla::contextMenuEvent(pEvent);
    else
        mContextMenu->exec(pEvent->globalPos());
}

//==============================================================================

void QScintillaWidget::dragEnterEvent(QDragEnterEvent *pEvent)
{
    // Accept the proposed action for the event, but only if we are not dropping
    // URIs
    // Note: this is not (currently?) needed on Windows and OS X, but if we
    //       don't have that check on Linux, then to drop some files on our
    //       Scintilla editor will result in the text/plain version of the data
    //       (e.g. file:///home/me/myFile) to be inserted in the text, so...

    if (!pEvent->mimeData()->hasFormat(Core::FileSystemMimeType))
        pEvent->acceptProposedAction();
    else
        pEvent->ignore();
}

//==============================================================================

bool QScintillaWidget::event(QEvent *pEvent)
{
    // Bypass QsciScintilla's handling of event()
    // Note: see the note on the clearing of some key mappings in the contructor
    //       above...

    return QsciScintillaBase::event(pEvent);
}

//==============================================================================

void QScintillaWidget::keyPressEvent(QKeyEvent *pEvent)
{
    // Reset the font size, if needed

    if (   !(pEvent->modifiers() & Qt::ShiftModifier)
        &&  (pEvent->modifiers() & Qt::ControlModifier)
        && !(pEvent->modifiers() & Qt::AltModifier)
        && !(pEvent->modifiers() & Qt::MetaModifier)
        &&  (pEvent->key() == Qt::Key_0)) {
        zoomTo(0);
    } else {
        // Default handling of the event

        QsciScintilla::keyPressEvent(pEvent);

        // Check whether the undo/redo is possible

        bool newCanUndo = isUndoAvailable();
        bool newCanRedo = isRedoAvailable();

        if (newCanUndo != mCanUndo) {
            mCanUndo = newCanUndo;

            emit canUndo(mCanUndo);
        }

        if (newCanRedo != mCanRedo) {
            mCanRedo = newCanRedo;

            emit canRedo(mCanRedo);
        }
    }
}

//==============================================================================

void QScintillaWidget::wheelEvent(QWheelEvent *pEvent)
{
    // Increasing/decrease the font size, if needed

    if (pEvent->modifiers() == Qt::ControlModifier) {
        int delta = pEvent->delta();

        if (delta > 0)
            zoomIn();
        else if (delta < 0)
            zoomOut();

        pEvent->accept();
    } else {
        QsciScintilla::wheelEvent(pEvent);
    }
}

//==============================================================================

void QScintillaWidget::updateUi()
{
    // Update our editing mode, if needed

    bool newOverwriteMode = overwriteMode();

    if (   (newOverwriteMode != mOverwriteMode)
        || mEditingModeWidget->text().isEmpty()) {
        mOverwriteMode = newOverwriteMode;

        mEditingModeWidget->setText(mOverwriteMode?"OVR":"INS");
    }
}

//==============================================================================

void QScintillaWidget::checkCanSelectAll()
{
    // Check whether we can select all the text

    bool newCanSelectAll = text().size() && selectedText().compare(text());

    if (newCanSelectAll != mCanSelectAll) {
        mCanSelectAll = newCanSelectAll;

        emit canSelectAll(mCanSelectAll);
    }
}

//==============================================================================

void QScintillaWidget::updateColors()
{
    // Compute and set the background colour of our caret line

    static const qreal Threshold = 0.875;

    QColor caretLineBackgroundColor = Core::highlightColor();

    qreal r = caretLineBackgroundColor.redF();
    qreal g = caretLineBackgroundColor.greenF();
    qreal b = caretLineBackgroundColor.blueF();

    while ((r < Threshold) || (g < Threshold) || (b < Threshold)) {
        r = 0.5*(r+1.0);
        g = 0.5*(g+1.0);
        b = 0.5*(b+1.0);
    }

    setCaretLineBackgroundColor(qRgba(r*255, g*255, b*255, caretLineBackgroundColor.alpha()));
}

//==============================================================================

void QScintillaWidget::cursorPositionChanged(const int &pLine,
                                             const int &pColumn)
{
    // Update our cursor position

    mCursorPositionWidget->setText(QString("Line: %1, Col: %2").arg(QString::number(pLine+1),
                                                                    QString::number(pColumn+1)));
}

//==============================================================================

}   // namespace QScintillaSupport
}   // namespace OpenCOR

//==============================================================================
// End of file
//==============================================================================
