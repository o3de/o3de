/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <Editor/Source/ComponentModes/Joints/JointsComponentModeCommon.h>

namespace PhysX
{
    //! Request bus for Joints ComponentMode operations.
    class JointsComponentModeRequests : public AZ::EntityComponentBus
    {
    public:
        //! Set the current sub-mode.
        virtual void SetCurrentSubMode(JointsComponentModeCommon::SubComponentModes::ModeType newMode) = 0;

        //! Reset the current sub-mode.
        virtual void ResetCurrentSubMode() = 0;

        //! Returns true if the sub-mode should be available for the current component.
        virtual bool IsCurrentSubModeAvailable(JointsComponentModeCommon::SubComponentModes::ModeType mode) const = 0;

    protected:
        ~JointsComponentModeRequests() = default;
    };

    using JointsComponentModeRequestBus = AZ::EBus<JointsComponentModeRequests>;
} // namespace WhiteBox
