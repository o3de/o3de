/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>

namespace AtomToolsFramework
{
    //! AtomToolsMainWindowSystemComponent is used for initialization and registration of other classes.
    class AtomToolsMainWindowSystemComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(AtomToolsMainWindowSystemComponent, "{6E42380B-4ECD-47CF-B904-E16AB4E87D0D}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

    private:

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////
    };
}
