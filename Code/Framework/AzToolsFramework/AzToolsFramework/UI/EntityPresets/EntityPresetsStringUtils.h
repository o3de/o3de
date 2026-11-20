/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/string_view.h>

#include <QByteArray>
#include <QString>
#endif

namespace AzToolsFramework
{
    namespace EntityPresets
    {
        //! UTF-8 conversions between AZStd::string and QString, with explicit lengths.
        //!
        //! Shared here because per-file copies in anonymous namespaces collide in unity builds.

        inline QString ToQString(const AZStd::string_view value)
        {
            return QString::fromUtf8(value.data(), aznumeric_cast<int>(value.size()));
        }

        inline AZStd::string ToAZString(const QString& value)
        {
            const QByteArray utf8 = value.toUtf8();
            return AZStd::string(utf8.constData(), aznumeric_cast<size_t>(utf8.size()));
        }
    } // namespace EntityPresets
} // namespace AzToolsFramework
