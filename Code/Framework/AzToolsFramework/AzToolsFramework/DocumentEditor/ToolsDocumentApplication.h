/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/DocumentEditor/Application/DocumentToolsApplication.h>
#include <AzToolsFramework/DocumentEditor/ToolsDocumentSystem.h>

namespace AzToolsFramework
{
    //! ToolsDocumentApplication is a base application class acting as a bridge between the base application class and the document
    //! system. It instantiates a document system for the provided tool ID and registers asset browser interactions for creating and opening
    //! supported document types.
    class ToolsDocumentApplication
        : public DocumentToolsApplication
    {
    public:
        AZ_CLASS_ALLOCATOR(ToolsDocumentApplication, AZ::SystemAllocator)
        AZ_TYPE_INFO(ToolsDocumentApplication, "{96A82321-0F2C-46A8-BFA9-126F9753C8C3}");
        AZ_DISABLE_COPY_MOVE(ToolsDocumentApplication);

        using Base = DocumentToolsApplication;

        ToolsDocumentApplication(const char* targetName, int* argc, char*** argv);

    protected:
        // ToolsApplication overrides...
        void StartCommon(AZ::Entity* systemEntity) override;
        void Destroy() override;
        void ProcessCommandLine(const AZ::CommandLine& commandLine) override;

        AZStd::unique_ptr<ToolsDocumentSystem> m_documentSystem; 
    };
} // namespace AzToolsFramework
