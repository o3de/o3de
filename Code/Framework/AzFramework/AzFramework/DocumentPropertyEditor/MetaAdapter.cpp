/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/DocumentPropertyEditor/MetaAdapter.h>
#include <AzFramework/DocumentPropertyEditor/AdapterBuilder.h>

namespace AZ::DocumentPropertyEditor
{
    void MetaAdapter::SetSourceAdapter(DocumentAdapterPtr sourceAdapter)
    {
        m_sourceAdapter = sourceAdapter;
        m_resetHandler = DocumentAdapter::ResetEvent::Handler(
            [this]()
            {
                this->HandleReset();
            });
        m_sourceAdapter->ConnectResetHandler(m_resetHandler);

        m_changedHandler = DocumentAdapter::ChangedEvent::Handler(
            [this](const Dom::Patch& patch)
            {
                this->HandleDomChange(patch);
            });
        m_sourceAdapter->ConnectChangedHandler(m_changedHandler);

        m_domMessageHandler = AZ::DocumentPropertyEditor::DocumentAdapter::MessageEvent::Handler(
            [this](const AZ::DocumentPropertyEditor::AdapterMessage& message, AZ::Dom::Value& value)
            {
                this->HandleDomMessage(message, value);
            });
        m_sourceAdapter->ConnectMessageHandler(m_domMessageHandler);

        // populate the filter data from the source's full adapter contents, just like a reset
        HandleReset();
    }

    void MetaAdapter::HandleDomMessage(const AZ::DocumentPropertyEditor::AdapterMessage& message, [[maybe_unused]] Dom::Value& value)
    {
        // forward all messages unaltered
        DocumentAdapter::SendAdapterMessage(message);
    }

    bool MetaAdapter::IsRow(const Dom::Value& domValue)
    {
        return (domValue.IsNode() && domValue.GetNodeName() == Dpe::GetNodeName<Dpe::Nodes::Row>());
    }

    bool MetaAdapter::IsRow(const Dom::Path& sourcePath) const
    {
        const auto& sourceRoot = m_sourceAdapter->GetContents();
        auto sourceNode = sourceRoot[sourcePath];
        return IsRow(sourceNode);
    }


    Dom::Path MetaAdapter::GetRowPath(const Dom::Path& sourcePath) const
    {
        Dom::Path rowPath;
        const auto& contents = m_sourceAdapter->GetContents();
        const auto* currDomValue = &contents;
        for (const auto& pathEntry : sourcePath)
        {
            currDomValue = &(*currDomValue)[pathEntry];
            if (IsRow(*currDomValue))
            {
                rowPath.Push(pathEntry);
            }
        }
        return rowPath;
    }

} // namespace AZ::DocumentPropertyEditor
