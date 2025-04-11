/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Edit/Configuration.h>
#include <AzCore/Name/Name.h>
#include <AzCore/std/containers/span.h>

namespace AZ
{
    namespace RPI
    {
        class MaterialAsset;

        //! Utility for building material property IDs.
        //! These IDs are represented like "groupA.groupB.[...].propertyName".
        //! The groups are optional, in which case the full property ID will just be like "propertyName".
        class ATOM_RPI_EDIT_API MaterialPropertyId
        {
        public:
            //! Creates a MaterialPropertyId from a full name string like "groupA.groupB.[...].propertyName" or just "propertyName".
            //! Also checks the name for validity.
            static MaterialPropertyId Parse(AZStd::string_view fullPropertyId);

            MaterialPropertyId() = default;
            explicit MaterialPropertyId(AZStd::string_view propertyName);
            MaterialPropertyId(AZStd::string_view groupName, AZStd::string_view propertyName);
            MaterialPropertyId(const Name& groupName, const Name& propertyName);
            explicit MaterialPropertyId(const AZStd::span<AZStd::string> names);
            MaterialPropertyId(const AZStd::span<AZStd::string> groupNames, AZStd::string_view propertyName);

            AZ_DEFAULT_COPY_MOVE(MaterialPropertyId);

            operator const Name&() const;

            //! Returns a pointer to the full name ("[groupName].[propertyName]"). 
            //! Same as Name::GetCStr()
            //! This is included for convenience so it can be used for error messages in the same way an AZ::Name is used.
            const char* GetCStr() const;
            
            //! Returns a string_view of the full name ("[groupName].[propertyName]").
            //! Same as Name::GetStringView()
            //! This is included for convenience so it can be used for string comparison in the same way an AZ::Name is used.
            AZStd::string_view GetStringView() const;

            //! Returns a hash of the full name. This is needed for compatibility with NameIdReflectionMap.
            Name::Hash GetHash() const;

            bool operator==(const MaterialPropertyId& rhs) const;
            bool operator!=(const MaterialPropertyId& rhs) const;

            bool IsValid() const;

        private:
            Name m_fullName;
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
