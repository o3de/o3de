/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/Locale.h>

namespace AZ::Locale
{
    ScopedSerializationLocale::ScopedSerializationLocale(bool autoActivate)
    {
        if (autoActivate)
        {
            Activate();
        }
    }

    ScopedSerializationLocale::~ScopedSerializationLocale()
    {
        ScopedSerializationLocale_Platform::Deactivate();
    }

    void ScopedSerializationLocale::Activate()
    {
        ScopedSerializationLocale_Platform::Activate();
    }

    void ScopedSerializationLocale::Deactivate()
    {
        ScopedSerializationLocale_Platform::Deactivate();
    }
} // namespace AZ::Locale
