/*
* All or portions of this file Copyright(c) Amazon.com, Inc.or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution(the "License").All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file.Do not
* remove or modify any license notices.This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <OtherActiveImGuiBus.h>

namespace AZ
{
    namespace LYIntegration
    {
        class ImGuiFeatureProcessor;

        //! When Atom is enabled, ImGuiManager from LY's ImGui gem will hand over drawing of ImGui via @OtherActiveImGuiRequestBus to this system component.
        //! Note: The LY ImGui gem only supports a single ImGui context, so Atom must have a single ImGui pass marked as the default.
        class ImguiAtomSystemComponent final
            : public AZ::Component
            , public ImGui::OtherActiveImGuiRequestBus::Handler
        {
        public:
            AZ_COMPONENT(ImguiAtomSystemComponent, "{D423E075-D971-435A-A9C1-57C3B0623A9B}");

            static void Reflect(AZ::ReflectContext* context);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatbile);

            ~ImguiAtomSystemComponent() override = default;

            // AZ::Component
            void Activate() override;
            void Deactivate() override;

            // OtherActiveImGuiRequestBus overrides ...
            void RenderImGuiBuffers(const ImDrawData& drawData) override;

        };
    } // namespace LYIntegration
} // namespace AZ
