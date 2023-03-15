// {BEGIN_LICENSE}
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
 // {END_LICENSE}

#pragma once

#include <AzCore/Component/Component.h>

#include <${Name}/${Name}Bus.h>

namespace ${SanitizedCppName}
{
    class ${SanitizedCppName}SystemComponent
        : public AZ::Component
        , protected ${SanitizedCppName}RequestBus::Handler
    {
    public:
        AZ_TYPE_INFO_WITH_NAME_DECL(${SanitizedCppName}SystemComponent)
        AZ_RTTI_NO_TYPE_INFO_DECL();
        AZ_COMPONENT_INTRUSIVE_DESCRIPTOR_TYPE(${SanitizedCppName}SystemComponent);
        AZ_COMPONENT_BASE_DECL();
        AZ_CLASS_ALLOCATOR_DECL

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        ${SanitizedCppName}SystemComponent();
        ~${SanitizedCppName}SystemComponent();

    protected:
        ////////////////////////////////////////////////////////////////////////
        // ${SanitizedCppName}RequestBus interface implementation

        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////
    };
}
