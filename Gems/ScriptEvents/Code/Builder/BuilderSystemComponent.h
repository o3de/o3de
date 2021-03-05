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

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace ScriptEventsBuilder
{
    class BuilderSystemComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(BuilderSystemComponent, "{6CE4EF5D-5A18-4E25-A676-501644676B58}");

        BuilderSystemComponent();
        ~BuilderSystemComponent() override;

        static void Reflect(AZ::ReflectContext* context);

        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

    private:
        BuilderSystemComponent(const BuilderSystemComponent&) = delete;

    };
}