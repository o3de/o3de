/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/unordered_set.h>
#include <AzFramework/DocumentPropertyEditor/MetaAdapter.h>
#include <AzFramework/AzFrameworkAPI.h>

namespace AZ::DocumentPropertyEditor
{
    class AZF_API RowFilterAdapter : public MetaAdapter
    {
    public:
        RowFilterAdapter();
        ~RowFilterAdapter();

        void SetIncludeAllMatchDescendants(bool includeAll);
        bool FilterIsActive() const;

        // MetaAdapter overrides
        Dom::Path MapFromSourcePath(const Dom::Path& sourcePath) const override;
        Dom::Path MapToSourcePath(const Dom::Path& filterPath) const override;

    protected:
        // If you make a subclass of RowFilterAdapter, you must implement these methods to provide the filtering logic for your adapter.
        // You can use the MatchInfoNode to store any additional information you need to determine whether a node matches the filter.
        // Make sure to override NewMatchInfoNode and return your own class, and then CacheDomInfoForNode to populate your node
        // with any information you'd like to cache, then
        // MatchesFilter(MatchInfoNode* matchNode) const override to return true if the node matches or not.
        struct AZF_API MatchInfoNode
        {
            virtual ~MatchInfoNode();

            bool IncludeInFilter();
            void RemoveChildAtIndex(size_t index);

            bool m_matchesSelf = false;

            //! this will only be set and checked if the FilterAdapter has m_includeAllMatchDescendants set
            bool m_hasMatchingAncestor = false;

            AZStd::unordered_set<MatchInfoNode*> m_matchingDescendants;

            //! Deque where only row node children are populated, other children are null entries
            AZStd::deque<MatchInfoNode*> m_childMatchState;
            MatchInfoNode* m_parentNode = nullptr;

            unsigned int m_lastFilterUpdateFrame = 0; //!< frame that this node's match state last changed

        protected:
            MatchInfoNode();
        };

        // pure virtual methods for new RowFilterAdapters
        virtual MatchInfoNode* NewMatchInfoNode() const = 0;
        virtual void CacheDomInfoForNode(const Dom::Value& domValue, MatchInfoNode* matchNode) const = 0;
        virtual bool MatchesFilter(MatchInfoNode* matchNode) const = 0;

        MatchInfoNode* MakeNewNode(MatchInfoNode* parentNode, unsigned int creationFrame);

        void SetFilterActive(bool activateFilter);
        void InvalidateFilter();

        // DocumentAdapter overrides ...
        Dom::Value GenerateContents() override;

        // MetaAdapter overrides ...
        void HandleReset() override;
        void HandleDomChange(const Dom::Patch& patch) override;

        MatchInfoNode* GetMatchNodeAtPath(const Dom::Path& sourcePath);
        Dom::Path GetSourcePathForNode(const MatchInfoNode* matchNode);

        /*! populates the MatchInfoNode nodes for the given path and any descendant row children created.
         *  All new nodes have their m_matchesSelf, m_matchingDescendants, and  m_matchableDomTerms set */
        void PopulateNodesAtPath(const Dom::Path& sourcePath, bool replaceExisting);
        void GenerateFullTree();

        static void CullUnmatchedChildRows(Dom::Value& rowValue, const MatchInfoNode* rowMatchNode);

        enum PatchType
        {
            Incremental, // patch any changes since last operation
            RemovalsFromSource, // patch all removals to get from the source adapter's state to the current filter state
            PatchToSource // patch all additions required to get from the current filter state to the source adapter's state
        };
        void GeneratePatch(PatchType patchType, AZ::Dom::Patch& patchToFill);

        /*! updates the match states (m_matchesSelf, m_matchingDescendants) for the given row node,
            and updates the m_matchingDescendants state for all its ancestors
            \param rowState the row to operate on */
        void UpdateMatchState(MatchInfoNode* rowState);
        void UpdateMatchDescendants(MatchInfoNode* startNode); //!< calls UpdateMatchState on the subtree starting at startNode

        /*! this increases every time the source Adapter changes or resets to keep track of which
         *  node updates are new */
        unsigned int m_updateFrame = 0;

        bool m_filterActive = false;

        MatchInfoNode* m_root = nullptr;

    private:
        //! indicates whether all children of a direct match are considered matching as well
        bool m_includeAllMatchDescendants = true;
    };
} // namespace AZ::DocumentPropertyEditor
