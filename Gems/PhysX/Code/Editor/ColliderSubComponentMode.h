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

namespace AZ
{
    class EntityComponentIdPair;
}

namespace PhysX
{
    /// Interface to implement when adding a new editing mode
    /// for collider component mode.
    class ColliderSubComponentMode
    {
    public:
        virtual ~ColliderSubComponentMode() {};

        /// Called when the mode is entered to initialise the mode.
        /// @param idPair The entity/component id pair.
        virtual void Setup(const AZ::EntityComponentIdPair& idPair) = 0;

        /// Called when the mode needs to refresh it's values.
        /// @param idPair The entity/component id pair.
        virtual void Refresh(const AZ::EntityComponentIdPair& idPair) = 0;

        /// Called when the mode exits to perform cleanup.
        /// @param idPair The entity/component id pair.
        virtual void Teardown(const AZ::EntityComponentIdPair& idPair) = 0;

        /// Called when reset hotkey is pressed.
        /// Should reset values in the sub component mode to sensible defaults.
        /// @param idPair The entity/component id pair.
        virtual void ResetValues(const AZ::EntityComponentIdPair& idPair) = 0;
    };
}