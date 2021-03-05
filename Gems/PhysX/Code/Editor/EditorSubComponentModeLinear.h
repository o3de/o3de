
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
