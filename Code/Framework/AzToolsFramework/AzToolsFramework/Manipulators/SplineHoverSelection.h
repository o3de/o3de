/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/smart_ptr/weak_ptr.h>
#include <AzToolsFramework/Manipulators/HoverSelection.h>

namespace AZ
{
    class Spline;
    class EntityComponentIdPair;
} // namespace AZ

namespace AzToolsFramework
{
    class SplineSelectionManipulator;

    //! SplineHoverSelection is a concrete implementation of HoverSelection wrapping a Spline and
    //! SplineManipulator. The underlying manipulators are used to control selection.
    class SplineHoverSelection : public HoverSelection
    {
    public:
        explicit SplineHoverSelection(
            const AZ::EntityComponentIdPair& entityComponentIdPair,
            ManipulatorManagerId managerId,
            const AZStd::shared_ptr<AZ::Spline>& spline);
        SplineHoverSelection(const SplineHoverSelection&) = delete;
        SplineHoverSelection& operator=(const SplineHoverSelection&) = delete;
        ~SplineHoverSelection();

        void Register(ManipulatorManagerId managerId) override;
        void Unregister() override;
        void SetBoundsDirty() override;
        void Refresh() override;
        void SetSpace(const AZ::Transform& worldFromLocal) override;
        void SetNonUniformScale(const AZ::Vector3& nonUniformScale) override;

    private:
        AZStd::shared_ptr<SplineSelectionManipulator> m_splineSelectionManipulator; //!< Manipulator for adding points to spline.
    };
} // namespace AzToolsFramework
