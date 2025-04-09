/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>

namespace AZ::Render
{
    class StarsAssetHandler;

    class StarsSystemComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(StarsSystemComponent, "{ce10f0f9-5fe3-4376-8ccf-d56ec780005d}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        StarsSystemComponent() = default;
        ~StarsSystemComponent() = default;

    protected:
        //! AZ::Component
        void Activate() override;
        void Deactivate() override;

    private:
        AZ::Render::StarsAssetHandler* m_starsAssetHandler;
    };

} // namespace AZ::Render 
