/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/map.h>
#include <AzCore/std/containers/vector.h>
#include <AzFramework/DocumentPropertyEditor/AdapterBuilder.h>
#include <AzFramework/DocumentPropertyEditor/DocumentAdapter.h>

namespace AZ::DocumentPropertyEditor
{
    //! class to allow multi-edits of row-based DPE adapters
    class RowAggregateAdapter : public DocumentAdapter
    {
    public:
        RowAggregateAdapter();
        virtual ~RowAggregateAdapter();

        void AddAdapter(DocumentAdapterPtr sourceAdapter);
        void RemoveAdapter(DocumentAdapterPtr sourceAdapter);
        void ClearAdapters();

        /*! determines whether to generate "values differ" rows at all
         *   if not, just generates rows based on the first adapter */
        void SetGenerateDiffRows(bool generateDiffRows)
        {
            m_generateDiffRows = generateDiffRows;
        }

        ExpanderSettings* CreateExpanderSettings(
            DocumentAdapter* referenceAdapter,
            const AZStd::string& settingsRegistryKey = AZStd::string(),
            const AZStd::string& propertyEditorName = AZStd::string()) override;

    protected:
        struct AggregateNode
        {
            bool HasEntryForAdapter(size_t adapterIndex);
            Dom::Path GetPathForAdapter(size_t adapterIndex);
            void AddEntry(size_t adapterIndex, size_t pathEntryIndex, bool matchesOtherEntries);

            //! get the index of the adapter that this node will use for comparisons and creating the aggregate row to display
            size_t GetComparisonAdapterIndex(size_t omitAdapterIndex = AggregateNode::InvalidEntry);

            /*! removes entry for adapter at adapterIndex for this node, and destroys the node if
             * it was subsequently empty. Returns true if destroyed */
            bool RemoveEntry(size_t adapterIndex);

            size_t EntryCount() const;
            void ShiftChildIndices(size_t adapterIndex, size_t startIndex, int delta);

            static constexpr size_t InvalidEntry = size_t(-1);
            AZStd::vector<size_t> m_pathEntries; // per-adapter DOM index represented by AggregateNode

            bool m_allEntriesMatch = true;

            // per-adapter mapping of DOM index to child
            AZStd::vector<AZStd::map<size_t, AggregateNode*>> m_pathIndexToChildMaps;

            AggregateNode* m_parent = nullptr;

            //! all children (even incomplete ones), ordered by primary adapter
            AZStd::vector<AZStd::unique_ptr<AggregateNode>> m_childRows;
        };

        //! implemented by child adapters to return a vector of message Names to forward to each sub-adapter
        virtual AZStd::vector<AZ::Name> GetMessagesToForward() = 0;

        //! virtual function to generate an aggregate row that represents all the matching Dom::Values with in this node
        virtual Dom::Value GenerateAggregateRow(AggregateNode* matchingNode) = 0;

        /*! pure virtual to generate a "values differ" row that is appropriate for this type of AggregateAdapter
            mismatchNode is provided so the row presented can include information from individual mismatched Dom::Values, if desired */
        virtual Dom::Value GenerateValuesDifferRow(AggregateNode* mismatchNode) = 0;

        /*! pure virtual to determine if the row value from one adapter should be
            considered the same aggregate row as a value from another adapter */
        virtual bool SameRow(const Dom::Value& newRow, const Dom::Value& existingRow) = 0;

        //! pure virtual to determine if two row values match such that they can be edited by one PropertyHandler
        virtual bool ValuesMatch(const Dom::Value& left, const Dom::Value& right) = 0;

        // message handlers for all owned adapters
        void HandleAdapterReset(DocumentAdapterPtr adapter);
        void HandleDomChange(DocumentAdapterPtr adapter, const Dom::Patch& patch);
        void HandleDomMessage(DocumentAdapterPtr adapter, const AZ::DocumentPropertyEditor::AdapterMessage& message, Dom::Value& value);

        // DocumentAdapter overrides
        Dom::Value GenerateContents() override;
        Dom::Value HandleMessage(const AdapterMessage& message) override;

        //! indicates whether this node has entries for all adapters and is therefore shown in the DOM
        bool NodeIsComplete(const AggregateNode* node) const
        {
            return (node->EntryCount() == m_adapters.size());
        }

        void EvaluateAllEntriesMatch(AggregateNode* node);

        size_t GetIndexForAdapter(const DocumentAdapterPtr& adapter);

        AggregateNode* GetNodeAtAdapterPath(size_t adapterIndex, const Dom::Path& path, Dom::Path& leftoverPath);

        //! get a Dom Value representing the given node by asking the first valid adapter that isn't at omitAdapterIndex
        Dom::Value GetComparisonRow(AggregateNode* aggregateNode, size_t omitAdapterIndex = AggregateNode::InvalidEntry);

        //! generates a Dom::Value for this node (not including row descendents) for use in GenerateContents or patch operations
        Dom::Value GetValueForNode(AggregateNode* aggregateNode);

        //! generates a Dom::Value for this node and its descendents for use in GenerateContents or patch operations
        Dom::Value GetValueHierarchyForNode(AggregateNode* aggregateNode);

        //! gets the node at the given path relative to this adapter, if it exists
        AggregateNode* GetNodeAtPath(const Dom::Path& aggregatePath);
        Dom::Path GetPathForNode(AggregateNode* node); //!< returns the resultant path for this node if it exists, otherwise an empty path

        void PopulateNodesForAdapter(size_t adapterIndex);

        //! adds a child row given the adapter index, parentNode, new value, and new index.
        //! \param outgoingPatch If generating a patch operation for this add is desirable, specify a non-null outgoingPatch
        AggregateNode* AddChildRow(
            size_t adapterIndex, AggregateNode* parentNode, const Dom::Value& childValue, size_t childIndex, Dom::Patch* outgoingPatch);

        void PopulateChildren(size_t adapterIndex, const Dom::Value& parentValue, AggregateNode* parentNode);

        //! removes the entry for adapterIndex in this node
        //! \param outgoingPatch when outgoingPatch is not null, this will add a remove patch operation to it, if appropriate
        void ProcessRemoval(AggregateNode* rowNode, size_t adapterIndex, Dom::Patch* outgoingPatch);

        void UpdateAndPatchNode(
            AggregateNode* rowNode,
            size_t adapterIndex,
            const Dom::PatchOperation& patchOperation,
            const Dom::Path& pathPastNode,
            Dom::Patch& outgoingPatch);

        static void RemoveChildRows(Dom::Value& rowValue);

        struct AdapterInfo
        {
            DocumentAdapter::ResetEvent::Handler resetHandler;
            ChangedEvent::Handler changedHandler;
            MessageEvent::Handler domMessageHandler;
            DocumentAdapterPtr adapter;
        };

        //! all the adapters represented in this aggregate (multi-edit)
        AZStd::vector<AZStd::unique_ptr<AdapterInfo>> m_adapters;

        // potential rows always in the row order of the first adapter in m_adapters
        AZStd::unique_ptr<AggregateNode> m_rootNode;

        //! monotonically increasing frame counter that increments whenever a source adapter gets an update
        unsigned int m_updateFrame = 0;
        bool m_generateDiffRows = true;
        AdapterBuilder m_builder;
    };

    class LabeledRowAggregateAdapter : public RowAggregateAdapter
    {
    protected:
        static AZStd::string_view GetFirstLabel(const Dom::Value& rowValue);

        AZStd::vector<AZ::Name> GetMessagesToForward();
        Dom::Value GenerateAggregateRow(AggregateNode* matchingNode) override;
        Dom::Value GenerateValuesDifferRow(AggregateNode* mismatchNode) override;
        bool SameRow(const Dom::Value& newRow, const Dom::Value& existingRow) override;
        bool ValuesMatch(const Dom::Value& left, const Dom::Value& right) override;
    };

} // namespace AZ::DocumentPropertyEditor
