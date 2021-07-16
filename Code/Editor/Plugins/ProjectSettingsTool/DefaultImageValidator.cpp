/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "DefaultImageValidator.h"

#include "PropertyImagePreview.h"

// Error for no default image set in overrides
const static char* noDefaultImageError = "Default must be set if not all dpi overrides are set";

namespace ProjectSettingsTool
{
    DefaultImageValidator::DefaultImageValidator(const FunctorValidator& validator)
        : FunctorValidator(validator.Functor())
    {
    }

    QValidator::State DefaultImageValidator::validate(QString& input, [[maybe_unused]] int& pos) const
    {
        return ValidateWithErrors(input).first;
    }

    FunctorValidator::ReturnType DefaultImageValidator::ValidateWithErrors(const QString& input) const
    {
        ReturnType result = m_functor(input);
        if (result.first == QValidator::Acceptable)
        {
            if (input.isEmpty())
            {
                int numCustoms = 0;
                // Check all specific overrides to see if any are set
                for (const PropertyImagePreviewCtrl* preview : m_specificOverrides)
                {
                    if (!preview->GetValue().isEmpty())
                    {
                        ++numCustoms;
                    }
                }
                if (numCustoms != 0 && numCustoms != m_specificOverrides.size())
                {
                    return FunctorValidator::ReturnType(QValidator::Intermediate, tr(noDefaultImageError));
                }
            }
        }

        return result;
    }

    void DefaultImageValidator::AddOverride(PropertyImagePreviewCtrl* preview)
    {
        m_specificOverrides.push_back(preview);
    }
} // namespace ProjectSettingsTool
#include <moc_DefaultImageValidator.cpp>
