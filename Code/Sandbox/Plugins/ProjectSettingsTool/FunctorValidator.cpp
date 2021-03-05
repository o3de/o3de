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
#include "FunctorValidator.h"

namespace ProjectSettingsTool
{
    FunctorValidator::FunctorValidator(FunctorType functor)
        : QValidator()
        , m_functor(functor)
    {
    }

    FunctorValidator::FunctorValidator()
        : QValidator()
        , m_functor(nullptr)
    {
    }

    QValidator::State FunctorValidator::validate(QString& input, [[maybe_unused]] int& pos) const
    {
        return m_functor(input).first;
    }

    FunctorValidator::ReturnType FunctorValidator::ValidateWithErrors(const QString& input) const
    {
        return m_functor(input);
    }

    FunctorValidator::FunctorType FunctorValidator::Functor() const
    {
        return m_functor;
    }
} // namespace ProjectSettingsTool

#include <moc_FunctorValidator.cpp>
