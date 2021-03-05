/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates, or 
* a third party where indicated.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,  
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  
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
