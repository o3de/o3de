/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzQtComponents/Components/ToastNotificationConfiguration.h>

namespace AzQtComponents
{
    ToastConfiguration::ToastConfiguration(ToastType toastType, const QString& title, const QString& description)
        : m_toastType(toastType)
        , m_title(title)
        , m_description(description)
    {
    }
} // namespace AzQtComponents
