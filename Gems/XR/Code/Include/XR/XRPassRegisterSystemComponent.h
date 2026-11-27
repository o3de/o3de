/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>

namespace XR
{
    //! This module is in charge of adding the pass classes that are needed for
    //! managing the shading rate image used on foveated rendering.
    //! This is separated from the XRSystemComponent because it needs to be initialized
    //! after the RPI PassSystem.
    class PassRegisterSystemComponent final
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(PassRegisterSystemComponent, "{96CCDB81-906A-43EE-A3B5-F955F324D6BF}");

        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void Reflect(AZ::ReflectContext* context);

        PassRegisterSystemComponent() = default;
        ~PassRegisterSystemComponent() override = default;

        //////////////////////////////////////////////////////////////////////////
        // Component
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////

    private:
        // Load the pass template mappings
        void LoadPassTemplateMappings() const;

        AZ::RPI::PassSystemInterface::OnReadyLoadTemplatesEvent::Handler m_loadTemplatesHandler;
    };
}