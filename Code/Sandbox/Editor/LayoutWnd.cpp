/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include "EditorDefs.h"

#include "LayoutWnd.h"

// Qt
#include <QMessageBox>
#include <QToolBar>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QTextDocumentFragment>

#include <AzCore/Interface/Interface.h>
#include <AzFramework/API/AtomActiveInterface.h>
#include <AzQtComponents/Components/Style.h>    // for AzQtComponents::Style

// Editor
#include "MainWindow.h"
#include "ViewPane.h"
#include "QtViewPaneManager.h"

#include <AzQtComponents/Components/Style.h>

class CLayoutSplitterHandle
    : public QSplitterHandle
{
public:
    CLayoutSplitterHandle(Qt::Orientation orientation, CLayoutSplitter* parent)
        : QSplitterHandle(orientation, parent)
    {
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        // draw nothing for flat look
    }
};

/////////////////////////////////////////////////////////////////////////////
// CLayoutSplitter
//////////////////////////////////////////////////////////////////////////
CLayoutSplitter::CLayoutSplitter(QWidget* parent)
    : QSplitter(parent)
{
}

CLayoutSplitter::~CLayoutSplitter()
{
}

QSplitterHandle* CLayoutSplitter::createHandle()
{
    return new CLayoutSplitterHandle(orientation(), this);
}

//////////////////////////////////////////////////////////////////////////
void CLayoutSplitter::resizeEvent(QResizeEvent* event)
{
    QSplitter::resizeEvent(event);

    // only the top level splitter should trigger resetting the layout when being resized
    if (qobject_cast<CLayoutSplitter*>(parentWidget()) == nullptr)
    {
        for (CLayoutSplitter* s : findChildren<CLayoutSplitter*>())
        {
            s->setSizes(QList<int>::fromVector(QVector<int>(s->count(), (s->orientation() == Qt::Horizontal ? s->width() : s->height()) / s->count())));
        }
        setSizes(QList<int>::fromVector(QVector<int>(count(), (orientation() == Qt::Horizontal ? width() : height()) / count())));
    }
}

//////////////////////////////////////////////////////////////////////////
void CLayoutSplitter::CreateLayoutView(int row, int col, int id)
{
    assert(row >= 0 && row < 3);
    assert(col >= 0 && col < 3);
    CLayoutViewPane* viewPane = new CLayoutViewPane(this);
    viewPane->setWindowFlags(Qt::Widget);
    insertWidget(orientation() == Qt::Horizontal ? col : row, viewPane);
    viewPane->SetId(id);
}

//////////////////////////////////////////////////////////////////////////
// InfoBarExpanderWatcher
//////////////////////////////////////////////////////////////////////////

class InfoBarExpanderWatcher
    : public QObject
{
public:
    InfoBarExpanderWatcher(QObject* parent = nullptr)
        : QObject(parent)
    {
    }

    bool eventFilter(QObject* obj, QEvent* event) override
    {
        switch (event->type())
        {
            case QEvent::MouseButtonPress:
            case QEvent::MouseButtonRelease:
            case QEvent::MouseButtonDblClick:
            {
                if (qobject_cast<QToolButton*>(obj))
                {
                    auto mouseEvent = static_cast<QMouseEvent*>(event);
                    auto expansion = qobject_cast<QToolButton*>(obj);

                    expansion->setPopupMode(QToolButton::InstantPopup);
                    auto menu = new QMenu(expansion);

                    auto toolbar = qobject_cast<QToolBar*>(expansion->parentWidget());

                    auto toolWidgets = toolbar->findChildren<QWidget*>();

                    if (toolWidgets.count() > 0)
                    {
                        for (auto toolWidget : toolWidgets)
                        {
                            if (AzQtComponents::Style::hasClass(toolWidget, "expanderMenu_hide"))
                            {
                                continue;
                            }

                            if (auto toolButton = qobject_cast<QToolButton*>(toolWidget))
                            {
                                if (!toolButton->isVisible())
                                {
                                    // Skip some empty buttons
                                    if (toolButton->text().isEmpty())
                                    {
                                        continue;
                                    }

                                    QString plainText = QTextDocumentFragment::fromHtml(toolButton->text()).toPlainText();
                                    QAction* action = new QAction(plainText, menu);

                                    if (!toolButton->isEnabled())
                                    {
                                        action->setEnabled(false);
                                    }

                                    connect(action, &QAction::triggered, toolButton, &QToolButton::clicked);

                                    if (toolButton->isCheckable())
                                    {
                                        action->setCheckable(true);
                                    }

                                    action->setChecked(toolButton->isChecked());

                                    menu->addAction(action);
                                }
                            }
                            else if (auto toolCombo = qobject_cast<QComboBox*>(toolWidget))
                            {
                                // Add custom menu for Speed
                                if (toolCombo->objectName() == "m_moveSpeed")
                                {
                                    double currentValue = toolCombo->lineEdit()->text().toDouble();

                                    QMenu* newMenu = menu->addMenu(QString("Speed: %1").arg(currentValue));

                                    double presets[] = { 0.1, 1.0, 10.0 };
                                    for (double preset : presets)
                                    {
                                        QAction* presetAction = new QAction(newMenu);
                                        presetAction->setText(QString::number(preset));

                                        connect(presetAction, &QAction::triggered, this, [preset, this]() {
                                            if (m_infoBar)
                                            {
                                                m_infoBar->SetSpeedComboBox(preset);
                                            }
                                        });

                                        newMenu->addAction(presetAction);
                                    }
                                }
                            }
                        }
                    }

                    menu->exec(mouseEvent->globalPos());
                    return true;
                }

                break;
            }
        }

        return QObject::eventFilter(obj, event);
    }

    void SetInfoBar(CInfoBar* infoBar)
    {
        m_infoBar = infoBar;
    }

private:
    CInfoBar* m_infoBar = nullptr;
};

//////////////////////////////////////////////////////////////////////////
// CLayoutWnd
//////////////////////////////////////////////////////////////////////////
CLayoutWnd::CLayoutWnd(QSettings* settings, QWidget* parent)
    : AzQtComponents::ToolBarArea(parent)
    , m_settings(settings)
    , m_expanderWatcher(new InfoBarExpanderWatcher(this))
{
    m_bMaximized = false;
    m_maximizedView = 0;
    m_layout = (EViewLayout) - 1;

    m_maximizedViewId = 0;
    m_infoBarSize = QSize(0, 0);

    m_infoBar = new CInfoBar(this);
    connect(qApp, &QApplication::focusChanged, this, &CLayoutWnd::OnFocusChanged);

    m_expanderWatcher->SetInfoBar(m_infoBar);

    m_infoToolBar = CreateToolBarFromWidget(m_infoBar,
                                            Qt::BottomToolBarArea,
                                            QStringLiteral("Info Panel"));
    m_infoToolBar->setMovable(false);
    m_infoToolBar->setObjectName("InfoBar");
    AzQtComponents::Style::addClass(m_infoToolBar, "DefaultSpacing");

    if (QToolButton* expansion = AzQtComponents::ToolBar::getToolBarExpansionButton(m_infoToolBar))
    {
        expansion->installEventFilter(m_expanderWatcher);
    }

    setContextMenuPolicy(Qt::NoContextMenu);
}

//////////////////////////////////////////////////////////////////////////
CLayoutWnd::~CLayoutWnd()
{
    delete m_splitWnd;
    delete m_splitWnd2;
    delete m_splitWnd3;

    OnDestroy();
}

//////////////////////////////////////////////////////////////////////////
void CLayoutWnd::UnbindViewports()
{
    // First unbind all views.
    for (int i = 0; i < MAX_VIEWPORTS; i++)
    {
        CLayoutViewPane* pViewPane = GetViewPane(i);
        if (pViewPane)
        {
            pViewPane->ReleaseViewport();
        }
    }

    if (m_maximizedView)
    {
        m_maximizedView->ReleaseViewport();
    }
}

//////////////////////////////////////////////////////////////////////////
void CLayoutWnd::BindViewports()
{
    // First unbind all views.
    UnbindViewports();

    for (int i = 0; i < MAX_VIEWPORTS; i++)
    {
        CLayoutViewPane* pViewPane = GetViewPane(i);
        if (pViewPane)
        {
            BindViewport(pViewPane, m_viewType[pViewPane->GetId()]);
        }
    }

    FocusFirstLayoutViewPane(m_splitWnd);
}

//////////////////////////////////////////////////////////////////////////
void CLayoutWnd::BindViewport(CLayoutViewPane* vp, const QString& viewClassName, QWidget* pViewport)
{
    assert(vp);
    if (!pViewport)
    {
        vp->SetViewClass(viewClassName);
    }
    else
    {
        vp->AttachViewport(pViewport);
    }
    vp->setVisible(true);
    m_viewType[vp->GetId()] = viewClassName;
}

//////////////////////////////////////////////////////////////////////////
void CLayoutWnd::MaximizeViewport(int paneId)
{
    // Ignore with full screen layout.
    if (m_layout == ET_Layout0 && m_bMaximized)
    {
        return;
    }

    QString viewClass = m_viewType[paneId];

    const QRect rc = rect();
    if (!m_bMaximized)
    {
        CLayoutViewPane* pViewPane = GetViewPane(paneId);
        m_maximizedViewId = paneId;
        m_bMaximized = true;

        if (m_maximizedView)
        {
            if (m_splitWnd)
            {
                m_splitWnd->setVisible(false);
            }

            if (pViewPane)
            {
                MoveViewport(pViewPane, m_maximizedView, viewClass);
            }
            else
            {
                BindViewport(m_maximizedView, viewClass);
            }
            m_maximizedView->setFocus();

            SetMainWidget(m_maximizedView);
            m_maximizedView->setVisible(true);

            MainWindow::instance()->SetActiveView(m_maximizedView);
        }
    }
    else
    {
        CLayoutViewPane* pViewPane = GetViewPane(m_maximizedViewId);
        m_bMaximized = false;
        m_maximizedViewId = 0;

        if (pViewPane && m_maximizedView)
        {
            MoveViewport(m_maximizedView, pViewPane, viewClass);
        }

        if (m_maximizedView)
        {
            m_maximizedView->setVisible(false);
        }

        if (m_splitWnd)
        {
            m_splitWnd->setVisible(true);
            SetMainWidget(m_splitWnd);
            FocusFirstLayoutViewPane(m_splitWnd);
        }
    }
}

QString CLayoutWnd::ViewportTypeToClassName(EViewportType viewType)
{
    QtViewPane* pane = QtViewPaneManager::instance()->GetViewportPane(viewType);
    return pane ? pane->m_name : QString();
}

void CLayoutWnd::CreateLayoutView(CLayoutSplitter* wndSplitter, int row, int col, int id, EViewportType viewType)
{
    QString viewClassName = ViewportTypeToClassName(viewType);
    wndSplitter->CreateLayoutView(row, col, id);
    m_viewType[id] = viewClassName;
}

//////////////////////////////////////////////////////////////////////////
void CLayoutWnd::CreateLayout(EViewLayout layout, bool bBindViewports, EViewportType defaultView)
{
    UnbindViewports();

    m_layout = layout;
    m_bMaximized = false;

    if (m_splitWnd)
    {
        m_splitWnd->setVisible(false);
        delete m_splitWnd;
    }

    delete m_splitWnd2;
    delete m_splitWnd3;

    if (m_maximizedView)
    {
        m_maximizedView->setVisible(false);
    }

    QRect rcView = rect();
    rcView.setBottom(rcView.bottom() - m_infoBar->height());

    if (m_maximizedView)
        m_maximizedView->deleteLater();

    m_maximizedView = new CLayoutViewPane(this);
    m_maximizedView->SetId(0);
    m_maximizedView->setGeometry(rcView);
    m_maximizedView->setVisible(false);
    m_maximizedView->SetFullscren(true);

    if (AZ::Interface<AzFramework::AtomActiveInterface>::Get()) // Only supports 1 viewport for now.
    {
        m_viewType[0] = ViewportTypeToClassName(defaultView);
        if (bBindViewports)
        {
            MaximizeViewport(0);
        }
    }
    else
    {
        switch (layout)
        {
        case ET_Layout0:
            m_viewType[0] = ViewportTypeToClassName(defaultView);
            if (bBindViewports)
            {
                MaximizeViewport(0);
            }
            break;

        case ET_Layout1:
            m_splitWnd = new CLayoutSplitter(this);
            m_splitWnd->setOrientation(Qt::Horizontal);
    #ifdef FEATURE_ORTHOGRAPHIC_VIEW
            CreateLayoutView(m_splitWnd, 0, 0, 2, ET_ViewportXY);
    #else
            CreateLayoutView(m_splitWnd, 0, 0, 2, ET_ViewportCamera);
    #endif
            CreateLayoutView(m_splitWnd, 0, 1, 1, defaultView);
            break;
        case ET_Layout2:
            m_splitWnd = new CLayoutSplitter(this);
            m_splitWnd->setOrientation(Qt::Vertical);
    #ifdef FEATURE_ORTHOGRAPHIC_VIEW
            CreateLayoutView(m_splitWnd, 0, 0, 2, ET_ViewportXY);
    #else
            CreateLayoutView(m_splitWnd, 0, 0, 2, ET_ViewportCamera);
    #endif
            CreateLayoutView(m_splitWnd, 1, 0, 1, defaultView);
            break;

        case ET_Layout3:
            m_splitWnd = new CLayoutSplitter(this);
            m_splitWnd->setOrientation(Qt::Horizontal);
            CreateLayoutView(m_splitWnd, 0, 1, 1, defaultView);

            m_splitWnd2 = new CLayoutSplitter;
            m_splitWnd2->setOrientation(Qt::Vertical);
            m_splitWnd->insertWidget(0, m_splitWnd2);
    #ifdef FEATURE_ORTHOGRAPHIC_VIEW
            CreateLayoutView(m_splitWnd2, 0, 0, 2, ET_ViewportXY);
            CreateLayoutView(m_splitWnd2, 1, 0, 3, ET_ViewportXZ);
    #else
            CreateLayoutView(m_splitWnd2, 0, 0, 2, ET_ViewportCamera);
            CreateLayoutView(m_splitWnd2, 1, 0, 3, ET_ViewportCamera);
    #endif
            break;

        case ET_Layout4:
            m_splitWnd = new CLayoutSplitter(this);
            m_splitWnd->setOrientation(Qt::Horizontal);
            CreateLayoutView(m_splitWnd, 0, 0, 1, defaultView);

            m_splitWnd2 = new CLayoutSplitter;
            m_splitWnd2->setOrientation(Qt::Vertical);
            m_splitWnd->insertWidget(1, m_splitWnd2);
    #ifdef FEATURE_ORTHOGRAPHIC_VIEW
            CreateLayoutView(m_splitWnd2, 0, 0, 2, ET_ViewportXY);
            CreateLayoutView(m_splitWnd2, 1, 0, 3, ET_ViewportXZ);
    #else
            CreateLayoutView(m_splitWnd2, 0, 0, 2, ET_ViewportCamera);
            CreateLayoutView(m_splitWnd2, 1, 0, 3, ET_ViewportCamera);
    #endif
            break;

        case ET_Layout5:
            m_splitWnd = new CLayoutSplitter(this);
            m_splitWnd->setOrientation(Qt::Vertical);
            CreateLayoutView(m_splitWnd, 1, 0, 1, defaultView);

            m_splitWnd2 = new CLayoutSplitter;
            m_splitWnd2->setOrientation(Qt::Horizontal);
            m_splitWnd->insertWidget(0, m_splitWnd2);
    #ifdef FEATURE_ORTHOGRAPHIC_VIEW
            CreateLayoutView(m_splitWnd2, 0, 0, 2, ET_ViewportXY);
            CreateLayoutView(m_splitWnd2, 0, 1, 3, ET_ViewportXZ);
    #else
            CreateLayoutView(m_splitWnd2, 0, 0, 2, ET_ViewportCamera);
            CreateLayoutView(m_splitWnd2, 0, 1, 3, ET_ViewportCamera);
    #endif
            break;

        case ET_Layout6:
            m_splitWnd = new CLayoutSplitter(this);
            m_splitWnd->setOrientation(Qt::Vertical);
            CreateLayoutView(m_splitWnd, 0, 0, 1, defaultView);

            m_splitWnd2 = new CLayoutSplitter;
            m_splitWnd->insertWidget(1, m_splitWnd2);
    #ifdef FEATURE_ORTHOGRAPHIC_VIEW
            CreateLayoutView(m_splitWnd2, 0, 0, 2, ET_ViewportXY);
            CreateLayoutView(m_splitWnd2, 0, 1, 3, ET_ViewportXZ);
    #else
            CreateLayoutView(m_splitWnd2, 0, 0, 2, ET_ViewportCamera);
            CreateLayoutView(m_splitWnd2, 0, 1, 3, ET_ViewportCamera);
    #endif
            break;

        case ET_Layout7:
            m_splitWnd = new CLayoutSplitter(this);
            m_splitWnd->setOrientation(Qt::Horizontal);
            m_splitWnd2 = new CLayoutSplitter;
            m_splitWnd2->setOrientation(Qt::Vertical);
            m_splitWnd3 = new CLayoutSplitter;
            m_splitWnd3->setOrientation(Qt::Vertical);
            m_splitWnd->addWidget(m_splitWnd2);
            m_splitWnd->addWidget(m_splitWnd3);
    #ifdef FEATURE_ORTHOGRAPHIC_VIEW
            CreateLayoutView(m_splitWnd2, 0, 0, 2, ET_ViewportXZ);
            CreateLayoutView(m_splitWnd3, 0, 1, 3, ET_ViewportYZ);
            CreateLayoutView(m_splitWnd2, 1, 0, 4, ET_ViewportXY);
    #else
            CreateLayoutView(m_splitWnd2, 0, 0, 2, ET_ViewportCamera);
            CreateLayoutView(m_splitWnd3, 0, 1, 3, ET_ViewportCamera);
            CreateLayoutView(m_splitWnd2, 1, 0, 4, ET_ViewportCamera);
    #endif
            CreateLayoutView(m_splitWnd3, 1, 1, 1, defaultView);
            connect(m_splitWnd2, &QSplitter::splitterMoved, m_splitWnd3, &CLayoutSplitter::otherSplitterMoved);
            connect(m_splitWnd3, &QSplitter::splitterMoved, m_splitWnd2, &CLayoutSplitter::otherSplitterMoved);
            break;

        case ET_Layout8:
            m_splitWnd = new CLayoutSplitter(this);
            m_splitWnd->setOrientation(Qt::Vertical);
            CreateLayoutView(m_splitWnd, 1, 0, 1, defaultView);

            m_splitWnd2 = new CLayoutSplitter;
            m_splitWnd2->setOrientation(Qt::Horizontal);
            m_splitWnd->insertWidget(0, m_splitWnd2);
    #ifdef FEATURE_ORTHOGRAPHIC_VIEW
            CreateLayoutView(m_splitWnd2, 0, 0, 2, ET_ViewportXY);
            CreateLayoutView(m_splitWnd2, 0, 1, 3, ET_ViewportXZ);
            CreateLayoutView(m_splitWnd2, 0, 2, 4, ET_ViewportYZ);
    #else
            CreateLayoutView(m_splitWnd2, 0, 0, 2, ET_ViewportCamera);
            CreateLayoutView(m_splitWnd2, 0, 1, 3, ET_ViewportCamera);
            CreateLayoutView(m_splitWnd2, 0, 2, 4, ET_ViewportCamera);
    #endif
            break;

        default:
            CLogFile::FormatLine("Trying to Create Unknown Layout %d", (int)layout);
            QMessageBox::critical(this, QString(), tr("Trying to Create Unknown Layout"));
            break;
        }
        ;
    }
    if (m_splitWnd)
    {
        m_splitWnd->setGeometry(rcView);
        m_splitWnd->setVisible(true);
        FocusFirstLayoutViewPane(m_splitWnd);
        SetMainWidget(m_splitWnd);
    }

    if (bBindViewports && !m_bMaximized)
    {
        BindViewports();
    }
}

//////////////////////////////////////////////////////////////////////////
void CLayoutWnd::SaveConfig()
{
    QSettings settings;
    settings.beginGroup(GetConfigGroupName());
    settings.setValue("Layout", static_cast<int>(m_layout));
    settings.setValue("Maximized", m_maximizedViewId);

    QString str;
    for (int i = 1; i < MAX_VIEWPORTS; i++)
    {
        str += QString::fromLatin1("%1,").arg(m_viewType[i]);
    }
    settings.setValue("Viewports", str);
}

//////////////////////////////////////////////////////////////////////////
bool CLayoutWnd::LoadConfig()
{
    QSettings settings;
    settings.beginGroup(GetConfigGroupName());
    int layout = settings.value("Layout", -1).toInt();
    int maximizedView = settings.value("Maximized", 0).toInt();
    if (layout < 0)
    {
        return false;
    }

    CreateLayout((EViewLayout)layout, false);

    bool bRebindViewports = false;
    if (m_splitWnd)
    {
        const QString str = settings.value("Viewports").toString();
        int nIndex = 1;
        int curPos = 0;
        for (auto resToken : str.split(","))
        {
            if (nIndex >= MAX_VIEWPORTS)
            {
                break;
            }
            bRebindViewports = true;
            if (!resToken.isEmpty())
            {
                m_viewType[nIndex] = resToken;
            }
            nIndex++;
        }
        ;
    }

    BindViewports();

    if (maximizedView || layout == ET_Layout0)
    {
        MaximizeViewport(maximizedView);
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
const char* CLayoutWnd::GetConfigGroupName()
{
    return "ViewportLayout";
}

unsigned int CLayoutWnd::GetViewPaneCount()
{
    // If there's a maximized viewport, then exactly 1 is visible.
    if (m_bMaximized)
    {
        return 1;
    }

    unsigned int numPanes = 0;

    // Otherwise, go searching through and count how many are visible.
    for (QSplitter* splitter : { m_splitWnd, m_splitWnd2, m_splitWnd3 })
    {
        if (splitter)
        {
            for (int i = 0; i < splitter->count(); ++i)
            {
                if (qobject_cast<CLayoutViewPane*>(splitter->widget(i)))
                {
                    numPanes++;
                }
            }
        }
    }

    return numPanes;
}

CLayoutViewPane* CLayoutWnd::GetViewPaneByIndex(unsigned int index)
{
    // If there's a maximized viewport, return it if index 0 is requested.
    // (All other index values are invalid in this state)
    if (m_bMaximized)
    {
        if (index == 0)
        {
            return m_maximizedView;
        }
        return nullptr;
    }

    unsigned int curIndex = 0;

    // Otherwise, go searching through until we've found the proper number of panes
    // and return the one at the correct index.
    for (QSplitter* splitter : { m_splitWnd, m_splitWnd2, m_splitWnd3 })
    {
        if (splitter)
        {
            for (int i = 0; i < splitter->count(); ++i)
            {
                if (QWidget* widget = splitter->widget(i))
                {
                    if (CLayoutViewPane* pane = qobject_cast<CLayoutViewPane*>(widget))
                    {
                        if (curIndex == index)
                        {
                            return pane;
                        }
                        curIndex++;
                    }
                }
            }
        }
    }

    return nullptr;
}


//////////////////////////////////////////////////////////////////////////
CLayoutViewPane* CLayoutWnd::GetViewPane(int id)
{
    QList<QSplitter*> splitters({ m_splitWnd, m_splitWnd2, m_splitWnd3 });

    for (QSplitter* splitter : splitters)
    {
        if (!splitter)
        {
            continue;
        }

        for (int i = 0; i < splitter->count(); ++i)
        {
            QWidget* widget = splitter->widget(i);
            if (widget == nullptr)
            {
                continue;
            }
            if (CLayoutViewPane* pane = qobject_cast<CLayoutViewPane*>(widget))
            {
                if (pane && pane->GetId() == id)
                {
                    return pane;
                }
            }
        }
    }

    return nullptr;
}

//////////////////////////////////////////////////////////////////////////
CLayoutViewPane* CLayoutWnd::FindViewByClass(const QString& viewClassName)
{
    if (m_viewType[0] == viewClassName)
    {
        return m_maximizedView;
    }

    // Starts from 1, 0 is the maximized view.
    for (int i = 1; i < MAX_VIEWPORTS; i++)
    {
        if (m_viewType[i] == viewClassName)
        {
            return GetViewPane(i);
        }
    }
    return nullptr;
}

//////////////////////////////////////////////////////////////////////////
bool CLayoutWnd::CycleViewport(EViewportType from, EViewportType to)
{
    const QString viewClassName = ViewportTypeToClassName(from);
    CLayoutViewPane* vp = FindViewByClass(viewClassName);
    if (m_layout == ET_Layout0 && !vp)
    {
        if (m_maximizedView)
        {
            if (m_maximizedView->GetViewClass() == viewClassName)
            {
                vp = m_maximizedView;
            }
        }
    }
    if (vp)
    {
        BindViewport(vp, ViewportTypeToClassName(to));
        return true;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
void CLayoutWnd::ResetLayout()
{
    // reset the layout settings. Mfc doesn't have a mechanism to delete settings, that I can find, so we use Qt.
    m_settings->beginGroup("Editor");
    m_settings->remove(GetConfigGroupName());
    m_settings->endGroup();

    // restore the default layout
    CreateLayout(ET_Layout0);
}

//////////////////////////////////////////////////////////////////////////
void CLayoutWnd::Cycle2DViewport()
{
    // Cycle between 3 2D viewports.
    switch (m_layout)
    {
    case ET_Layout0:
        if (CycleViewport(ET_ViewportCamera, ET_ViewportXY))
        {
            return;
        }
        if (CycleViewport(ET_ViewportXY, ET_ViewportXZ))
        {
            return;
        }
        if (CycleViewport(ET_ViewportXZ, ET_ViewportYZ))
        {
            return;
        }
        if (CycleViewport(ET_ViewportYZ, ET_ViewportCamera))
        {
            return;
        }
        break;

    default:
        if (CycleViewport(ET_ViewportXY, ET_ViewportXZ))
        {
            return;
        }
        if (CycleViewport(ET_ViewportXZ, ET_ViewportYZ))
        {
            return;
        }
        if (CycleViewport(ET_ViewportYZ, ET_ViewportXY))
        {
            return;
        }
        break;
    }
}

//////////////////////////////////////////////////////////////////////////
void CLayoutWnd::OnDestroy()
{
    if (m_maximizedView)
    {
        delete m_maximizedView;
        m_maximizedView = 0;
    }
}

void CLayoutWnd::FocusFirstLayoutViewPane(CLayoutSplitter* splitter)
{
    // When starting in multi-layout mode we focus the first CLayoutViewPane
    // Note that splitter->widget(0) might be another splitter, not a CLayoutViewPane
    if (splitter)
    {
        if (auto view = splitter->findChild<CLayoutViewPane*>())
        {
            view->setFocus();
            MainWindow::instance()->SetActiveView(view);
        }
    }
}

void CLayoutWnd::MoveViewport(CLayoutViewPane* from, CLayoutViewPane* to, const QString& viewClassName)
{
    // First detach from old pane, allowing the viewport to be disconnected from the event bus
    // This must be done before re-binding the viewport and connecting to the bus with a new id
    auto viewport = from->GetViewport();
    from->DetachViewport();
    BindViewport(to, viewClassName, viewport);
}

static CLayoutViewPane* layoutViewPaneForChild(QObject* child)
{
    CLayoutViewPane* result = nullptr;

    while (child)
    {
        result = qobject_cast<CLayoutViewPane*>(child);
        if (result)
        {
            break;
        }
        child = child->parent();
    }

    return result;
}

void CLayoutWnd::OnFocusChanged(QWidget* /* oldWidget */, QWidget* newWidget)
{
    if (CLayoutViewPane* layoutViewPane = layoutViewPaneForChild(newWidget))
    {
        MainWindow::instance()->SetActiveView(layoutViewPane);
    }
}

#include <moc_LayoutWnd.cpp>
