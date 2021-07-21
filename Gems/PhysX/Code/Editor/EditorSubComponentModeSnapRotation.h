
/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
