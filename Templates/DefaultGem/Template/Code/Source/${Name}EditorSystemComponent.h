// {BEGIN_LICENSE}
/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 */
 // {END_LICENSE}

#pragma once

#include <AzCore/Component/Component.h>

#include <AzToolsFramework/Entity/EditorEntityContextBus.h>

namespace ${Name}
{
    /// System component for ${Name} editor
    class ${Name}EditorSystemComponent
        : public AZ::Component
        , private AzToolsFramework::EditorEvents::Bus::Handler
    {
    public:
        AZ_COMPONENT(${Name}EditorSystemComponent, "${EditorSysCompClassId}");
        static void Reflect(AZ::ReflectContext* context);

        ${Name}EditorSystemComponent() = default;

    private:
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("${Name}EditorService"));
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("${Name}Service"));
        }

        // AZ::Component
        void Activate() override;
        void Deactivate() override;
    };
} // namespace ${Name}
