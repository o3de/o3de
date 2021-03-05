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

#include <AzCore/EBus/Event.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/std/functional.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/containers/fixed_vector.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/string_view.h>
#include <AzCore/StringFunc/StringFunc.h>

namespace AZ
{
    //! The Settings Registry is the central storage for global settings. Having application-wide settings
    //! stored in a central location allows different tools such as command lines, consoles, configuration
    //! files, etc. to work in a universal way.
    //! Paths in the functions for the Settings Registry follow the the JSON Pointer pattern (see
    //! https://tools.ietf.org/html/rfc6901) such as "/My/Object/Settings" or "/My/Array/0".
    class SettingsRegistryInterface
    {
    public:
        AZ_RTTI(AZ::SettingsRegistryInterface, "{D62619D8-0C0B-4D9F-9FE8-F8EBC330DC55}");

        static constexpr char Extension[] = "setreg";
        static constexpr char PatchExtension[] = "setregpatch";
        static constexpr char RegistryFolder[] = "Registry";

        static constexpr char DevUserRegistryFolder[] = "user" AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING "Registry";


        static constexpr char PlatformFolder[] = "Platform";

        //! Represents a fixed size non-allocating string type that can be used to query the settings registry using Get()
        //! If the value is longer than FixedValueString::max_size(), then either the heap allocating 
        //! AZStd::string overload must be used or the Visit method must be used
        using FixedValueString = AZ::StringFunc::Path::FixedString;

        class Specializations
        {
        public:
            static constexpr size_t MaxTagNameSize = 32;
            static constexpr size_t MaxCount = 15;
            static constexpr size_t NotFound = static_cast<size_t>(-1);
            using TagName = AZStd::fixed_string<MaxTagNameSize>;

            Specializations() = default;
            // Explicitly allowed so a list of specializations can be transparently set.
            Specializations(AZStd::initializer_list<AZStd::string_view> specializations);
            Specializations& operator=(AZStd::initializer_list<AZStd::string_view> specializations);

            bool Append(AZStd::string_view specialization);

            //! Whether or not the specialization is in the list.
            bool Contains(AZStd::string_view specialization) const;
            //! Whether or not the specialization is in the list.
            bool Contains(size_t hash) const;
            //! Gets the location within in the list, with a lower value meaning a higher priority.
            //! return NotFound if the specialization isn't found.
            size_t GetPriority(AZStd::string_view specialization) const;
            //! Gets the location within in the list, with a lower value meaning a higher priority.
            //! return NotFound if the specialization isn't found.
            size_t GetPriority(size_t hash) const;
            //! Returns the number of specializations currently stored.
            size_t GetCount() const;
            //! Gets the specialization at the requested index or an empty string if non were assigned.
            AZStd::string_view GetSpecialization(size_t index) const;

            static size_t Hash(AZStd::string_view specialization);
        private:
            AZStd::fixed_vector<TagName, MaxCount> m_names;
            AZStd::fixed_vector<size_t, MaxCount> m_hashes;
        };

        //! Type of the store value, or None if there's no value stored.
        enum class Type
        {
            NoType,
            Null,
            Boolean,
            Integer,
            FloatingPoint,
            String,
            Array,
            Object
        };

        //! The response to let the visit call know what to do next.
        enum class VisitResponse
        {
            Continue,   //!< Continue visiting.
            Skip,       //!< Skip descending into value. This only applies to objects and arrays.
            Done        //!< Stop traversing the Setting Registry.
        };

        //! The action the visit call is taken
        enum class VisitAction
        {
            Begin,      //!< Begins iterating over the elements of an object or the values in an array.
            Value,      //!< Reports a value.
            End         //!< Ends the iteration over the elements of an object or the values in an array.
        };

        //! File formats supported to load settings from.
        enum class Format
        {
            JsonPatch,      //!< Using the json patch format to merge JSON data into the Settings Registry.
            JsonMergePatch  //!< Using the json merge patch format to merge JSON data into the Settings Registry.
        };

        using NotifyCallback = AZStd::function<void(AZStd::string_view path, Type type)>;
        using NotifyEvent = AZ::Event<AZStd::string_view, Type>;
        using NotifyEventHandler = typename NotifyEvent::Handler;

        using VisitorCallback =
            AZStd::function<VisitResponse(AZStd::string_view path, AZStd::string_view valueName, VisitAction action, Type type)>;
        //! Base class for the visitor class during traversal over the Settings Registry. The type-agnostic function is always
        //! called and, if applicable, the overloaded functions with the appropriate values.
        class Visitor
        {
        public:
            virtual ~Visitor() = 0;

            virtual VisitResponse Traverse(AZStd::string_view path, AZStd::string_view valueName, VisitAction action, Type type)
            { AZ_UNUSED(path); AZ_UNUSED(valueName); AZ_UNUSED(action); AZ_UNUSED(type); return VisitResponse::Continue; }
            virtual void Visit(AZStd::string_view path, AZStd::string_view valueName, Type type, bool value)
            { AZ_UNUSED(path); AZ_UNUSED(valueName); AZ_UNUSED(type); AZ_UNUSED(value); }
            virtual void Visit(AZStd::string_view path, AZStd::string_view valueName, Type type, s64 value)
            { AZ_UNUSED(path); AZ_UNUSED(valueName); AZ_UNUSED(type); AZ_UNUSED(value); }
            virtual void Visit(AZStd::string_view path, AZStd::string_view valueName, Type type, u64 value)
            { AZ_UNUSED(path); AZ_UNUSED(valueName); AZ_UNUSED(type); AZ_UNUSED(value); }
            virtual void Visit(AZStd::string_view path, AZStd::string_view valueName, Type type, double value)
            { AZ_UNUSED(path); AZ_UNUSED(valueName); AZ_UNUSED(type); AZ_UNUSED(value); }
            virtual void Visit(AZStd::string_view path, AZStd::string_view valueName, Type type, AZStd::string_view value)
            { AZ_UNUSED(path); AZ_UNUSED(valueName); AZ_UNUSED(type); AZ_UNUSED(value); }
        };

        SettingsRegistryInterface() = default;
        AZ_DISABLE_COPY_MOVE(SettingsRegistryInterface);
        virtual ~SettingsRegistryInterface() = default;

        //! Returns the type of an entry in the Settings Registry or Type::None if there's no value or the path is invalid.
        virtual Type GetType(AZStd::string_view path) const = 0;
        //! Traverses over the entries in the Settings Registry. Use this version to retrieve the values of entries as well.
        //! @param visitor An instance of a class derived from Visitor that will repeatedly be called as entries are encountered.
        //! @param path An offset at which traversal should start.
        //! @return Whether or not entries could be visited.
        virtual bool Visit(Visitor& visitor, AZStd::string_view path) const = 0;
        //! Traverses over the entries in the Settings Registry. Use this version if only the names and/or types are needed.
        //! @param callback A callback function that will repeatedly be called as entries are encountered. 
        //! @param path An offset at which traversal should start.
        //! @return Whether or not entries could be visited.
        virtual bool Visit(const VisitorCallback& callback, AZStd::string_view path) const = 0;
        //! Register a callback that will be called whenever an entry gets a new/updated value.
        //! @callback The function to call when an entry gets a new/updated value.
        [[nodiscard]] virtual NotifyEventHandler RegisterNotifier(const NotifyCallback& callback) = 0;
        //! Register a callback that will be called whenever an entry gets a new/updated value.
        //! @callback The function to call when an entry gets a new/updated value.
        [[nodiscard]] virtual NotifyEventHandler RegisterNotifier(NotifyCallback&& callback) = 0;

        //! Gets the boolean value at the provided path.
        //! @param result The target to write the result to.
        //! @param path The path to the value.
        //! @return Whether or not the value was retrieved. An invalid path or type-mismatch will return false;
        virtual bool Get(bool& result, AZStd::string_view path) const = 0;
        //! Gets the integer value at the provided path.
        //! @param result The target to write the result to.
        //! @param path The path to the value.
        //! @return Whether or not the value was retrieved. An invalid path or type-mismatch will return false;
        virtual bool Get(s64& result, AZStd::string_view path) const = 0;
        virtual bool Get(u64& result, AZStd::string_view path) const = 0;
        //! Gets the floating point value at the provided path.
        //! @param result The target to write the result to.
        //! @param path The path to the value.
        //! @return Whether or not the value was retrieved. An invalid path or type-mismatch will return false;
        virtual bool Get(double& result, AZStd::string_view path) const = 0;
        //! Gets the string value at the provided path.
        //! @param result The target to write the result to.
        //! @param path The path to the value.
        //! @return Whether or not the value was retrieved. An invalid path or type-mismatch will return false;
        virtual bool Get(AZStd::string& result, AZStd::string_view path) const = 0;
        virtual bool Get(FixedValueString& result, AZStd::string_view path) const = 0;
        //! Gets the object value at the provided path serialized to the target struct/class. Classes retrieved
        //! through this call needs to be registered with the Serialize Context.
        //! Prefer to use GetObject(T& result, AZStd::string_view path) over this one.
        //! @param result The target to write the result to.
        //! @param resultTypeId The type id of the target that's being written to.
        //! @param path The path to the value.
        //! @return Whether or not the value was stored. An invalid path will return false;
        virtual bool GetObject(void* result, Uuid resultTypeID, AZStd::string_view path) const = 0;
        //! Gets the json object value at the provided path serialized to the target struct/class. Classes retrieved
        //! through this call needs to be registered with the Serialize Context.
        //! @param result The target to write the result to.
        //! @param path The path to the value.
        //! @return Whether or not the value was stored. An invalid path will return false;
        template<typename T>
        bool GetObject(T& result, AZStd::string_view path) const { return GetObject(&result, azrtti_typeid(result), path); }

        //! Sets or replaces the boolean value at the provided path.
        //! @param path The path to the value.
        //! @param value The new value to store.
        //! @return Whether or not the value was stored. An invalid path will return false;
        virtual bool Set(AZStd::string_view path, bool value) = 0;
        //! Sets or replaces the integer value at the provided path.
        //! @param path The path to the value.
        //! @param value The new value to store.
        //! @return Whether or not the value was stored. An invalid path will return false;
        virtual bool Set(AZStd::string_view path, s64 value) = 0;
        virtual bool Set(AZStd::string_view path, u64 value) = 0;
        //! Sets or replaces the floating point value at the provided path.
        //! @param path The path to the value.
        //! @param value The new value to store.
        //! @return Whether or not the value was stored. An invalid path will return false;
        virtual bool Set(AZStd::string_view path, double value) = 0;
        //! Sets or replaces the string value at the provided path.
        //! @param path The path to the value.
        //! @param value The new value to store.
        //! @return Whether or not the value was stored. An invalid path will return false;
        virtual bool Set(AZStd::string_view path, AZStd::string_view value) = 0;
        //! Sets or replaces the string value at the provided path.
        //! Note that this overload was added as the compiler considers boolean a more closer match than string_view.
        //! @param path The path to the value.
        //! @param value The new value to store.
        //! @return Whether or not the value was stored. An invalid path will return false;
        virtual bool Set(AZStd::string_view path, const char* value) = 0;
        
        //! Sets the value at the provided path to the serialized version of the provided struct/class.
        //! Classes used for this call need to be registered with the Serialize Context.
        //! Prefer to use SetObject(AZStd::string_view path, const T& result) over this one.
        //! @param path The path to the value.
        //! @param value The new value to store.
        //! @param valueTypeId The type id of the target that's being stored.
        //! @return Whether or not the value was stored. An invalid path will return false;
        virtual bool SetObject(AZStd::string_view path, const void* value, Uuid valueTypeID) = 0;
        template<typename T>
        //! Sets the value at the provided path to the serialized version of the provided struct/class.
        //! Classes used for this call need to be registered with the Serialize Context.
        //! @param path The path to the value.
        //! @param value The new value to store.
        //! @return Whether or not the value was stored. An invalid path will return false;
        bool SetObject(AZStd::string_view path, const T& value) { return SetObject(path, &value, azrtti_typeid(value)); }

        //! Remove the value at the provided path 
        //! @param path The path to a value that should be removed
        //! @return Whether or not the value was stored at the provided path. An invalid path will return false;
        virtual bool Remove(AZStd::string_view path) = 0;

        //! Structure which contains configuration settings for how to parse a single command line argument
        //! It supports supplying a functor for determining if a character is a delimiter
        struct CommandLineArgumentSettings
        {
            inline static constexpr AZStd::string_view CommandLineArgumentDelimiters{ "=:"};
            CommandLineArgumentSettings()
            {
                m_delimiterFunc = [](const char delimiter) -> bool
                {
                    return CommandLineArgumentDelimiters.find_first_of(delimiter) != AZStd::string_view::npos;
                };
            }
            
            //! Callback function which is invoked to determine whether a delimiter has been found
            //! return value of true indicates that a delimiter has been found 
            using DelimiterFunc = AZStd::function<bool(const char delimiter)>;
            DelimiterFunc m_delimiterFunc;
        };
        //! Merges a single command line argument into the settings registry. Command line arguments
        //! take the form of <path>=<value> for instance: "/My/Path=My string value"
        //! Values are interpreted as follows:
        //! - "true" (case sensitive) -> boolean with true value
        //! - "false" (case sensitive) -> boolean with false value
        //! - all digits -> number
        //! - all digits and dot -> floating point number
        //! - Everything else is considered a string.
        //! @param argument The command line argument.
        //! @param structure which contains functors which determine what characters are delimiters
        //! @return True if the command line argument could be parsed, otherwise false.
        virtual bool MergeCommandLineArgument(AZStd::string_view argument, AZStd::string_view rootKey = "",
            const CommandLineArgumentSettings& commandLineSettings = {}) = 0;
        //! Merges the json data provided into the settings registry.
        //! @param data The json data stored in a string.
        //! @param format The format of the provided data.
        //! @return True if the data was successfully merged, otherwise false.
        virtual bool MergeSettings(AZStd::string_view data, Format format) = 0;
        //! Loads a settings file and merges it into the registry.
        //! @param path The path to the registry file.
        //! @param format The format of the text data in the file at the provided path.
        //! @param rootKey The key where the root of the settings file will be stored under.
        //! @param scratchBuffer An optional buffer that's used to load the file into. Use this when loading multiple patches to
        //!     reduce the number of intermediate memory allocations.
        //! @return True if the registry file was successfully merged, otherwise false.
        virtual bool MergeSettingsFile(AZStd::string_view path, Format format, AZStd::string_view rootKey = "",
            AZStd::vector<char>* scratchBuffer = nullptr) = 0;
        //! Loads all settings files in a folder and merges them into the registry.
        //!     With the specializations "a" and "b" and platform "c" the files would be loaded in the order:
        //!     - file1.setreg
        //!     - Platform/c/file1.setreg
        //!     - file1.a.setreg
        //!     - Platform/c/file.b.setreg
        //!     - file1.a.b.setreg
        //!     - file2.setreg
        //! @param path The path to the registry folder.
        //! @param specializations The specializations that will be used filter out registry files.
        //! @param platform An optional name of a platform. Platform overloads are located at <path>/Platform/<platform>/
        //!     Files in a platform are applied in the same order as for the main folder but always after the same file
        //!     in the main folder.
        //! @param scratchBuffer An optional buffer that's used to load the file into. Use this when loading multiple patches to
        //!     reduce the number of intermediate memory allocations.
        //! @return True if the registry folder was successfully merged, otherwise false.
        virtual bool MergeSettingsFolder(AZStd::string_view path, const Specializations& specializations,
            AZStd::string_view platform = {}, AZStd::string_view rootKey = "", AZStd::vector<char>* scratchBuffer = nullptr) = 0;
    };

    inline SettingsRegistryInterface::Visitor::~Visitor() = default;

    using SettingsRegistry = Interface<SettingsRegistryInterface>;
} // namespace AZ
