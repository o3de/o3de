/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>

namespace WhiteBox
{
    //! Runtime WhiteBoxColliderComponent requests.
    //! @note Addressed by EntityId (a single White Box collider per entity).
    class WhiteBoxColliderRequests : public AZ::ComponentBus
    {
    public:
        //! Rebuild the physics body from the base or boolean cooked mesh (CSG cannot run at runtime).
        //! @param forceRebuild rebuild even if the variant is unchanged, to pick up the current entity scale.
        virtual void BakeCollider(bool useBooleanMesh, bool forceRebuild = false) = 0;

    protected:
        ~WhiteBoxColliderRequests() = default;
    };

    using WhiteBoxColliderRequestBus = AZ::EBus<WhiteBoxColliderRequests>;
} // namespace WhiteBox
