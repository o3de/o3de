/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>

namespace ImageProcessingAtomEditor
{
    class ImageProcessingEditorRequests
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        /////////////////////////////////////////////////////////////////////////

        //! Open single texture file
        virtual void OpenSourceTextureFile(const AZ::Uuid& textureSourceID) = 0;
    };

    using ImageProcessingEditorRequestBus = AZ::EBus<ImageProcessingEditorRequests>;
}//namespace ImageProcessingAtomEditor

