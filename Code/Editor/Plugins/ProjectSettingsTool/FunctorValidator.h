/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/std/utils.h>

#include <QValidator>
#endif


namespace ProjectSettingsTool
{
    class FunctorValidator
        : public QValidator
    {
        Q_OBJECT

    public:
        typedef AZStd::pair<QValidator::State, const QString> ReturnType;
        typedef ReturnType(* FunctorType)(const QString&);

        FunctorValidator(FunctorType functor);

        // Validates using QValidates api
        QValidator::State validate(QString& input, int& pos) const override;
        // Validates and returns the result with an error string if one occurred
        virtual ReturnType ValidateWithErrors(const QString& input) const;
        // Returns the function used to validate
        FunctorType Functor() const;

    protected:
        FunctorValidator();

        // The function to use for validating
        FunctorType m_functor;
    };
} // namespace ProjectSettingsTool
