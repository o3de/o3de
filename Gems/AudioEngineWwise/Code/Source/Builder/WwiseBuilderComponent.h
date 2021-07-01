/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <Builder/WwiseBuilderWorker.h>

namespace WwiseBuilder
{
    class BuilderPluginComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(BuilderPluginComponent, "{8630414A-0BA6-4759-809A-C6903994AE30}");

        BuilderPluginComponent() = default;
        ~BuilderPluginComponent() override = default;

        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("WwiseBuilderService"));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("WwiseBuilderService"));
        }

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////

    private:
        WwiseBuilderWorker m_wwiseBuilder;
    };

} // namespace WwiseBuilder
