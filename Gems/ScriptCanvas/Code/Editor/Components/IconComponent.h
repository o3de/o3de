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

#include <AzCore/Math/Uuid.h>
#include <AzCore/Component/Component.h>
#include <Editor/Include/ScriptCanvas/Bus/IconBus.h>

namespace ScriptCanvasEditor
{
    class IconComponent
        : public AZ::Component
        , public IconBus::Handler
    {
    public:
        AZ_COMPONENT(IconComponent, "{242FEF0E-1E3D-4F49-877F-F83E6B70F138}");
        static AZStd::string LookupClassIcon(const AZ::Uuid& classId);

        IconComponent() = default;
        IconComponent(const AZ::Uuid& classId);
        ~IconComponent() = default;

        static void Reflect(AZ::ReflectContext* serialize);

        // AZ::Component
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        void Init() override;
        void Activate() override;
        void Deactivate() override;

        // IconBus
        AZStd::string GetIconPath() const override;

    private:
        AZStd::string m_iconPath;
    };
}