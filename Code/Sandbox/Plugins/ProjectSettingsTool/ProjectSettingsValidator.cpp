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

#include "ProjectSettingsTool_precompiled.h"
#include "ProjectSettingsValidator.h"

#include "Validators.h"

namespace ProjectSettingsTool
{
    Validator::Validator()
    {}

    Validator::~Validator()
    {
        for (AZStd::pair<FunctorValidator::FunctorType, FunctorValidator*>& pair : m_validatorToQValidator)
        {
            delete pair.second;
        }
        for (FunctorValidator* qValidator : m_otherValidators)
        {
            delete qValidator;
        }
    }

    FunctorValidator* Validator::GetQValidator(FunctorValidator::FunctorType validator)
    {
        if (validator != nullptr)
        {
            auto iter = m_validatorToQValidator.find(validator);

            if (iter != m_validatorToQValidator.end())
            {
                return iter->second;
            }
            else
            {
                FunctorValidator* qValidator = new FunctorValidator(validator);
                m_validatorToQValidator.insert(
                    AZStd::pair<FunctorValidator::FunctorType, FunctorValidator*>(validator, qValidator));

                return qValidator;
            }
        }
        else
        {
            return nullptr;
        }
    }

    void Validator::TrackThisValidator(FunctorValidator* validator)
    {
        m_otherValidators.push_back(validator);
    }
} // namespace ProjectSettingsTool
