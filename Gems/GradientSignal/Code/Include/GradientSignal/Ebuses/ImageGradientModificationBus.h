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
    //! EBus that can be used to modify the image data for an Image Gradient.
    class ImageGradientModifications
        : public AZ::ComponentBus
    {
    public:
         // Overrides the default AZ::EBusTraits handler policy to allow only one listener per entity.
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        //! Start an image modification session.
        //! This will create a modification buffer that contains an uncompressed copy of the current image data.
        virtual void StartImageModification() = 0;

        //! Finish an image modification session.
        //! Currently does nothing, but might eventually need to perform some cleanup logic.
        virtual void EndImageModification() = 0;

        //! Temporary API for testing the end-to-end painting workflow.
        //! This will get replaced with better APIs, likely SetValues() or SetPixels() or something.
        virtual AZStd::vector<float>* GetImageModificationBuffer()  = 0;
    };

    using ImageGradientModificationBus = AZ::EBus<ImageGradientModifications>;
}
