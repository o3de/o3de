/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <AzCore/Module/Module.h>

namespace AZ::Meshlets::IntegrationTest
{
    class MeshletsIntegrationTestModule : public AZ::Module
    {
    public:
        AZ_RTTI(MeshletsIntegrationTestModule,
                "{8F5C2A7E-3D9B-4A1F-8C6D-2E7B5F9A1D40}", AZ::Module);
        MeshletsIntegrationTestModule() = default;
    };
}

AZ_DECLARE_MODULE_CLASS(Gem_Meshlets_IntegrationTest,
                        AZ::Meshlets::IntegrationTest::MeshletsIntegrationTestModule)
