/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "FunctorValidator.h"

#include <AzCore/EBus/EBus.h>

namespace ProjectSettingsTool
{
    class ValidatorTraits
        : public AZ::EBusTraits
    {
    public:
        using Bus = AZ::EBus<ValidatorTraits>;

        // Bus Configuration
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        virtual FunctorValidator* GetValidator(FunctorValidator::FunctorType) = 0;
        virtual void TrackValidator(FunctorValidator*) = 0;
    };

    typedef AZ::EBus<ValidatorTraits> ValidatorBus;
} // namespace ProjectSettingsTool
