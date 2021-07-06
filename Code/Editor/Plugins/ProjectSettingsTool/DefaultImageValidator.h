/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
