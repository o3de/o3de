/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <WhiteBox/WhiteBoxToolApi.h>

namespace WhiteBox
{
    //! EditorWhiteBoxColliderComponent requests.
    class EditorWhiteBoxColliderRequests : public AZ::ComponentBus
    {
    public:
        //! Creates a physical representation of the White Box mesh.
        virtual void CreatePhysics(const WhiteBoxMesh& mesh) = 0;
        //! Destroys the physics mesh.
        virtual void DestroyPhysics() = 0;

    protected:
        ~EditorWhiteBoxColliderRequests() = default;
    };

    using EditorWhiteBoxColliderRequestBus = AZ::EBus<EditorWhiteBoxColliderRequests>;

} // namespace WhiteBox
