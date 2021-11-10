/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <MCore/Source/StandardHeaders.h>
#include <EMotionFX/CommandSystem/Source/SelectionCommands.h>
#include "EMStudioConfig.h"
#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
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

struct EMSTUDIO_API SelectionItem
{
    uint32          m_actorInstanceId;
    uint32          m_nodeNameId;
    uint32          m_morphTargetId;

    SelectionItem()
    {
        m_actorInstanceId    = MCORE_INVALIDINDEX32;
        m_nodeNameId         = MCORE_INVALIDINDEX32;
        m_morphTargetId      = MCORE_INVALIDINDEX32;
    }

    SelectionItem(const uint32 actorInstanceID, const char* nodeName, const uint32 morphTargetID = MCORE_INVALIDINDEX32)
        : m_actorInstanceId(actorInstanceID), m_morphTargetId(morphTargetID)
    {
        SetNodeName(nodeName);
    }

    void SetNodeName(const char* nodeName)             { m_nodeNameId = MCore::GetStringIdPool().GenerateIdForString(nodeName); }
    const char* GetNodeName() const                    { return MCore::GetStringIdPool().GetName(m_nodeNameId).c_str(); }
    const AZStd::string& GetNodeNameString() const     { return MCore::GetStringIdPool().GetName(m_nodeNameId); }

    EMotionFX::Node* GetNode() const;
};

namespace EMStudio
{
    class EMSTUDIO_API NodeHierarchyWidget
        : public QWidget
    {
        Q_OBJECT
        MCORE_MEMORYOBJECTCATEGORY(NodeHierarchyWidget, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_EMSTUDIOSDK)

    public:
        NodeHierarchyWidget(QWidget* parent, bool useSingleSelection, bool useDefaultMinWidth = true);
        virtual ~NodeHierarchyWidget();

        void SetSelectionMode(bool useSingleSelection);
        void Update(uint32 actorInstanceID, CommandSystem::SelectionList* selectionList = nullptr);
        void Update(const AZStd::vector<uint32>& actorInstanceIDs, CommandSystem::SelectionList* selectionList = nullptr);
        void FireSelectionDoneSignal();
        MCORE_INLINE QTreeWidget* GetTreeWidget()                                                               { return m_hierarchy; }
        MCORE_INLINE AzQtComponents::FilteredSearchWidget* GetSearchWidget()                                    { return m_searchWidget; }

        // is node shown in the hierarchy widget?
        bool CheckIfNodeVisible(EMotionFX::ActorInstance* actorInstance, EMotionFX::Node* node);
        bool CheckIfNodeVisible(const AZStd::string& nodeName, bool isMeshNode, bool isBone, bool isNode);

        // this calls UpdateSelection() and then returns the member array containing the selected items
        AZStd::vector<SelectionItem>& GetSelectedItems();

        const AZStd::string& GetSearchWidgetText() const                                                        { return m_searchWidgetText; }
        bool GetDisplayNodes() const;
        bool GetDisplayBones() const;
        bool GetDisplayMeshes() const;

        bool CheckIfNodeSelected(const char* nodeName, uint32 actorInstanceID);
        bool CheckIfActorInstanceSelected(uint32 actorInstanceID);

        enum class FilterType
        {
            Meshes  = 1,
            Nodes   = 2,
            Bones   = 4,
        };
        Q_DECLARE_FLAGS(FilterTypes, FilterType)

    signals:
        void OnSelectionDone(AZStd::vector<SelectionItem> selectedNodes);
        void OnDoubleClicked(AZStd::vector<SelectionItem> selectedNodes);

        void SelectionChanged();

        void FilterStateChanged(const FilterTypes& filterState);

    public slots:
        //void OnVisibilityChanged(bool isVisible);
        void Update();
        void UpdateSelection();
        void ItemDoubleClicked(QTreeWidgetItem* item, int column);
        void TreeContextMenu(const QPoint& pos);
        void OnTextFilterChanged(const QString& text);

        void OnSelectionChanged();

    private:
        void RecursivelyAddChilds(QTreeWidgetItem* parent, EMotionFX::Actor* actor, EMotionFX::ActorInstance* actorInstance, EMotionFX::Node* node);
        void AddActorInstance(EMotionFX::ActorInstance* actorInstance);

        void ConvertFromSelectionList(CommandSystem::SelectionList* selectionList);
        void RemoveNodeFromSelectedNodes(const char* nodeName, uint32 actorInstanceID);
        void RemoveActorInstanceFromSelectedNodes(uint32 actorInstanceID);
        void AddNodeToSelectedNodes(const char* nodeName, uint32 actorInstanceID);
        void AddNodeToSelectedNodes(const SelectionItem& item);
        void RecursiveRemoveUnselectedItems(QTreeWidgetItem* item);

        AZStd::vector<SelectionItem>        m_selectedNodes;
        QTreeWidget*                        m_hierarchy;
        AzQtComponents::FilteredSearchWidget* m_searchWidget;
        AZStd::string                       m_searchWidgetText;
        QIcon*                              m_boneIcon;
        QIcon*                              m_nodeIcon;
        QIcon*                              m_meshIcon;
        QIcon*                              m_characterIcon;
        AZStd::vector<size_t>                m_boneList;
        AZStd::vector<uint32>                m_actorInstanceIDs;
        AZStd::string                       m_itemName;
        AZStd::string                       m_actorInstanceIdString;
        bool                                m_useSingleSelection;
        FilterTypes                         m_filterState;
    };
} // namespace EMStudio
