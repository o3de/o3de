/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>

namespace AZ::Render
{
    //! The enumerations of the editor state feedback effects in the editor state pass system.
    enum class EditorState : AZ::u8
    {
        FocusMode, //!< The focus mode effect when prefabs are being edited.
        EntitySelection //!< The entity selection outlining effect.
    };

    //! Bus for controlling the state of the editor state feedback effects.
    class EditorStateRequests 
        : public AZ::EBusTraits
    {
    public:
        // EBusTraits overrides ...
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = EditorState;

        //! Enables/disables the specified editor state feedback effect.
        virtual void SetEnabled(bool enabled) = 0;
    };

    using EditorStateRequestsBus = AZ::EBus<EditorStateRequests>;
} // namespace AZ::Render
