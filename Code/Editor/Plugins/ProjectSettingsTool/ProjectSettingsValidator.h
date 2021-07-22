/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "FunctorValidator.h"

#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/unordered_map.h>


namespace ProjectSettingsTool
{
    class Validator
    {
    public:
        typedef FunctorValidator::ReturnType ValidatorReturnType;
        typedef FunctorValidator::FunctorType ValidatorType;

        Validator();
        ~Validator();

        // Finds the QValidator for a given validator or makes one and returns it
        FunctorValidator* GetQValidator(FunctorValidator::FunctorType validator);
        // Tracks this QValidator and deletes it in the destructor
        void TrackThisValidator(FunctorValidator* validator);

    private:
        typedef AZStd::unordered_map<FunctorValidator::FunctorType, FunctorValidator*> ValidatorToQValidatorType;
        typedef AZStd::list<FunctorValidator*> QValidatorList;

        AZ_DISABLE_COPY_MOVE(Validator);

        // Maps validator functions to QValidators
        ValidatorToQValidatorType m_validatorToQValidator;
        // Tracks allocations of other QValidators so they don't leak
        QValidatorList m_otherValidators;
    };
} // namespace ProjectSettingsTool
