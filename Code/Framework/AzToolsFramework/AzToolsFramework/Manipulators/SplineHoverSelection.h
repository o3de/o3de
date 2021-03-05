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

#include <AzCore/std/smart_ptr/weak_ptr.h>
#include <AzToolsFramework/Manipulators/HoverSelection.h>

namespace AZ
{
    class Spline;
    class EntityComponentIdPair;
}

namespace AzToolsFramework
{
    class SplineSelectionManipulator;

    /// SplineHoverSelection is a concrete implementation of HoverSelection wrapping a Spline and
    /// SplineManipulator. The underlying manipulators are used to control selection.
    class SplineHoverSelection
        : public HoverSelection
    {
    public:
        explicit SplineHoverSelection(
            const AZ::EntityComponentIdPair& entityComponentIdPair, ManipulatorManagerId managerId,
            const AZStd::shared_ptr<AZ::Spline>& spline);
        SplineHoverSelection(const SplineHoverSelection&) = delete;
        SplineHoverSelection& operator=(const SplineHoverSelection&) = delete;
        ~SplineHoverSelection();

        void Register(ManipulatorManagerId managerId) override;
        void Unregister() override;
        void SetBoundsDirty() override;
        void Refresh() override;
        void SetSpace(const AZ::Transform& worldFromLocal) override;

    private:
        AZStd::shared_ptr<SplineSelectionManipulator> m_splineSelectionManipulator; ///< Manipulator for adding points to spline.
    };
} // namespace AzToolsFramework