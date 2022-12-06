/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>

#include <GradientSignal/Util.h>

namespace GradientSignal
{
    class EditorImageGradientRequests
        : public AZ::ComponentBus
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        //! Start the image modification flow for an ImageGradient that has an Editor component as well.
        //! @return The EntityComponentIdPair for the runtime ImageGradient. This is used with the paintbrush to connect the two together.
        virtual AZ::EntityComponentIdPair StartImageModification() = 0;

        //! End the image modification flow for an ImageGradient that has an Editor component as well.
        virtual void EndImageModification() = 0;

        //! Save the edited runtime image asset.
        virtual bool SaveImage() = 0;
    };

    using EditorImageGradientRequestBus = AZ::EBus<EditorImageGradientRequests>;
}
