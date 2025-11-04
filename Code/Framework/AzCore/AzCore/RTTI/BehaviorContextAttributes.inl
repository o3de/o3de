/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/Math/Crc.h>

namespace AZ
{
    namespace Script
    {
        namespace Attributes
        {
            const static AZ::Crc32 Ignore = AZ_CRC_CE("ScriptIgnore"); ///< Don't use the element in the script reflection
            const static AZ::Crc32 ClassNameOverride = AZ_CRC_CE("ScriptClassNameOverride"); ///< Provide a custom name for script reflection, that doesn't match the behavior Context name
            const static AZ::Crc32 MethodOverride = AZ_CRC_CE("ScriptFunctionOverride"); ///< Use a custom function in the attribute instead of the function
            const static AZ::Crc32 ConstructorOverride = AZ_CRC_CE("ConstructorOverride"); ///< You can provide a custom constructor to be called when created from Lua script
            const static AZ::Crc32 GenericConstructorOverride = AZ_CRC_CE("GenericConstructorOverride"); ///< You can provide a custom constructor to be called when creating a script
            const static AZ::Crc32 ReaderWriterOverride = AZ_CRC_CE("ReaderWriterOverride"); ///< paired with \ref ScriptContext::CustomReaderWriter allows you to customize read/write to Lua VM
            const static AZ::Crc32 ConstructibleFromNil = AZ_CRC_CE("ConstructibleFromNil"); ///< Applied to classes. Value (bool) specifies if the class be default constructed when nil is provided.
            const static AZ::Crc32 ToolTip = AZ_CRC_CE("ToolTip"); ///< Add a tooltip for a method/event/property
            const static AZ::Crc32 Category = AZ_CRC_CE("Category"); ///< Provide a category to allow for partitioning/sorting/ordering of the element
            const static AZ::Crc32 Deprecated = AZ_CRC_CE("Deprecated"); ///< Marks a reflected class, method, EBus or property as deprecated.
            const static AZ::Crc32 DisallowBroadcast = AZ_CRC_CE("DisallowBroadcast"); ///< Marks a reflected EBus as not allowing Broadcasts, only Events.
            ///< This attribute can be attached to the EditContext Attribute of a reflected class, the BehaviorContext Attribute of a reflected class, method, ebus or property.
            ///< ExcludeFlags can be used to prevent elements from appearing in List, Documentation, etc...
            const static AZ::Crc32 ExcludeFrom = AZ_CRC_CE("ExcludeFrom");
            enum ExcludeFlags : AZ::u64
            {
                List = 1 << 0,
                Documentation = 1 << 1,
                Preview = 1 << 2,
                ListOnly = 1 << 3,
                All = (List | Documentation | Preview)
            };

            const static AZ::Crc32 Storage = AZ_CRC_CE("ScriptStorage");
            enum class StorageType
            {
                ScriptOwn, // default
                RuntimeOwn, // C++
                Value, // Object is stored by value in the VM
            };

            const static AZ::Crc32 Operator = AZ_CRC_CE("ScriptOperator");
            enum class OperatorType
            {
                // note storage policy can be T*,T (only if we store raw pointers), shared_ptr<T>, intrusive pointer<T>
                Add,       // '+'      StoragePolicy<T> add(const StoragePolicy<T>& rhs)
                Sub,       // '-'      --"--
                Mul,       // '*'      --"--
                Div,       // '/'      --"--
                Mod,       // '%'      --"--
                Pow,       // '^'      --"--
                Unary,     // '- (unary)'  StoragePolicy<T> unary(const StoragePolicy<T>&);  // const T& usually not used so it can skipped from the signature
                Concat,    // '..'     StoragePolicy<T> concat(const StoragePolicy<T>& rhs);
                Length,    // '#'      int length(const StoragePolicy<T>&); // const T& not used normally so it can be skipped from the signature
                Equal,     // '=='     bool eq(const StoragePolicy<T>& rhs)
                LessThan,  // '<'      --"--
                LessEqualThan, // '<='     --"---
                ToString,  // converts object to string AZStd::string tostring()
                IndexRead, // given a key/index, you should return a value at this key/index. Keep in mind that IndexRead/Write functions can't take string as index as it's reserved for functions and properties
                IndexWrite, // given a key/index and a value, you can store it in the class
            };
        } // Attributes
    } // Script

    // set type for the helper enums
    AZ_TYPE_INFO_SPECIALIZE(Script::Attributes::OperatorType, "{26B98C03-7E07-4E3E-9E31-03DA2168E896}");
    AZ_TYPE_INFO_SPECIALIZE(Script::Attributes::StorageType, "{57FED71F-B590-4002-9599-A48CB50B0F8E}");

}
