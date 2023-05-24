/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <WhiteBox/WhiteBoxToolApi.h>

namespace WhiteBox
{
    //! Enumerates the different type of transform sub-modes available.
    enum class TransformType
    {
        Translation,
        Rotation,
        Scale
    };

    //! Request bus for White Box ComponentMode operations while in 'transform' mode.
    class EditorWhiteBoxTransformModeRequests : public AZ::EntityComponentBus
    {
    public:
        //! Change the TransformType for the WhiteBox Transform sub-mode.
        virtual void ChangeTransformType(TransformType subModeType) = 0;

    protected:
        ~EditorWhiteBoxTransformModeRequests() = default;
    };

    using EditorWhiteBoxTransformModeRequestBus = AZ::EBus<EditorWhiteBoxTransformModeRequests>;
} // namespace WhiteBox
