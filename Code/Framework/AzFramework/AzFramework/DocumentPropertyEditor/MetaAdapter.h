/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/DocumentPropertyEditor/DocumentAdapter.h>

namespace AZ::DocumentPropertyEditor
{

    class MetaAdapter : public DocumentAdapter
    {
    public:
        void SetSourceAdapter(DocumentAdapterPtr sourceAdapter);

    protected:
        // handlers for source adapter's messages
        void HandleDomMessage(const AZ::DocumentPropertyEditor::AdapterMessage& message, Dom::Value& value);
        virtual void HandleReset() = 0;
        virtual void HandleDomChange(const Dom::Patch& patch) = 0;

        static bool IsRow(const Dom::Value& domValue);
        bool IsRow(const Dom::Path& sourcePath) const;
        
        //! returns the first path in the ancestry of sourcePath that is of type Row, including self
        Dom::Path GetRowPath(const Dom::Path& sourcePath) const;

        DocumentAdapter::ResetEvent::Handler m_resetHandler;
        ChangedEvent::Handler m_changedHandler;
        MessageEvent::Handler m_domMessageHandler;

        DocumentAdapterPtr m_sourceAdapter;
    };

} // AZ::DocumentPropertyEditor
