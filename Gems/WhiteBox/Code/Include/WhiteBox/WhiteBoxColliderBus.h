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
        //! Rebuild the physics body from one of the pre-baked cooked collision meshes.
        //! @param useBooleanMesh When true, use the boolean-evaluated mesh baked at build
        //! time; when false, use the base mesh. If the requested variant was not baked the
        //! call is a no-op and the current body is left unchanged.
        //! @note CSG cannot be evaluated at runtime, so this selects between geometry that
        //! was cooked in the Editor when the game entity was built.
        virtual void BakeCollider(bool useBooleanMesh) = 0;

    protected:
        ~WhiteBoxColliderRequests() = default;
    };

    using WhiteBoxColliderRequestBus = AZ::EBus<WhiteBoxColliderRequests>;
} // namespace WhiteBox
