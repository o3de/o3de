/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/std/hash.h>
#include <EMotionFX/Source/AnimGraphBus.h>
#include <EMotionFX/Source/AnimGraphNode.h>
#include <EMotionFX/Source/AnimGraphStateTransition.h>
#include <EMotionFX/Source/BlendTreeConnection.h>
#include <MCore/Source/Command.h>
#include <QtCore/QAbstractItemModel>
#include <QtCore/QItemSelectionModel>
#include <Editor/QtMetaTypes.h>
#endif


namespace EMotionFX
{
    class AnimGraphInstance;
}

namespace EMStudio
{
    // This class serves as the model in the MVC pattern to be used by anim graph related UIs
    //
    // This model represents the data in a hierarchy. There are nodes, states, connections, etc. Nodes
    // can contain other nodes, connections. State machine nodes can contain states.
    // Children are considered to be rows within the parent. The parent will have as many rows as
    // children.
    //
    // Each row has two different data dimensions:
    // - columns: currently this model has only 2. The first row represent the element and if used
    //   in a Qt view, it will display the name of the node. For state and connections it will display
    //   an empty string. The second column will show the palette name of the node (empty for connections
    //   and transitions)
    // - roles: we can extract multiple data from each element by using roles. There are roles that are
    //   specific to a type. If the type is unknown, it can be checked with teh ROLE_MODEL_ITEM_TYPE
    //
    //
    class AnimGraphModel
        : public QAbstractItemModel
        , private EMotionFX::AnimGraphNotificationBus::Handler
        , private AZ::Data::AssetBus::Router // to load anim graphs from non-command sources
    {
        Q_OBJECT

    public:
        enum ColumnIndex
        {
            COLUMN_NAME,
            COLUMN_PALETTE_NAME
        };

        enum Role
        {
            // Roles that can be used with any type of item
            //
            // Type of item (check ModelItemType)
            ROLE_MODEL_ITEM_TYPE = Qt::UserRole,
            // ID of the item (e.g. node id, connection id, etc)
            ROLE_ID,
            // A void* to the original item. This is useful for cases where we need to do comparisons
            ROLE_POINTER,
            // The anim graph instance for the item
            ROLE_ANIM_GRAPH_INSTANCE,

            // Roles to be used with nodes and transitions
            //
            // The rtti type of the item (BlendTreeConnection dont have RTTI)
            ROLE_RTTI_TYPE_ID,
            // A pointer to the AnimGraphOject
            ROLE_ANIM_GRAPH_OBJECT_PTR,

            // Roles to only be used with nodes
            //
            // An AnimGraphNode* to the node
            ROLE_NODE_POINTER,
            // If the node can act as state
            ROLE_NODE_CAN_ACT_AS_STATE,
            // Entry node changed (only supported for dataChanged) (only on state machine nodes)
            ROLE_NODE_ENTRY_STATE,

            // Roles to only be used with transitions
            //
            // An AnimGraphStateTransition* to the transition
            ROLE_TRANSITION_POINTER,
            // If the conditions in the transition changed (only supported for dataChanged)
            ROLE_TRANSITION_CONDITIONS,
            // If the trigger actions changed (only supported for dataChanged)
            ROLE_TRIGGER_ACTIONS,

            // Roles to only be used with connections
            //
            // A BlendTreeConnection* to the connection
            ROLE_CONNECTION_POINTER
        };
        enum class ModelItemType
        {
            NODE,
            TRANSITION,
            CONNECTION
        };

        // Model item type for the actual object template specialization.
        template<class ObjectType>
        struct ItemTypeForClass;

        // Model role for the actual object template specialization.
        template<class ObjectType>
        struct RoleForClass;

        AnimGraphModel();
        ~AnimGraphModel() override;

        QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
        QModelIndex parent(const QModelIndex& child) const override;

        int rowCount(const QModelIndex& parent = QModelIndex()) const override;
        int columnCount(const QModelIndex& parent = QModelIndex()) const override;

        QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

        // The focused element is an element that multiple UIs use to locate an element. For example, the BlendGraphViewWidget
        // will show the parent node in the view and center the view on that element. The NavigationWidget will scroll to such
        // element. Note that this does not affect selection, however the element in question could be selected.
        void Focus(const QModelIndex& focusIndex = QModelIndex(), bool forceEmitFocusChangeEvent = false);

        // Finds all the model indexes that are linked to this animGraphObject. Multiple reference nodes could be referencing
        // the same anim graph. Therefore, there could be multiple model indices.
        QModelIndexList FindModelIndexes(EMotionFX::AnimGraphObject* animGraphObject);
        QModelIndexList FindModelIndexes(EMotionFX::BlendTreeConnection* blendTreeConnection);

        // TODO: temporal until all the UI understands QModelIndex and handles selection through the QItemSelectionModel
        // UIs that dont, have to manually set the selection and connect to the QItemSelectionModel::selectionChanged signal
        QModelIndex FindModelIndex(EMotionFX::AnimGraphObject* animGraphObject, EMotionFX::AnimGraphInstance* graphInstance);
        QModelIndex FindFirstModelIndex(EMotionFX::AnimGraphObject* animGraphObject);

        QItemSelectionModel& GetSelectionModel();

        static void AddToItemSelection(QItemSelection& selection, const QModelIndex& modelIndex, bool wasPreviouslySelected, bool isNewlySelected, bool toggleMode, bool clearSelection);
        static QString GetTransitionName(const EMotionFX::AnimGraphStateTransition* transition);

        template<class AnimGraphObjectType>
        AZStd::unordered_map<EMotionFX::AnimGraph*, AZStd::vector<AnimGraphObjectType*> > GetSelectedObjectsOfType() const;

        QModelIndex GetFocus() const { return m_focus; }
        QModelIndex GetParentFocus() const { return m_parentFocus; }
        EMotionFX::AnimGraph* GetFocusedAnimGraph() const;
        // This function will search up to the top level, thus make sure it's not a reference graph but the parent graph. 
        EMotionFX::AnimGraph* FindRootAnimGraph(const QModelIndex& modelIndex) const;

        bool CheckAnySelectedNodeBelongsToReferenceGraph() const;

        // Method to control the anim graph instance stored in the model. These methods are called during activation
        void SetAnimGraphInstance(EMotionFX::AnimGraph* currentAnimGraph, EMotionFX::AnimGraphInstance* currentAnimGraphInstance, EMotionFX::AnimGraphInstance* newAnimGraphInstance);

    signals:
        // Emitted when focus has changed. Index is the element we are focusing on. IndexContainer is the index of a potential
        // parent. UIs use the indexContainer to dive into that node/graph. Is possible that index is the same as indexContainer
        // for the cases where index is a container at the same time. If they are not the same, indexContainer will be the first
        // immediate parent
        void FocusChanged(const QModelIndex& newFocusIndex, const QModelIndex& newFocusParent, const QModelIndex& oldFocusIndex, const QModelIndex& oldFocusParent);

        // Emitted when parameters in an anim graph change. This signal could eventually become more granular. For the time being
        // we need a way to reinit the ParameterWindow when parameter change.
        void ParametersChanged(EMotionFX::AnimGraph* animGraph);

    private:
        void Reset();

        // We want to be able to represent the model even when we dont have an AnimGraphInstance. In those
        // cases we are going to populate the model with a null AnimGraphInstance. If the graph is activated
        // (which creates an instance) then we remove the graph and add the with the AnimGraphInstance.
        QModelIndex Add(EMotionFX::AnimGraph* animGraph);
        void Remove(EMotionFX::AnimGraph* animGraph);

        // AnimGraphNotificationBus
        void OnSyncVisualObject(EMotionFX::AnimGraphObject* object) override;
        void OnReferenceAnimGraphAboutToBeChanged(EMotionFX::AnimGraphReferenceNode* referenceNode) override;
        void OnReferenceAnimGraphChanged(EMotionFX::AnimGraphReferenceNode* referenceNode) override;

        // With reference nodes, an AnimGraphObject could belong to multiple AnimGraphReferenceNodes.
        // If we want to uniquely identify each node we need to do so by a combination of AnimGraphInstance
        // and AnimGraphNode.
        // The remaining data is just a way to keep the track of the relationship between the nodes. We need
        // to have a parent because in the graph, children of AnimGraphReferenceNodes could have multiple parents.
        struct ModelItemData
        {
            ModelItemType m_type;

            union ObjectPtr
            {
                ObjectPtr(EMotionFX::AnimGraphNode* ptr)
                    : m_node(ptr) {
                }
                ObjectPtr(EMotionFX::AnimGraphStateTransition* ptr)
                    : m_transition(ptr) {
                }
                ObjectPtr(EMotionFX::BlendTreeConnection* ptr)
                    : m_connection(ptr) {
                }
                ObjectPtr(void* ptr)
                    : m_ptr(ptr) {
                }

                EMotionFX::AnimGraphNode* m_node;
                EMotionFX::AnimGraphStateTransition* m_transition;
                EMotionFX::BlendTreeConnection* m_connection;
                void* m_ptr;
            };
            ObjectPtr m_object;

            EMotionFX::AnimGraphInstance* m_animGraphInstance;

            ModelItemData*                m_parent;
            int                           m_row;
            AZStd::vector<ModelItemData*> m_children;

            ModelItemData(EMotionFX::AnimGraphNode* animGraphNode, EMotionFX::AnimGraphInstance* animGraphInstance = nullptr, ModelItemData* parent = nullptr, int row = -1);
            ModelItemData(EMotionFX::AnimGraphStateTransition* animGrapStateTransition, EMotionFX::AnimGraphInstance* animGraphInstance = nullptr, ModelItemData* parent = nullptr, int row = -1);
            ModelItemData(EMotionFX::BlendTreeConnection* animGraphConnection, EMotionFX::AnimGraphInstance* animGraphInstance = nullptr, ModelItemData* parent = nullptr, int row = -1);

            // Constructor used for the lookups
            ModelItemData(EMotionFX::AnimGraphInstance* animGraphInstance, void* animGraphPtr)
                : m_animGraphInstance(animGraphInstance)
                , m_object(animGraphPtr)
                , m_parent(nullptr)
                , m_row(-1)
            {}
        };

        // Note that for lookup, we just use the AnimGraphInstance and AnimGraphObject
        //
        struct ModelItemDataComparator
        {
            bool operator()(const ModelItemData* modelItemData1, const ModelItemData* modelItemData2) const
            {
                return modelItemData1->m_animGraphInstance < modelItemData2->m_animGraphInstance
                       || (modelItemData1->m_animGraphInstance == modelItemData2->m_animGraphInstance
                           && modelItemData1->m_object.m_ptr < modelItemData2->m_object.m_ptr);
            }
        };

        ModelItemData* RecursivelyAddNode(EMotionFX::AnimGraphInstance* animGraphInstance, EMotionFX::AnimGraphNode* node, ModelItemData* parent, int row);
        void RecursivelyAddReferenceNodeContents(EMotionFX::AnimGraphInstance* animGraphInstance, EMotionFX::AnimGraphReferenceNode* referenceNode, ModelItemData* referenceNodeModelItemData, int row);
        void AddConnection(EMotionFX::AnimGraphInstance* animGraphInstance, EMotionFX::BlendTreeConnection* connection, ModelItemData* parent, int row);
        void AddTransition(EMotionFX::AnimGraphInstance* animGraphInstance, EMotionFX::AnimGraphStateTransition* transition, ModelItemData* parent, int row);
        void Remove(const QModelIndexList& modelIndexList);
        void Remove(const QModelIndex& modelIndex);
        void Remove(ModelItemData* modelItemData);
        void RemoveAlias(const QModelIndex& modelIndex);
        void RemoveChildren(const QModelIndexList& modelIndexList);
        void RemoveChildren(const QModelIndex& modelIndex);
        void RecursiveSetAnimGraphInstance(ModelItemData* modelItemData, EMotionFX::AnimGraphInstance* currentAnimGraphInstance, EMotionFX::AnimGraphInstance* newAnimGraphInstance);
        void RecursiveSetReferenceNodeAnimGraphInstance(ModelItemData* modelItemData, EMotionFX::AnimGraphInstance* currentAnimGraphInstance, EMotionFX::AnimGraphInstance* newAnimGraphInstance);

        // QModelIndex can contain a pointer to ModelItemData, but we need the
        // inverse lookup. We use a multiset since we are dealing with a key
        // that can have duplicates in the case where there is no animGraphInstance
        using ModelItemDataSet = AZStd::multiset<ModelItemData*, ModelItemDataComparator>;
        ModelItemDataSet m_modelItemDataSet;

        // We keep track of the roots so we are not doing lots of lookups
        // querying the amount of elements at the first level (used mostly in rowCount)
        AZStd::vector<ModelItemData*> m_rootModelItemData;

        // We track the alias items (root state machines in reference nodes) so we can remove them.
        // The map is from reference node to root state machine.
        using AliasMap = AZStd::unordered_map<ModelItemData*, ModelItemData*>;
        AliasMap m_aliasMap;

        // Since reference nodes are integrated to the asset system, we need to track the asset ids to root anim graph
        // so we know which root items were added by the asset system.
        AZStd::unordered_map<AZ::Data::AssetId, QPersistentModelIndex> m_modelIndexByAssetId;

        // Indicates the current node/graph we are currently looking at. The UI shows this
        // node/a parent in bold in the navigation widget and other UIs
        QPersistentModelIndex m_focus;
        QPersistentModelIndex m_parentFocus;
        QPersistentModelIndex m_pendingFocus;

        // When we are deleting something, the pre-callback will cache the QModelIndex(es) of the thing being deleted.
        // Then the post-callback will use the cache to actually remove the elements from the model
        AZStd::vector<QPersistentModelIndex> m_pendingToDeleteIndices;

        // When we are editing something while something is being deleted, we can't call edit directly.
        // Putting it on this cached list, and then call edit after removePending get called.
        AZStd::vector<QPersistentModelIndex> m_pendingToEditIndices;

        QItemSelectionModel m_selectionModel;

        // Callbacks
#define ANIMGRAPHMODEL_CALLBACK(CLASSNAME)                                                                      \
    class CLASSNAME                                                                                             \
        : public MCore::Command::Callback                                                                       \
    {                                                                                                           \
    public:                                                                                                     \
        CLASSNAME(AnimGraphModel & animGraphModel, bool executePreUndo = false, bool executePreCommand = false) \
            : MCore::Command::Callback(executePreUndo, executePreCommand)                                       \
            , m_animGraphModel(animGraphModel)                                                                  \
        {}                                                                                                      \
        ~CLASSNAME() override {}                                                                                \
        bool Execute(MCore::Command * command, const MCore::CommandLine& commandLine) override;                 \
        bool Undo(MCore::Command* command, const MCore::CommandLine& commandLine) override;                     \
    private:                                                                                                    \
        AnimGraphModel& m_animGraphModel;                                                                       \
    };                                                                                                          \
    friend class CLASSNAME;

        ANIMGRAPHMODEL_CALLBACK(CommandDidLoadAnimGraphCallback);
        ANIMGRAPHMODEL_CALLBACK(CommandDidCreateAnimGraphCallback);
        ANIMGRAPHMODEL_CALLBACK(CommandWillRemoveAnimGraphCallback);
        ANIMGRAPHMODEL_CALLBACK(CommandDidRemoveAnimGraphCallback);
        ANIMGRAPHMODEL_CALLBACK(CommandDidActivateAnimGraphCallback);
        ANIMGRAPHMODEL_CALLBACK(CommandDidActivateAnimGraphPostUndoCallback);

        ANIMGRAPHMODEL_CALLBACK(CommandDidCreateNodeCallback);
        ANIMGRAPHMODEL_CALLBACK(CommandWillRemoveNodeCallback);
        ANIMGRAPHMODEL_CALLBACK(CommandDidRemoveNodeCallback);
        ANIMGRAPHMODEL_CALLBACK(CommandDidAdjustNodeCallback);

        ANIMGRAPHMODEL_CALLBACK(CommandDidCreateConnectionCallback);
        ANIMGRAPHMODEL_CALLBACK(CommandWillRemoveConnectionCallback);
        ANIMGRAPHMODEL_CALLBACK(CommandDidRemoveConnectionCallback);
        ANIMGRAPHMODEL_CALLBACK(CommandDidAdjustConnectionCallback);

        static bool CommandDidConditionChangeCallbackHelper(AnimGraphModel& animGraphModel, MCore::Command* command);
        ANIMGRAPHMODEL_CALLBACK(CommandDidAddRemoveConditionCallback);
        ANIMGRAPHMODEL_CALLBACK(CommandDidAdjustConditionCallback);

        ANIMGRAPHMODEL_CALLBACK(CommandDidEditActionCallback);

        ANIMGRAPHMODEL_CALLBACK(CommandDidSetEntryStateCallback);

        ANIMGRAPHMODEL_CALLBACK(CommandDidCreateParameterCallback);
        ANIMGRAPHMODEL_CALLBACK(CommandDidAdjustParameterCallback);
        ANIMGRAPHMODEL_CALLBACK(CommandDidRemoveParameterCallback);
        ANIMGRAPHMODEL_CALLBACK(CommandDidMoveParameterCallback);
        ANIMGRAPHMODEL_CALLBACK(CommandDidAddGroupParameterCallback);
        ANIMGRAPHMODEL_CALLBACK(CommandDidRemoveGroupParameterCallback);
        ANIMGRAPHMODEL_CALLBACK(CommandDidAdjustGroupParameterCallback);

        ANIMGRAPHMODEL_CALLBACK(CommandDidCreateMotionSetCallback);
        ANIMGRAPHMODEL_CALLBACK(CommandDidRemoveMotionSetCallback);
        ANIMGRAPHMODEL_CALLBACK(CommandDidAdjustMotionSetCallback);
        ANIMGRAPHMODEL_CALLBACK(CommandDidMotionSetAddMotionCallback);
        ANIMGRAPHMODEL_CALLBACK(CommandDidMotionSetRemoveMotionCallback);
        ANIMGRAPHMODEL_CALLBACK(CommandDidMotionSetAdjustMotionCallback);
        ANIMGRAPHMODEL_CALLBACK(CommandDidLoadMotionSetCallback);
        ANIMGRAPHMODEL_CALLBACK(CommandDidSaveMotionSetCallback);
        ANIMGRAPHMODEL_CALLBACK(CommandDidPlayMotionCallback);
        ANIMGRAPHMODEL_CALLBACK(CommandRemoveActorInstanceCallback);

        static bool OnParameterChangedCallback(AnimGraphModel& animGraphModel, MCore::Command* command, const MCore::CommandLine& commandLine);

#undef ANIMGRAPHMODEL_CALLBACK

        // AZ::Data::AssetBus::Router
        void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        void OnAssetUnloaded(const AZ::Data::AssetId assetId, const AZ::Data::AssetType assetType) override;

        // Helper functions so we reuse some code across command callbacks. These functions find the model items that get affected and
        // update them properly
        bool NodeAdded(EMotionFX::AnimGraphNode* animGraphNode);
        bool ConnectionAdded(EMotionFX::AnimGraphNode* animGraphTargetNode, EMotionFX::BlendTreeConnection* animGraphConnection);
        bool TransitionAdded(EMotionFX::AnimGraphStateTransition* animGraphTransition);
        void RemoveIndices(const QModelIndexList& modelIndexList);
        bool Edited(EMotionFX::AnimGraphObject* animGraphObject, const QVector<int>& roles = QVector<int>());
        bool ParameterEdited(EMotionFX::AnimGraph* animGraph);
        bool MotionEdited();

        void GetParentIfReferencedRootStateMachine(QModelIndex& modelIndex) const;

        AZStd::vector<MCore::Command::Callback*> m_commandCallbacks;
    };

    struct QModelIndexHash
    {
        std::size_t operator()(const QModelIndex& modelIndex) const
        {
            return qHash(modelIndex);
        }
    };

    struct QPersistentModelIndexHash
    {
        std::size_t operator()(const QPersistentModelIndex& modelIndex) const
        {
            return qHash(modelIndex);
        }
    };

    template<>
    struct AnimGraphModel::ItemTypeForClass<EMotionFX::AnimGraphNode>
    {
        static constexpr ModelItemType itemType = AnimGraphModel::ModelItemType::NODE;
    };
    template<>
    struct AnimGraphModel::ItemTypeForClass<EMotionFX::AnimGraphStateTransition>
    {
        static constexpr AnimGraphModel::ModelItemType itemType = AnimGraphModel::ModelItemType::TRANSITION;
    };
    template<>
    struct AnimGraphModel::ItemTypeForClass<EMotionFX::BlendTreeConnection>
    {
        static constexpr AnimGraphModel::ModelItemType itemType = AnimGraphModel::ModelItemType::CONNECTION;
    };

    template<>
    struct AnimGraphModel::RoleForClass<EMotionFX::AnimGraphNode>
    {
        static constexpr AnimGraphModel::Role role = AnimGraphModel::ROLE_NODE_POINTER;
    };
    template<>
    struct AnimGraphModel::RoleForClass<EMotionFX::AnimGraphStateTransition>
    {
        static constexpr AnimGraphModel::Role role = AnimGraphModel::ROLE_TRANSITION_POINTER;
    };
    template<>
    struct AnimGraphModel::RoleForClass<EMotionFX::BlendTreeConnection>
    {
        static constexpr AnimGraphModel::Role role = AnimGraphModel::ROLE_CONNECTION_POINTER;
    };
}   // namespace EMStudio

// Required to return different types through a QVariant and to make signal/slot connections
Q_DECLARE_METATYPE(EMStudio::AnimGraphModel::ModelItemType);
