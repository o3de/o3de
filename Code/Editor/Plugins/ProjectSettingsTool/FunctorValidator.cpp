/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

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
