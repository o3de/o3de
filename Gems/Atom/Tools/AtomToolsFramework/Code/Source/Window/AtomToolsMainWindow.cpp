/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/Window/AtomToolsMainWindow.h>

namespace AtomToolsFramework
{
    AtomToolsMainWindow::AtomToolsMainWindow(QWidget* parent)
        : AzQtComponents::DockMainWindow(parent)
    {
        m_advancedDockManager = new AzQtComponents::FancyDocking(this);

        setDockNestingEnabled(true);
        setCorner(Qt::TopLeftCorner, Qt::LeftDockWidgetArea);
        setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
        setCorner(Qt::TopRightCorner, Qt::RightDockWidgetArea);
        setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);

        m_statusBar = new QStatusBar(this);
        m_statusBar->setObjectName("StatusBar");
        statusBar()->addPermanentWidget(m_statusBar, 1);

        m_centralWidget = new QWidget(this);

        AtomToolsMainWindowRequestBus::Handler::BusConnect();
    }

    AtomToolsMainWindow::~AtomToolsMainWindow()
    {
        AtomToolsMainWindowRequestBus::Handler::BusDisconnect();
    }

    void AtomToolsMainWindow::ActivateWindow()
    {
        activateWindow();
        raise();
    }

    bool AtomToolsMainWindow::AddDockWidget(const AZStd::string& name, QWidget* widget, uint32_t area, uint32_t orientation)
    {
        auto dockWidgetItr = m_dockWidgets.find(name);
        if (dockWidgetItr != m_dockWidgets.end() || !widget)
        {
            return false;
        }

        auto dockWidget = new AzQtComponents::StyledDockWidget(name.c_str());
        dockWidget->setObjectName(QString("%1_DockWidget").arg(name.c_str()));
        dockWidget->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetMovable);
        widget->setObjectName(name.c_str());
        widget->setParent(dockWidget);
        widget->setMinimumSize(QSize(300, 300));
        dockWidget->setWidget(widget);
        addDockWidget(aznumeric_cast<Qt::DockWidgetArea>(area), dockWidget);
        resizeDocks({ dockWidget }, { 400 }, aznumeric_cast<Qt::Orientation>(orientation));
        m_dockWidgets[name] = dockWidget;
        return true;
    }

    void AtomToolsMainWindow::RemoveDockWidget(const AZStd::string& name)
    {
        auto dockWidgetItr = m_dockWidgets.find(name);
        if (dockWidgetItr != m_dockWidgets.end())
        {
            delete dockWidgetItr->second;
            m_dockWidgets.erase(dockWidgetItr);
        }
    }

    void AtomToolsMainWindow::SetDockWidgetVisible(const AZStd::string& name, bool visible)
    {
        auto dockWidgetItr = m_dockWidgets.find(name);
        if (dockWidgetItr != m_dockWidgets.end())
        {
            dockWidgetItr->second->setVisible(visible);
        }
    }

    bool AtomToolsMainWindow::IsDockWidgetVisible(const AZStd::string& name) const
    {
        auto dockWidgetItr = m_dockWidgets.find(name);
        if (dockWidgetItr != m_dockWidgets.end())
        {
            return dockWidgetItr->second->isVisible();
        }
        return false;
    }

    AZStd::vector<AZStd::string> AtomToolsMainWindow::GetDockWidgetNames() const
    {
        AZStd::vector<AZStd::string> names;
        names.reserve(m_dockWidgets.size());
        for (const auto& dockWidgetPair : m_dockWidgets)
        {
            names.push_back(dockWidgetPair.first);
        }
        return names;
    }

    void AtomToolsMainWindow::CreateMenu()
    {
        m_menuBar = new QMenuBar(this);
        m_menuBar->setObjectName("MenuBar");
        setMenuBar(m_menuBar);
    }

    void AtomToolsMainWindow::CreateTabBar()
    {
        m_tabWidget = new AzQtComponents::TabWidget(m_centralWidget);
        m_tabWidget->setObjectName("TabWidget");
        m_tabWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
        m_tabWidget->setContentsMargins(0, 0, 0, 0);

        // The tab bar should only be visible if it has active documents
        m_tabWidget->setVisible(false);
        m_tabWidget->setTabBarAutoHide(false);
        m_tabWidget->setMovable(true);
        m_tabWidget->setTabsClosable(true);
        m_tabWidget->setUsesScrollButtons(true);

        // Add context menu for right-clicking on tabs
        m_tabWidget->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
        connect(
            m_tabWidget, &QWidget::customContextMenuRequested, this,
            [this]()
            {
                OpenTabContextMenu();
            });
    }

    void AtomToolsMainWindow::AddTabForDocumentId(
        const AZ::Uuid& documentId, const AZStd::string& label, const AZStd::string& toolTip, AZStd::function<QWidget*()> widgetCreator)
    {
        // Blocking signals from the tab bar so the currentChanged signal is not sent while a document is already being opened.
        // This prevents the OnDocumentOpened notification from being sent recursively.
        const QSignalBlocker blocker(m_tabWidget);

        // If a tab for this document already exists then select it instead of creating a new one
        for (int tabIndex = 0; tabIndex < m_tabWidget->count(); ++tabIndex)
        {
            if (documentId == GetDocumentIdFromTab(tabIndex))
            {
                m_tabWidget->setCurrentIndex(tabIndex);
                m_tabWidget->repaint();
                return;
            }
        }

        const int tabIndex = m_tabWidget->addTab(widgetCreator(), label.c_str());

        // The user can manually reorder tabs which will invalidate any association by index.
        // We need to store the document ID with the tab using the tab instead of a separate mapping.
        m_tabWidget->tabBar()->setTabData(tabIndex, QVariant(documentId.ToString<QString>()));
        m_tabWidget->setTabToolTip(tabIndex, toolTip.c_str());
        m_tabWidget->setCurrentIndex(tabIndex);
        m_tabWidget->setVisible(true);
        m_tabWidget->repaint();
    }

    void AtomToolsMainWindow::RemoveTabForDocumentId(const AZ::Uuid& documentId)
    {
        // We are not blocking signals here because we want closing tabs to close the associated document
        // and automatically select the next document.
        for (int tabIndex = 0; tabIndex < m_tabWidget->count(); ++tabIndex)
        {
            if (documentId == GetDocumentIdFromTab(tabIndex))
            {
                m_tabWidget->removeTab(tabIndex);
                m_tabWidget->setVisible(m_tabWidget->count() > 0);
                m_tabWidget->repaint();
                break;
            }
        }
    }

    void AtomToolsMainWindow::UpdateTabForDocumentId(
        const AZ::Uuid& documentId, const AZStd::string& label, const AZStd::string& toolTip, bool isModified)
    {
        // Whenever a document is opened, saved, or modified we need to update the tab label
        if (!documentId.IsNull())
        {
            // Because tab order and indexes can change from user interactions, we cannot store a map
            // between a tab index and document ID.
            // We must iterate over all of the tabs to find the one associated with this document.
            for (int tabIndex = 0; tabIndex < m_tabWidget->count(); ++tabIndex)
            {
                if (documentId == GetDocumentIdFromTab(tabIndex))
                {
                    // We use an asterisk appended to the file name to denote modified document
                    const AZStd::string modifiedLabel = isModified ? label + " *" : label;
                    m_tabWidget->setTabText(tabIndex, modifiedLabel.c_str());
                    m_tabWidget->setTabToolTip(tabIndex, toolTip.c_str());
                    m_tabWidget->repaint();
                    break;
                }
            }
        }
    }

    AZ::Uuid AtomToolsMainWindow::GetDocumentIdFromTab(const int tabIndex) const
    {
        const QVariant tabData = m_tabWidget->tabBar()->tabData(tabIndex);
        if (!tabData.isNull())
        {
            // We need to be able to convert between a UUID and a string to store and retrieve a document ID from the tab bar
            const QString documentIdString = tabData.toString();
            const QByteArray documentIdBytes = documentIdString.toUtf8();
            const AZ::Uuid documentId(documentIdBytes.data(), documentIdBytes.size());
            return documentId;
        }
        return AZ::Uuid::CreateNull();
    }

    void AtomToolsMainWindow::OpenTabContextMenu()
    {
    }
    
    void AtomToolsMainWindow::SelectPreviousTab()
    {
        if (m_tabWidget->count() > 1)
        {
            // Adding count to wrap around when index <= 0
            m_tabWidget->setCurrentIndex((m_tabWidget->currentIndex() + m_tabWidget->count() - 1) % m_tabWidget->count());
        }
    }

    void AtomToolsMainWindow::SelectNextTab()
    {
        if (m_tabWidget->count() > 1)
        {
            m_tabWidget->setCurrentIndex((m_tabWidget->currentIndex() + 1) % m_tabWidget->count());
        }
    }
} // namespace AtomToolsFramework
