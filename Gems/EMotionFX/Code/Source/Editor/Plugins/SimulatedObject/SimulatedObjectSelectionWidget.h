/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/std/containers/vector.h>
#include <MCore/Source/StandardHeaders.h>
#include <EMotionFX/CommandSystem/Source/SelectionCommands.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/StandardPluginsConfig.h>
#include <QDialog>
#endif


// forward declarations
QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QIcon)
QT_FORWARD_DECLARE_CLASS(QTreeWidget)
QT_FORWARD_DECLARE_CLASS(QTreeWidgetItem)

namespace AzQtComponents
{
    class FilteredSearchWidget;
}

namespace EMotionFX
{
    class SimulatedObject;
}

namespace EMStudio
{
    class SimulatedObjectSelectionWidget
        : public QWidget
    {
        Q_OBJECT // AUTOMOC
        MCORE_MEMORYOBJECTCATEGORY(SimulatedObjectSelectionWidget, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH)

    public:
        SimulatedObjectSelectionWidget(QWidget* parent);

        void Update(EMotionFX::Actor* actor, const AZStd::vector<AZStd::string>& selectedSimulatedObjectNames);
        QTreeWidget* GetTreeWidget()                                                               { return m_treeWidget; }
        AzQtComponents::FilteredSearchWidget* GetSearchWidget()                                    { return m_searchWidget; }

        // This calls UpdateSelection() and then returns the member array containing the selected items
        AZStd::vector<AZStd::string>& GetSelectedSimulatedObjectNames();

    signals:
        void OnSelectionDone(const AZStd::vector<AZStd::string>& selectedItems);
        void OnDoubleClicked(const AZStd::string& item);

    public slots:
        void Update();
        void UpdateSelection();
        void ItemDoubleClicked(QTreeWidgetItem* item, int column);
        void OnTextFilterChanged(const QString& text);

    private:
        void AddSimulatedObjectToInterface(const EMotionFX::SimulatedObject* object);

        EMotionFX::Actor*                       m_actor = nullptr;
        QTreeWidget*                            m_treeWidget = nullptr;
        AzQtComponents::FilteredSearchWidget*   m_searchWidget = nullptr;
        AZStd::string                           m_searchWidgetText;
        AZStd::vector<AZStd::string>            m_selectedSimulatedObjectNames;
        AZStd::vector<AZStd::string>            m_oldSelectedSimulatedObjectNames;
    };
} // namespace EMStudio
