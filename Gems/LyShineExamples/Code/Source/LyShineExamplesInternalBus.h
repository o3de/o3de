/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>

namespace LyShineExamples
{
    class UiDynamicContentDatabase;

    class LyShineExamplesInternal
        : public AZ::EBusTraits
    {

    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        // Public functions

        //! Get the UiDynamicContentDatabase. It is gauranteed to be created when the
        //! gem system component is activated.
        virtual UiDynamicContentDatabase* GetUiDynamicContentDatabase() = 0;
    };
    using LyShineExamplesInternalBus = AZ::EBus<LyShineExamplesInternal>;
} // namespace LyShineExamples
