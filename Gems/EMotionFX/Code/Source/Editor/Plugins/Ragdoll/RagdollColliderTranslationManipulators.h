/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Editor/Plugins/Ragdoll/RagdollManipulators.h>
#include <AzToolsFramework/Manipulators/TranslationManipulators.h>

namespace EMotionFX
{
    class RagdollColliderTranslationManipulators
        : public RagdollManipulatorsBase
    {
    public:
        RagdollColliderTranslationManipulators();
        void Setup(RagdollManipulatorData& ragdollManipulatorData) override;
        void Refresh(RagdollManipulatorData& ragdollManipulatorData) override;
        void Teardown() override;
        void ResetValues(RagdollManipulatorData& ragdollManipulatorData) override;

    private:
        void OnManipulatorMoved(const AZ::Vector3& startPosition, const AZ::Vector3& offset);

        RagdollManipulatorData m_ragdollManipulatorData;
        AzToolsFramework::TranslationManipulators m_translationManipulators;
    };
} // namespace EMotionFX
