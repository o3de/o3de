/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Public/RPISystem.h>
#include <AzCore/Component/Component.h>

namespace UnitTest
{
    class MockTerrainRPISystemComponent : public AZ::Component
    {
    public:
        AZ_COMPONENT(MockTerrainRPISystemComponent, "{1e42c9a8-a264-4b4f-aaa5-cc66558cce7f}");

        static void Reflect([[maybe_unused]] AZ::ReflectContext* context)
        {
        }

        void Activate() override
        {
            AZ::RPI::RPISystemDescriptor rpiSystemDescriptor;
            m_rpiSystem = AZStd::make_unique<AZ::RPI::RPISystem>();
            m_rpiSystem->Initialize(rpiSystemDescriptor);

            AZ::RPI::ImageSystemDescriptor imageSystemDescriptor;
            m_imageSystem = AZStd::make_unique<AZ::RPI::ImageSystem>();
            m_imageSystem->Init(imageSystemDescriptor);
        }

        void Deactivate() override
        {
            m_imageSystem->Shutdown();
            m_imageSystem = nullptr;
            m_rpiSystem->Shutdown();
            m_rpiSystem = nullptr;
        }

    private:
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
        {
            services.push_back(AZ_CRC_CE("RPISystem"));
        }

        AZStd::unique_ptr<AZ::RPI::RPISystem> m_rpiSystem;
        AZStd::unique_ptr<AZ::RPI::ImageSystem> m_imageSystem;
    };

} // namespace UnitTest
