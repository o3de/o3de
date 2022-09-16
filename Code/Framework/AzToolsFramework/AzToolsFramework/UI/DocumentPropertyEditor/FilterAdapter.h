/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/DocumentPropertyEditor/DocumentAdapter.h>
#include <QString>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/set.h>

namespace AZ::DocumentPropertyEditor
{
    class RowFilterAdapter : public DocumentAdapter
    {
    public:
        RowFilterAdapter();
        ~RowFilterAdapter();

        void SetSourceAdapter(DocumentAdapterPtr sourceAdapter);

    protected:
        struct MatchInfoNode
        {
            ~MatchInfoNode()
            {
                for (auto child : m_childMatchState)
                {
                    delete child;
                }
                m_childMatchState.clear();
            }

            bool m_matchesSelf = false;
            AZStd::set<MatchInfoNode*> m_matchingDescendents;
            MatchInfoNode* m_parentNode = nullptr;

            //! Deque where only row node children are populate, other children are null entries
            AZStd::deque<MatchInfoNode*> m_childMatchState;

        protected:
            MatchInfoNode(MatchInfoNode* parentNode)
                : m_parentNode(parentNode)
            {
            }
        };

        // pure virtual methods for new RowFilterAdapters
        virtual MatchInfoNode* NewMatchInfoNode(MatchInfoNode* parentRow) const = 0;
        virtual void CacheDomInfoForNode(const Dom::Value& domValue, MatchInfoNode* matchNode) const = 0;
        virtual bool MatchesFilter(MatchInfoNode* matchNode) const = 0;

        static bool IsRow(const Dom::Value& domValue);
        bool IsRow(const Dom::Path& sourcePath) const;

        void SetFilterActive(bool activateFilter);
        void InvalidateFilter();
        
        Dom::Value GenerateContents() override;

        DocumentAdapterPtr m_sourceAdapter;

        void HandleReset();
        void HandleDomChange(const Dom::Patch& patch);
        void HandleDomMessage(const AZ::DocumentPropertyEditor::AdapterMessage& message, Dom::Value& value);

        DocumentAdapter::ResetEvent::Handler m_resetHandler;
        ChangedEvent::Handler m_changedHandler;
        MessageEvent::Handler m_domMessageHandler;

        MatchInfoNode* GetMatchNodeAtPath(const Dom::Path& sourcePath);

        /*! populates the MatchInfoNode nodes for the given path and any descendent row children created.
         *  All new nodes have their m_matchesSelf, m_matchingDescendents, and  m_matchableDomTerms set */
        void PopulateNodesAtPath(const Dom::Path& sourcePath, bool replaceExisting);

        /*! updates the match states (m_matchesSelf, m_matchingDescendents) for the given row node,
            and updates the m_matchingDescendents state for all its ancestors
            \param rowState the row to operate on */
        void UpdateMatchState(MatchInfoNode* rowState);

        //! returns the first path in the ancestry of sourcePath that is of type Row, including self
        Dom::Path GetRowPath(const Dom::Path& sourcePath) const;

        //! indicates whether all children of a direct match are considered matching as well
        bool m_includeAllMatchDescendents = true;
        bool m_filterActive = false;

        MatchInfoNode* m_root = nullptr;
    };

    class ValueStringFilter : public RowFilterAdapter
    {
    public:
        ValueStringFilter();
        void SetFilterString(QString filterString);

        struct StringMatchNode : public RowFilterAdapter::MatchInfoNode
        {
            StringMatchNode(MatchInfoNode* parentRow)
                : MatchInfoNode(parentRow)
            {
            }

            void AddStringifyValue(const Dom::Value& domValue);

            QString m_matchableDomTerms;
        };

    protected:
        // pure virtual overrides
        virtual MatchInfoNode* NewMatchInfoNode(MatchInfoNode* parentRow) const  override;
        virtual void CacheDomInfoForNode(const Dom::Value& domValue, MatchInfoNode* matchNode) const override;
        virtual bool MatchesFilter(MatchInfoNode* matchNode) const override;

        bool m_includeDescriptions = true;
        QString m_filterString;
    };
} // namespace AZ::DocumentPropertyEditor
