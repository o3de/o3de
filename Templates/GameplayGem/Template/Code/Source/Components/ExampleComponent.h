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
#include <Components/${SanitizedCppName}ComponentController.h>

namespace ${SanitizedCppName}
{
    class ${SanitizedCppName}Component
        : public AZ::Component
    {
    public:
        AZ_COMPONENT_DECL(${SanitizedCppName}Component);

        ${SanitizedCppName}Component() = default;
        explicit ${SanitizedCppName}Component(const ${SanitizedCppName}ComponentConfig& config);

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

    protected:
        void Activate() override;
        void Deactivate() override;

    private:
        ${SanitizedCppName}ComponentController m_controller;
    };
} // namespace ${SanitizedCppName}
