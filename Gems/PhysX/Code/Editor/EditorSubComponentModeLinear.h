
/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Editor/EditorSubComponentModeBase.h>

namespace AzToolsFramework
{
    class LinearManipulator;
}

namespace PhysX
{
    class EditorSubComponentModeLinear
        : public PhysX::EditorSubComponentModeBase
    {
    public:
        EditorSubComponentModeLinear(
            const AZ::EntityComponentIdPair& entityComponentIdPair
            , const AZ::Uuid& componentType
            , const AZStd::string& name
            , float exponent
            , float max
            , float min);
        ~EditorSubComponentModeLinear();

        // PhysX::EditorSubComponentModeBase
        void Refresh() override;

    private:
        float DisplacementToDeltaValue(float displacement) const;
        float ValueToDisplacement(float value) const;
        
        float m_exponent = 1.0f;
        float m_inverseExponent = 1.0f;
        AZStd::shared_ptr<AzToolsFramework::LinearManipulator> m_manipulator;
        float m_max = FLT_MAX;
        float m_min = -FLT_MAX;
    };
} // namespace PhysX
