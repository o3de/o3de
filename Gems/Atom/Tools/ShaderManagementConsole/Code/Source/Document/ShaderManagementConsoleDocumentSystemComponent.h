/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>

namespace ShaderManagementConsole
{
    //! ShaderManagementConsoleDocumentSystemComponent
    class ShaderManagementConsoleDocumentSystemComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(ShaderManagementConsoleDocumentSystemComponent, "{1610159D-59DC-48B1-B2D1-FCE7AFD3B012}");

        ShaderManagementConsoleDocumentSystemComponent() = default;
        ~ShaderManagementConsoleDocumentSystemComponent() = default;
        ShaderManagementConsoleDocumentSystemComponent(const ShaderManagementConsoleDocumentSystemComponent&) = delete;
        ShaderManagementConsoleDocumentSystemComponent& operator =(const ShaderManagementConsoleDocumentSystemComponent&) = delete;

        static void Reflect(AZ::ReflectContext* context);

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
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
