/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Asset/AssetCommon.h>
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>

namespace AZ::Render
{
    class ${Name}SystemComponent final
        : public Component
    {
    public:
        AZ_COMPONENT(${Name}SystemComponent, "{${Random_Uuid}}");

        ${Name}SystemComponent() = default;
        ~${Name}SystemComponent() = default;

        static void Reflect(ReflectContext* context);

        static void GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(ComponentDescriptor::DependencyArrayType& required);

    private:

        ////////////////////////////////////////////////////////////////////////
        // Component interface implementation
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

    };
}

