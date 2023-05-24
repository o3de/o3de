/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/ComponentMode/EditorBaseComponentMode.h>
#include <AzToolsFramework/Manipulators/ScaleManipulators.h>

namespace AzToolsFramework
{
    namespace Components
    {
        class NonUniformScaleComponentMode
            : public AzToolsFramework::ComponentModeFramework::EditorBaseComponentMode
        {
        public:
            AZ_CLASS_ALLOCATOR(NonUniformScaleComponentMode, AZ::SystemAllocator)
            AZ_RTTI(NonUniformScaleComponentMode, "{E7D16788-DE62-4D13-8899-28A7C5DCA039}", EditorBaseComponentMode)

            static void Reflect(AZ::ReflectContext* context);

            NonUniformScaleComponentMode(const AZ::EntityComponentIdPair& entityComponentIdPair, AZ::Uuid componentType);
            NonUniformScaleComponentMode(const NonUniformScaleComponentMode&) = delete;
            NonUniformScaleComponentMode& operator=(const NonUniformScaleComponentMode&) = delete;
            NonUniformScaleComponentMode(NonUniformScaleComponentMode&&) = delete;
            NonUniformScaleComponentMode& operator=(NonUniformScaleComponentMode&&) = delete;
            ~NonUniformScaleComponentMode();

            // EditorBaseComponentMode overrides ...
            void Refresh() override;
            AZStd::string GetComponentModeName() const override;
            AZ::Uuid GetComponentModeType() const override;

        private:
            AZ::EntityComponentIdPair m_entityComponentIdPair;
            AZStd::unique_ptr<ScaleManipulators> m_manipulators;
            AZ::Vector3 m_initialScale;
        };
    } // namespace Components
} // namespace AzToolsFramework
