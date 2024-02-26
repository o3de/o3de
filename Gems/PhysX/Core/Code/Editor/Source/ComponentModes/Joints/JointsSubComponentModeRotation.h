/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Vector3.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <Editor/Source/ComponentModes/PhysXSubComponentModeBase.h>

namespace AzToolsFramework
{
    class AngularManipulator;
}

namespace PhysX
{
    class JointsSubComponentModeRotation final
        : public PhysXSubComponentModeBase
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL;

        JointsSubComponentModeRotation() = default;

        // PhysXSubComponentModeBase ...
        void Setup(const AZ::EntityComponentIdPair& idPair) override;
        void Refresh(const AZ::EntityComponentIdPair& idPair) override;
        void Teardown(const AZ::EntityComponentIdPair& idPair) override;
        void ResetValues(const AZ::EntityComponentIdPair& idPair) override;

    private:
        void InstallManipulatorMouseCallbacks(const AZ::EntityComponentIdPair& idPair);

        AZ::Vector3 m_resetValue = AZ::Vector3::CreateZero();
        AZStd::array<AZStd::shared_ptr<AzToolsFramework::AngularManipulator>, 3> m_manipulators;
    };
}
