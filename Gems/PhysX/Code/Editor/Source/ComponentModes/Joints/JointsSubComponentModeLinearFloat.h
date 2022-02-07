/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/Memory.h>
#include <AzCore/std/limits.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/string/string.h>
#include <Editor/Source/ComponentModes/PhysXSubComponentModeBase.h>

namespace AZ
{
    class Vector3;
} // namespace AZ

namespace AzToolsFramework
{
    class LinearManipulator;
} // namespace AzToolsFramework

namespace PhysX
{
    class JointsSubComponentModeLinearFloat final
        : public PhysXSubComponentModeBase
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL;

        JointsSubComponentModeLinearFloat(
            const AZStd::string& propertyName, float exponent, float max, float min);

        // PhysXSubComponentModeBase ...
        void Setup(const AZ::EntityComponentIdPair& idPair) override;
        void Refresh(const AZ::EntityComponentIdPair& idPair) override;
        void Teardown(const AZ::EntityComponentIdPair& idPair) override;
        void ResetValues(const AZ::EntityComponentIdPair& idPair) override;

    private:
        float DisplacementToDeltaValue(float displacement) const;
        float ValueToDisplacement(float value) const;

        float m_exponent = 1.0f;
        float m_inverseExponent = 1.0f;
        float m_max = AZStd::numeric_limits<float>::max();
        float m_min = -AZStd::numeric_limits<float>::max();
        float m_resetValue = 0.0f;
        AZStd::string m_propertyName;
        AZStd::shared_ptr<AzToolsFramework::LinearManipulator> m_manipulator;
    };
}
