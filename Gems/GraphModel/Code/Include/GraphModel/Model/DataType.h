/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

// AZ
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/any.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/string_view.h>

// GraphModel
#include <GraphModel/Model/Common.h>

namespace AZ
{
    class ReflectContext;
}

namespace GraphModel
{
    //! Provides a way for client systems to describe each data type that they support, including a 
    //! unique enum value, the AZ type Uuid, and a user-friendly display name. Client systems may 
    //! subclass DataType if desired, for example to provide additional name formats.
    class DataType
    {
    public:
        AZ_CLASS_ALLOCATOR(DataType, AZ::SystemAllocator);
        AZ_RTTI(DataType, "{B8CBD17E-B8F7-4090-99A7-E9E9970D3EF3}");

        static void Reflect(AZ::ReflectContext* context);

        //! Data types can be described by a simple enum value. Client systems can
        //! use whatever value they want as long as each type has a unique value.
        using Enum = uint32_t;

        static const Enum ENUM_INVALID = uint32_t(-1);

        DataType() = default;

        //! Constructs a new DataType object.
        //! @param typeEnum - The main unique ID used by the GraphModel framework for this DataType object.Every DataType in the GraphContext must have a unique enum value.
        //! @param typeUuid - An alternate unique ID that is used by the node graph UI system. (This is not necessarily the same thing as an RTTI TypeId.The only requirement is that it maps 1:1 with the typeEnum).
        //! @param defaultValue - The default value assigned to any slot that uses this data type upon creation.
        //! @param typeDisplayName - Used for tooltips or other UI elements as well as debug messages.This should be unique, and similar to typeEnum.
        //! @param cppTypeName - The name of the c++ class that the DataType maps to.This is only used for debug messages.
        //! @param valueValidator - An optional function used to check for specific values compatible with this data type.
        DataType(
            Enum typeEnum,
            const AZ::Uuid& typeUuid,
            const AZStd::any& defaultValue,
            AZStd::string_view typeDisplayName,
            AZStd::string_view cppTypeName,
            const AZStd::function<bool(const AZStd::any&)>& valueValidator = {});

        template<typename T>
        DataType(Enum typeEnum, const T& defaultValue, AZStd::string_view typeDisplayName)
            : DataType(typeEnum, AZ::AzTypeInfo<T>::Uuid(), AZStd::any(defaultValue), typeDisplayName, AZ::AzTypeInfo<T>::Name()){};

        template<typename T>
        DataType(Enum typeEnum, const T& defaultValue)
            : DataType(typeEnum, AZ::AzTypeInfo<T>::Uuid(), AZStd::any(defaultValue), AZ::AzTypeInfo<T>::Name(), AZ::AzTypeInfo<T>::Name()){};

        virtual ~DataType() = default;

        bool operator==(const DataType& other) const;
        bool operator!=(const DataType& other) const;

        bool IsValid() const;

        //! Return the enum value that identifies this DataType
        Enum GetTypeEnum() const;

        //! Return the type Uuid that corresponds to this DataType
        const AZ::Uuid& GetTypeUuid() const;

        //! Returns GetTypeUuid() as a string (for convenience)
        AZStd::string GetTypeUuidString() const;

        //! Returns a default value for data of this type
        const AZStd::any& GetDefaultValue() const;

        //! Returns a user friently type name, for UI display
        const AZStd::string& GetDisplayName() const;

        //! Returns the C++ type name
        const AZStd::string& GetCppName() const;

        // Return true if the input type id matches the storage type ID or the type ID of the default value.
        // This supports special cases where the same underlying type is registered with multiple type IDs. 
        bool IsSupportedType(const AZ::Uuid& typeUuid) const;

        // Return true if the input value is of a supported type or is accepted by the value validator callback.
        bool IsSupportedValue(const AZStd::any& value) const;

    private:
        Enum m_typeEnum = ENUM_INVALID;
        AZ::Uuid m_typeUuid;
        AZStd::any m_defaultValue;
        AZStd::string m_cppName = "INVALID";
        AZStd::string m_displayName = "INVALID";
        AZStd::function<bool(const AZStd::any&)> m_valueValidator;
    };
} // namespace GraphModel




