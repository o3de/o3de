/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AtomToolsFramework/Application/AtomToolsApplication.h>

namespace AtomToolsFramework
{
    class AtomToolsDocumentApplication
        : public AtomToolsApplication
    {
    public:
        AZ_TYPE_INFO(AtomToolsDocumentApplication, "{F4B43677-EB95-4CBB-8B8E-9EF4247E6F0D}");

        using Base = AtomToolsApplication;

        AtomToolsDocumentApplication(int* argc, char*** argv);

        // AtomToolsApplication overrides...
        void ProcessCommandLine(const AZ::CommandLine& commandLine) override;
    };
} // namespace AtomToolsFramework
