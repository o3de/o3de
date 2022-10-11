/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>

namespace Physics
{
    //! This system will take care of reflecting physics material classes and
    //! registering the physics material asset.
    class MaterialSystemComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(Physics::MaterialSystemComponent, "{81CDF024-EC13-4472-9FBB-76235897889C}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        MaterialSystemComponent() = default;
        MaterialSystemComponent(const MaterialSystemComponent&) = delete;
        MaterialSystemComponent& operator=(const MaterialSystemComponent&) = delete;

        // AZ::Component overrides...
        void Activate() override;
        void Deactivate() override;

    protected:
        // Assets related data
        AZStd::vector<AZStd::unique_ptr<AZ::Data::AssetHandler>> m_assetHandlers;
    };
} // namespace Physics
