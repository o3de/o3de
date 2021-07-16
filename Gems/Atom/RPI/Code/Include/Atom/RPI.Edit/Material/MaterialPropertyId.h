/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Name/Name.h>

namespace AZ
{
    namespace RPI
    {
        class MaterialAsset;

        //! Utility for building material property names consisting of a group name and a property sub-name.
        //! Represented as "[groupName].[propertyName]".
        //! The group name is optional, in which case the ID will just be "[propertyName]".
        class MaterialPropertyId
        {
        public:                
            static bool IsValidName(AZStd::string_view name);
            static bool IsValidName(const AZ::Name& name);

            //! Creates a MaterialPropertyId from a full name string like "[groupName].[propertyName]" or just "[propertyName]"
            static MaterialPropertyId Parse(AZStd::string_view fullPropertyId);

            MaterialPropertyId() = default;
            MaterialPropertyId(AZStd::string_view groupName, AZStd::string_view propertyName);
            MaterialPropertyId(const Name& groupName, const Name& propertyName);

            AZ_DEFAULT_COPY_MOVE(MaterialPropertyId);

            const Name& GetGroupName() const;
            const Name& GetPropertyName() const;
            const Name& GetFullName() const;

            //! Returns a pointer to the full name ("[groupName].[propertyName]"). 
            //! This is included for convenience so it can be used for error messages in the same way an AZ::Name is used.
            const char* GetCStr() const;

            //! Returns a hash of the full name. This is needed for compatibility with NameIdReflectionMap.
            Name::Hash GetHash() const;

            bool operator==(const MaterialPropertyId& rhs) const;
            bool operator!=(const MaterialPropertyId& rhs) const;

            bool IsValid() const;

        private:
            Name m_fullName;
            Name m_groupName;
            Name m_propertyName;
        };
    } // namespace RPI

} // namespace AZ

namespace AZStd
{
    template<>
    struct hash<AZ::RPI::MaterialPropertyId>
    {
        size_t operator()(const AZ::RPI::MaterialPropertyId& propertyId) const
        {
            return propertyId.GetHash();
        }
    };
}
