//==============================================================================
// Single cell view graph panels widget
//==============================================================================

#ifndef SINGLECELLVIEWGRAPHPANELSWIDGET_H
#define SINGLECELLVIEWGRAPHPANELSWIDGET_H

//==============================================================================

#include "commonwidget.h"

//==============================================================================

#include <QSplitter>

//==============================================================================

namespace OpenCOR {
namespace SingleCellView {

//==============================================================================

class SingleCellViewGraphPanelWidget;

//==============================================================================

class SingleCellViewGraphPanelsWidget : public QSplitter,
                                        public Core::CommonWidget
{
    Q_OBJECT

public:
    explicit SingleCellViewGraphPanelsWidget(QWidget *pParent = 0);

    virtual void loadSettings(QSettings *pSettings);
    virtual void saveSettings(QSettings *pSettings) const;

    QList<SingleCellViewGraphPanelWidget *> graphPanels() const;
    SingleCellViewGraphPanelWidget * activeGraphPanel() const;

    SingleCellViewGraphPanelWidget * addGraphPanel();
    void removeGraphPanel();

private:
    QList<int> mSplitterSizes;

Q_SIGNALS:
    void grapPanelAdded(SingleCellViewGraphPanelWidget *pGraphPanel);
    void grapPanelRemoved(SingleCellViewGraphPanelWidget *pGraphPanel);

    void removeGraphPanelsEnabled(const bool &pEnabled);

    void graphPanelActivated(SingleCellViewGraphPanelWidget *pGraphPanel);

private Q_SLOTS:
    void splitterMoved();

    void updateGraphPanels(SingleCellViewGraphPanelWidget *pGraphPanel);
};

//==============================================================================

}   // namespace SingleCellView
}   // namespace OpenCOR

//==============================================================================

#endif

//==============================================================================
// End of file
//==============================================================================
