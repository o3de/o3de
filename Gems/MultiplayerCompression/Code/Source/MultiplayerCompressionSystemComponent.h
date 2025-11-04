/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/std/containers/unordered_set.h>

#include <MultiplayerCompressionFactory.h>

namespace MultiplayerCompression
{
    /**
    * System component whose sole purpose is to own a compression factory and expose it via EBUS
    * so Multiplayer Gem can easily ingest the compressor.
    */
    class MultiplayerCompressionSystemComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(MultiplayerCompressionSystemComponent, "{C3099AC9-47A6-41D2-8928-F38F904BAC1B}");

        MultiplayerCompressionSystemComponent() = default;
        ~MultiplayerCompressionSystemComponent() override = default;

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override {}
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////
    private:
        AZStd::string_view m_multiplayerCompressionFactoryName;
    };
}
