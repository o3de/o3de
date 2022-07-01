/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>

namespace PhysX
{
    //! Provides an interface to the Editor Heightfield Collider component
    class EditorHeightfieldColliderInterface : public AZ::ComponentBus
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        virtual ~EditorHeightfieldColliderInterface() = default;

        //! Signals the component to bake the heightfield
        virtual void RequestHeightfieldBaking() = 0;
    };

    using EditorHeightfieldColliderRequestBus = AZ::EBus<EditorHeightfieldColliderInterface>;
} // namespace PhysX
