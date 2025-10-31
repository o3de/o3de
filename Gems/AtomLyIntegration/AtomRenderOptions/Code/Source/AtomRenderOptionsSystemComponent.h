/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Memory/Memory.h>

#include "AtomRenderOptionsActionHandler.h"

namespace AZ::Render
{
    class AtomRenderOptionsSystemComponent : public AZ::Component
    {
    public:
        AZ_COMPONENT(AtomRenderOptionsSystemComponent, "{46FDACDF-8A4F-4CCE-85E3-2178398E0BDD}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

    protected:
        // AZ::Component overrides...
        void Activate() override;
        void Deactivate() override;

    private:
        AZStd::unique_ptr<AtomRenderOptionsActionHandler> m_actionHandler;
    };
} // namespace AZ::Render
