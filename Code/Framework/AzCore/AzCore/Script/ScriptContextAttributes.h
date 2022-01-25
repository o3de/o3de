/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
            static constexpr AZ::Crc32 Ignore = AZ_CRC_CE("ScriptIgnore"); ///< Don't use the element in the script reflection
            static constexpr AZ::Crc32 ClassNameOverride = AZ_CRC_CE("ScriptClassNameOverride"); ///< Provide a custom name for script reflection, that doesn't match the behavior Context name
            static constexpr AZ::Crc32 MethodOverride = AZ_CRC_CE("ScriptFunctionOverride"); ///< Use a custom function in the attribute instead of the function  
            static constexpr AZ::Crc32 ConstructorOverride = AZ_CRC_CE("ConstructorOverride"); ///< You can provide a custom constructor to be called when created from Lua script
            static constexpr AZ::Crc32 DefaultConstructorOverrideIndex = AZ_CRC_CE("DefaultConstructorOverrideIndex"); ///< Use a different class constructor as the default constructor in Lua
            static constexpr AZ::Crc32 EventHandlerCreationFunction = AZ_CRC_CE("EventHandlerCreationFunction"); ///< helps create a handler for any script target so that script functions can be used for AZ::Event signals
            static constexpr AZ::Crc32 GenericConstructorOverride = AZ_CRC_CE("GenericConstructorOverride"); ///< You can provide a custom constructor to be called when creating a script
            static constexpr AZ::Crc32 ReaderWriterOverride = AZ_CRC_CE("ReaderWriterOverride"); ///< paired with \ref ScriptContext::CustomReaderWriter allows you to customize read/write to Lua VM
            static constexpr AZ::Crc32 ConstructibleFromNil = AZ_CRC_CE("ConstructibleFromNil"); ///< Applied to classes. Value (bool) specifies if the class be default constructed when nil is provided.
            static constexpr AZ::Crc32 ToolTip = AZ_CRC_CE("ToolTip"); ///< Add a tooltip for a method/event/property
            static constexpr AZ::Crc32 Category = AZ_CRC_CE("Category"); ///< Provide a category to allow for partitioning/sorting/ordering of the element
            static constexpr AZ::Crc32 Deprecated = AZ_CRC_CE("Deprecated"); ///< Marks a reflected class, method, EBus or property as deprecated. 
            static constexpr AZ::Crc32 DisallowBroadcast = AZ_CRC_CE("DisallowBroadcast"); ///< Marks a reflected EBus as not allowing Broadcasts, only Events.
            static constexpr AZ::Crc32 ClassConstantValue = AZ_CRC_CE("ClassConstantValue"); ///< Indicates the property is backed by a constant value
            static constexpr AZ::Crc32 UseClassIndexAllowNil = AZ_CRC_CE("UseClassIndexAllowNil"); ///< Use the Class__IndexAllowNil method, which will not report an error on accessing undeclared values (allows for nil)

            //! Attribute which stores BehaviorAzEventDescription structure which contains
            //! the script name of an AZ::Event and the name of it's parameter arguments
            static constexpr AZ::Crc32 AzEventDescription = AZ_CRC_CE("AzEventDescription");

            //! Applied to AZ Event reflected classes.
            //! Stores a vector<TypeId> containing the Uuid of each event param
            static constexpr AZ::Crc32 EventParameterTypes = AZ_CRC_CE("EventParameterTypes");

            ///< Recommends that the Lua runtime look up the member function in the meta table of the first argument, rather than in the original table
            static constexpr AZ::Crc32 TreatAsMemberFunction = AZ_CRC_CE("TreatAsMemberFunction");

            ///< This attribute can be attached to the EditContext Attribute of a reflected class, the BehaviorContext Attribute of a reflected class, method, ebus or property.
            ///< ExcludeFlags can be used to prevent elements from appearing in List, Documentation, etc...
            static constexpr AZ::Crc32 ExcludeFrom = AZ_CRC_CE("ExcludeFrom");
            enum ExcludeFlags : AZ::u64
            {
                List               = 1 << 0, //< The reflected item will be excluded from any list (e.g. node palette)
                Documentation      = 1 << 1, //< The reflected item will be excluded from the Lua class reference
                Unused             = 1 << 2, //< This flag is unused (deprecated)
                ListOnly           = 1 << 3, //< Some elements should be excluded from lists, but available for documentation
                All           = (List | Documentation) //< Used to exclude reflections from lists and documentation
            };

            //! Used to specify the usage of a Behavior Context element (e.g. Class or EBus) designed for automation scripts
            static constexpr AZ::Crc32 Scope = AZ_CRC_CE("Scope"); 
            enum class ScopeFlags : AZ::u64
            {
                Launcher = 1 << 0,               //< a type meant for game run-time Launcher client (default value)
                Automation = 1 << 1,             //< a type meant for Editor automation
                Common = (Launcher | Automation) //< a common type used with the Launcher, content creation in the Editor, and the Editor's game mode
            };

            //! Provide a partition hierarchy in a string dotted notation to namespace a script element
            static constexpr AZ::Crc32 Module = AZ_CRC_CE("Module");

            //! Provide an alternate name for script elements such as helpful PEP8 Python methods and property aliases
            static constexpr AZ::Crc32 Alias = AZ_CRC_CE("Alias");

            static constexpr AZ::Crc32 EnableAsScriptEventParamType = AZ_CRC_CE("ScriptEventParam");
            static constexpr AZ::Crc32 EnableAsScriptEventReturnType = AZ_CRC_CE("ScriptEventReturn");

            static constexpr AZ::Crc32 Storage = AZ_CRC_CE("ScriptStorage");
            enum class StorageType
            {
                ScriptOwn, // default, Host allocated memory, Lua will destruct object, Lua will free host-memory via host-supplied function
                RuntimeOwn, // C++,    Host allocated memory, host will destruct object, host will free memory
                Value, // Object is    Lua allocated memory, Lua will destruct object, Lua will free Lua-memory 
            };

            static constexpr AZ::Crc32 Operator = AZ_CRC_CE("ScriptOperator");
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

            static constexpr AZ::Crc32 AssetType = AZ_CRC_CE("AssetType"); ///< Provide an asset type for a generic AssetId method
        } // Attributes
    } // Script

    // set type for the helper enums
    AZ_TYPE_INFO_SPECIALIZE(Script::Attributes::OperatorType, "{26B98C03-7E07-4E3E-9E31-03DA2168E896}");
    AZ_TYPE_INFO_SPECIALIZE(Script::Attributes::StorageType, "{57FED71F-B590-4002-9599-A48CB50B0F8E}");
} // AZ


#endif // AZCORE_SCRIPT_CONTEXT_ATTRIBUTES_H
