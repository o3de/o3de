/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/TickBus.h>
#include <MotionMatchingSystemComponent.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <MotionMatchingData.h>

namespace EMotionFX::MotionMatching
{
    /// System component for MotionMatching editor
    class MotionMatchingEditorSystemComponent
        : public MotionMatchingSystemComponent
        , protected MotionMatchingEditorRequestBus::Handler
        , private AzToolsFramework::EditorEvents::Bus::Handler
    {
        using BaseSystemComponent = MotionMatchingSystemComponent;
    public:
        AZ_COMPONENT(MotionMatchingEditorSystemComponent, "{a43957d3-5a2d-4c29-873d-7daacc357722}", BaseSystemComponent);
        static void Reflect(AZ::ReflectContext* context);

        MotionMatchingEditorSystemComponent();
        ~MotionMatchingEditorSystemComponent();

        // AZTickBus overrides ...
        int GetTickOrder() override
        {
            return AZ::TICK_PRE_RENDER;
        }
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

        // MotionMatchingRequestBus overrides ...
        void SetDebugDrawFeatureSchema(FeatureSchema* featureSchema) override;
        FeatureSchema* GetDebugDrawFeatureSchema() const override;

    private:
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        // AZ::Component
        void Activate() override;
        void Deactivate() override;

        // Debug drawing
        void DebugDraw(AZ::s32 debugDisplayId) override;
        void DebugDrawFrameFeatures(AzFramework::DebugDisplayRequests* debugDisplay);
        MotionInstance* m_lastMotionInstance = nullptr;
        AZStd::unique_ptr<MotionMatchingData> m_data;
        FeatureSchema* m_debugVisFeatureSchema = nullptr;
    };
} // namespace EMotionFX::MotionMatching
