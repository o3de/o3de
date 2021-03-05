/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>

namespace WhiteBox
{
    //! WhiteBoxComponent requests.
    class WhiteBoxComponentRequests : public AZ::EntityComponentBus
    {
    public:
        // EBusTraits overrides ...
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        virtual bool WhiteBoxIsVisible() const = 0;

    protected:
        ~WhiteBoxComponentRequests() = default;
    };

    using WhiteBoxComponentRequestBus = AZ::EBus<WhiteBoxComponentRequests>;
} // namespace WhiteBox
