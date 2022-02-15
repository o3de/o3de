/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AtomToolsFramework/Application/AtomToolsApplication.h>
#include <AtomToolsFramework/Document/AtomToolsDocumentSystem.h>

namespace AtomToolsFramework
{
    class AtomToolsDocumentApplication
        : public AtomToolsApplication
    {
    public:
        AZ_TYPE_INFO(AtomToolsDocumentApplication, "{AC892170-D353-404A-A3D8-BB039C717295}");
        AZ_DISABLE_COPY_MOVE(AtomToolsDocumentApplication);

        using Base = AtomToolsApplication;

        AtomToolsDocumentApplication(const char* targetName, int* argc, char*** argv);

    protected:
        // AtomToolsApplication overrides...
        void Reflect(AZ::ReflectContext* context) override;
        void StartCommon(AZ::Entity* systemEntity) override;
        void Destroy() override;
        void ProcessCommandLine(const AZ::CommandLine& commandLine) override;

        AZStd::unique_ptr<AtomToolsDocumentSystem> m_documentSystem; 
    };
} // namespace AtomToolsFramework
