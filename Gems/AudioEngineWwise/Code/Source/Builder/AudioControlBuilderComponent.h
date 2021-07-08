/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <Builder/AudioControlBuilderWorker.h>

namespace AudioControlBuilder
{
    class BuilderPluginComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(BuilderPluginComponent, "{4C0E0008-3D09-4628-8CEE-E9C6475AFB62}");

        BuilderPluginComponent() = default;
        ~BuilderPluginComponent() override = default;

        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("AudioControlBuilderService"));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("AudioControlBuilderService"));
        }

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////

    private:
        AudioControlBuilderWorker m_audioControlBuilder;
    };

} // namespace AudioControlBuilder
