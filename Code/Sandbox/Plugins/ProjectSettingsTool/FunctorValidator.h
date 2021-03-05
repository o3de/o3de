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
