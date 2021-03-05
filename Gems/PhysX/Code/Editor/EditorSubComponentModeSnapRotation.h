
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

#include <Editor/EditorSubComponentModeSnap.h>

namespace PhysX
{
    /// TThis sub-component mode gets an entity position from its base class on mouse down
    /// and sets a rotation (AZ::Quaternion) value in the component that uses it.
    class EditorSubComponentModeSnapRotation
        : public EditorSubComponentModeSnap
    {
    public:
        EditorSubComponentModeSnapRotation(
            const AZ::EntityComponentIdPair& entityComponentIdPair
            , const AZ::Uuid& componentType
            , const AZStd::string& name);
        ~EditorSubComponentModeSnapRotation() override;

    protected:
        // PhysX::EditorSubComponentModeSnap
        void DisplaySpecificSnapType(
            const AzFramework::ViewportInfo& viewportInfo,
            AzFramework::DebugDisplayRequests& debugDisplay,
            const AZ::Vector3& jointPosition,
            const AZ::Vector3& snapDirection,
            float snapLength) override;

        void InitMouseDownCallBack() override;
    };
} // namespace PhysX
