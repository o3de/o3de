/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef AZCORE_SCRIPT_CONTEXT_ATTRIBUTES_H
#define AZCORE_SCRIPT_CONTEXT_ATTRIBUTES_H

#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/Math/Crc.h>

namespace AZ
{
    namespace Script
    {
        namespace Attributes
        {
            const static AZ::Crc32 Ignore = AZ_CRC("ScriptIgnore", 0xeb7615e1); ///< Don't use the element in the script reflection
            const static AZ::Crc32 ClassNameOverride = AZ_CRC("ScriptClassNameOverride", 0x891238a3); ///< Provide a custom name for script reflection, that doesn't match the behavior Context name
            const static AZ::Crc32 MethodOverride = AZ_CRC("ScriptFunctionOverride", 0xf89a7882); ///< Use a custom function in the attribute instead of the function  
            const static AZ::Crc32 ConstructorOverride = AZ_CRC("ConstructorOverride", 0xef5ce4aa); ///< You can provide a custom constructor to be called when created from Lua script
            const static AZ::Crc32 EventHandlerCreationFunction = AZ_CRC_CE("EventHandlerCreationFunction"); ///< helps create a handler for any script target so that script functions can be used for AZ::Event signals
            const static AZ::Crc32 GenericConstructorOverride = AZ_CRC("GenericConstructorOverride", 0xe6a1698e); ///< You can provide a custom constructor to be called when creating a script
            const static AZ::Crc32 ReaderWriterOverride = AZ_CRC("ReaderWriterOverride", 0x1ad9ce2a); ///< paired with \ref ScriptContext::CustomReaderWriter allows you to customize read/write to Lua VM
            const static AZ::Crc32 ConstructibleFromNil = AZ_CRC("ConstructibleFromNil", 0x23908169); ///< Applied to classes. Value (bool) specifies if the class be default constructed when nil is provided.
            const static AZ::Crc32 ToolTip = AZ_CRC("ToolTip", 0xa1b95fb0); ///< Add a tooltip for a method/event/property
            const static AZ::Crc32 Category = AZ_CRC("Category", 0x064c19c1); ///< Provide a category to allow for partitioning/sorting/ordering of the element
            const static AZ::Crc32 Deprecated = AZ_CRC("Deprecated", 0xfe49a138); ///< Marks a reflected class, method, EBus or property as deprecated. 
            const static AZ::Crc32 DisallowBroadcast = AZ_CRC("DisallowBroadcast", 0x389b0ac7); ///< Marks a reflected EBus as not allowing Broadcasts, only Events.
            const static AZ::Crc32 ClassConstantValue = AZ_CRC_CE("ClassConstantValue"); ///< Indicates the property is backed by a constant value

            //! Attribute which stores BehaviorAzEventDescription structure which contains
            //! the script name of an AZ::Event and the name of it's parameter arguments
            static constexpr AZ::Crc32 AzEventDescription = AZ_CRC_CE("AzEventDescription");

            //! Applied to AZ Event reflected classes.
            //! Stores a vector<TypeId> containing the Uuid of each event param
            static constexpr AZ::Crc32 EventParameterTypes = AZ_CRC_CE("EventParameterTypes");

            ///< Recommends that the Lua runtime look up the member function in the meta table of the first argument, rather than in the original table
            const static AZ::Crc32 TreatAsMemberFunction = AZ_CRC("TreatAsMemberFunction", 0x64be831a);

            ///< This attribute can be attached to the EditContext Attribute of a reflected class, the BehaviorContext Attribute of a reflected class, method, ebus or property.
            ///< ExcludeFlags can be used to prevent elements from appearing in List, Documentation, etc...
            const static AZ::Crc32 ExcludeFrom = AZ_CRC("ExcludeFrom", 0xa98972fe);
            enum ExcludeFlags : AZ::u64
            {
                List               = 1 << 0, //< The reflected item will be excluded from any list (e.g. node palette)
                Documentation      = 1 << 1, //< The reflected item will be excluded from the Lua class reference
                Unused             = 1 << 2, //< This flag is unused (deprecated)
                ListOnly           = 1 << 3, //< Some elements should be excluded from lists, but available for documentation
                All           = (List | Documentation) //< Used to exclude reflections from lists and documentation
            };

            //! Used to specify the usage of a Behavior Context element (e.g. Class or EBus) designed for automation scripts
            const static AZ::Crc32 Scope = AZ_CRC("Scope", 0x00af55d3); 
            enum class ScopeFlags : AZ::u64
            {
                Launcher = 1 << 0,               //< a type meant for game run-time Launcher client (default value)
                Automation = 1 << 1,             //< a type meant for Editor automation
                Common = (Launcher | Automation) //< a common type used with the Launcher, content creation in the Editor, and the Editor's game mode
            };

            //! Provide a partition hierarchy in a string dotted notation to namespace a script element
            const static AZ::Crc32 Module = AZ_CRC("Module", 0x0c242628);

            //! Provide an alternate name for script elements such as helpful PEP8 Python methods and property aliases
            const static AZ::Crc32 Alias = AZ_CRC("Alias", 0xe16c6b94);

            const static AZ::Crc32 EnableAsScriptEventParamType = AZ_CRC("ScriptEventParam", 0xa41e4cb0);
            const static AZ::Crc32 EnableAsScriptEventReturnType = AZ_CRC("ScriptEventReturn", 0xf89b5337);

            const static AZ::Crc32 Storage = AZ_CRC("ScriptStorage", 0xcd95b44d);
            enum class StorageType
            {
                ScriptOwn, // default, Host allocated memory, Lua will destruct object, Lua will free host-memory via host-supplied function
                RuntimeOwn, // C++,    Host allocated memory, host will destruct object, host will free memory
                Value, // Object is    Lua allocated memory, Lua will destruct object, Lua will free Lua-memory 
            };

            const static AZ::Crc32 Operator = AZ_CRC("ScriptOperator", 0xfee681b6);
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

            const static AZ::Crc32 AssetType = AZ_CRC("AssetType", 0xabbf8d5f); ///< Provide an asset type for a generic AssetId method
        } // Attributes
    } // Script

    // set type for the helper enums
    AZ_TYPE_INFO_SPECIALIZE(Script::Attributes::OperatorType, "{26B98C03-7E07-4E3E-9E31-03DA2168E896}");
    AZ_TYPE_INFO_SPECIALIZE(Script::Attributes::StorageType, "{57FED71F-B590-4002-9599-A48CB50B0F8E}");
} // AZ


#endif // AZCORE_SCRIPT_CONTEXT_ATTRIBUTES_H
