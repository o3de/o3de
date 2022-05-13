/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <NavigationMeshAsset.h>
#include <AzCore/Component/Component.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace RecastNavigation
{
    class RecastNavigationSystemComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(RecastNavigationSystemComponent, "{CD9BD47E-C984-4E89-AD88-450F055AA1CA}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        RecastNavigationSystemComponent() = default;
        ~RecastNavigationSystemComponent() = default;

    protected:
        //! AZ::Component
        void Activate() override;
        void Deactivate() override;

    private:
        AZStd::unique_ptr<NavigationMeshAssetHandler> m_navigationMeshAssetHandler;
    };

} // namespace AZ::Render
