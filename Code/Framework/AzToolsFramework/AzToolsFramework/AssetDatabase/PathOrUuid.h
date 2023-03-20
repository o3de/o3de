/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Uuid.h>
#include <AzCore/std/string/string.h>

namespace AzToolsFramework::AssetDatabase
{
    class PathOrUuid
    {
    public:
        PathOrUuid() = default;
        explicit PathOrUuid(AZStd::string path);
        explicit PathOrUuid(const char* path);
        explicit PathOrUuid(AZ::Uuid uuid);

        static PathOrUuid Create(AZStd::string val);

        bool IsUuid() const;

        const AZ::Uuid& GetUuid() const;
        const AZStd::string& GetPath() const;
        const AZStd::string& ToString() const;

        bool operator==(const PathOrUuid& rhs) const;
        // Returns true if a non-empty path or non-null UUID has been set
        explicit operator bool() const;

    private:
        AZ::Uuid m_uuid;
        AZStd::string m_path;
    };
}

namespace AZStd
{
    template<>
    struct hash<AzToolsFramework::AssetDatabase::PathOrUuid>
    {
        AZStd::size_t operator()(const AzToolsFramework::AssetDatabase::PathOrUuid& obj) const;
    };
} // namespace AZStd
