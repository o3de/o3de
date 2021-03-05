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
#include "FunctorValidator.h"

#include <AzCore/std/containers/list.h>
#endif

namespace ProjectSettingsTool
{
    // Forward Declaration
    class PropertyImagePreviewCtrl;

    class DefaultImageValidator
        : public FunctorValidator
    {
        Q_OBJECT

    public:
        DefaultImageValidator(const FunctorValidator& validator);

        QValidator::State validate(QString& input, int& pos) const override;
        ReturnType ValidateWithErrors(const QString& input) const override;
        // Adds a specific override to the list for this default override
        void AddOverride(PropertyImagePreviewCtrl* preview);

    protected:
        AZStd::list<const PropertyImagePreviewCtrl*> m_specificOverrides;
    };
} // namespace ProjectSettingsTool
