/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#pragma once

// AZ
#include <AzCore/Memory/SystemAllocator.h>
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
        AZ_CLASS_ALLOCATOR(DataType, AZ::SystemAllocator, 0);
        AZ_RTTI(DataType, "{B8CBD17E-B8F7-4090-99A7-E9E9970D3EF3}");

        static void Reflect(AZ::ReflectContext* reflection);

        //! Data types can be described by a simple enum value. Client systems can
        //! use whatever value they want as long as each type has a unique value.
        using Enum = uint32_t;

        static const Enum ENUM_INVALID = uint32_t(-1);

        DataType();

        //! Constructs a new DataType object.
        //! @param typeEnum - The main unique ID used by the GraphModel framework for this DataType object.Every DataType in the IGraphContext must have a unique enum value.
        //! @param typeUuid - An alternate unique ID that is used by the node graph UI system. (This is not necessarily the same thing as an RTTI TypeId.The only requirement is that it maps 1:1 with the typeEnum).
        //! @param defaultValue - The default value assigned to any slot that uses this data type upon creation.
        //! @param typeDisplayName - Used for tooltips or other UI elements as well as debug messages.This should be unique, and similar to typeEnum.
        //! @param cppTypeName - The name of the c++ class that the DataType maps to.This is only used for debug messages.
        DataType(Enum typeEnum, AZ::Uuid typeUuid, AZStd::any defaultValue, AZStd::string_view typeDisplayName, AZStd::string_view cppTypeName);

        DataType(const DataType& other);
        virtual ~DataType() = default;

        // Because the DataType class is supposed to be immutable
        bool operator=(const DataType& other) = delete;

        bool operator==(const DataType& other) const;
        bool operator!=(const DataType& other) const;

        bool IsValid() const;

        //! Return the enum value that identifies this DataType
        Enum GetTypeEnum() const { return m_typeEnum; }

        //! Return the type Uuid that corresponds to this DataType
        AZ::Uuid GetTypeUuid() const;

        //! Returns GetTypeUuid() as a string (for convenience)
        AZStd::string GetTypeUuidString() const;

        //! Returns a default value for data of this type
        AZStd::any GetDefaultValue() const;

        //! Returns the C++ type name
        AZStd::string GetCppName() const;

        //! Returns a user friently type name, for UI display
        AZStd::string GetDisplayName() const;

    private:
        Enum m_typeEnum;
        AZ::Uuid m_typeUuid;
        AZStd::any m_defaultValue;
        AZStd::string m_cppName;
        AZStd::string m_displayName;
    };

    
} // namespace GraphModel




