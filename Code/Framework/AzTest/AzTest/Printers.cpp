/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/EntityId.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Math/Uuid.h>

#include <ostream>

namespace AZ::IO
{
    void PrintTo(const AZ::IO::PathView& path, ::std::ostream* os)
    {
        *os << "path: " << AZ::IO::Path(path.Native(), AZ::IO::PosixPathSeparator).MakePreferred().c_str();
    }

    void PrintTo(const AZ::IO::Path& path, ::std::ostream* os)
    {
        *os << "path: " << AZ::IO::Path(path.Native(), AZ::IO::PosixPathSeparator).MakePreferred().c_str();
    }

    void PrintTo(const AZ::IO::FixedMaxPath& path, ::std::ostream* os)
    {
        *os << "path: " << AZ::IO::FixedMaxPath(path.Native(), AZ::IO::PosixPathSeparator).MakePreferred().c_str();
    }
} // namespace AZ::IO

namespace AZ
{
    void PrintTo(const AZ::EntityId entityId, ::std::ostream* os)
    {
        *os << entityId.ToString().c_str();
    }

    void PrintTo(const AZ::Uuid& uuid, ::std::ostream* os)
    {
        *os << uuid.ToFixedString().c_str();
    }
} // namespace AZ
