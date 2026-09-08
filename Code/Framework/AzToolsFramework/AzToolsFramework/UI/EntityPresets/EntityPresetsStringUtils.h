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
        //! Conversions between AZStd::string and QString.
        //!
        //! Shared rather than repeated per file. Defining these locally in each .cpp works fine on
        //! its own, but O3DE builds with unity blobs - several translation units concatenated into
        //! one - and two anonymous namespaces in the same blob are the *same* namespace, so the
        //! second definition is a redefinition. Having one inline copy sidesteps that and stops the
        //! conversions drifting apart.
        //!
        //! Both go through UTF-8 with an explicit length: AZStd::string is not null-terminated by
        //! contract and may legitimately contain embedded nulls, so the QString(const char*)
        //! overload would truncate.

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
