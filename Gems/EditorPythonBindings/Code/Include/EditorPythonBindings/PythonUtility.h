/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Outcome/Outcome.h>
#include <AzCore/RTTI/AttributeReader.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/std/optional.h>
#include <AzCore/std/string/string_view.h>
#include <EditorPythonBindings/PythonCommon.h>
#include <pybind11/pybind11.h>

namespace AZ
{
    struct BehaviorParameter;
    struct BehaviorArgument;
    class BehaviorMethod;
}

namespace EditorPythonBindings
{
    namespace Scope
    {
        inline bool IsBehaviorFlaggedForEditor(const AZ::AttributeArray& attributes)
        {
            // defaults to Launcher
            AZ::Script::Attributes::ScopeFlags scopeType = AZ::Script::Attributes::ScopeFlags::Launcher;
            AZ::Attribute* scopeAttribute = AZ::FindAttribute(AZ::Script::Attributes::Scope, attributes);
            if (scopeAttribute)
            {
                AZ::AttributeReader scopeAttributeReader(nullptr, scopeAttribute);
                scopeAttributeReader.Read<AZ::Script::Attributes::ScopeFlags>(scopeType);
            }
            return (scopeType == AZ::Script::Attributes::ScopeFlags::Automation || scopeType == AZ::Script::Attributes::ScopeFlags::Common);
        }
        inline void FetchScriptName(const AZ::AttributeArray& attributes, AZStd::string& baseName)
        {
            AZ::Attribute* scriptNameAttribute = AZ::FindAttribute(AZ::Script::Attributes::Alias, attributes);
            if (scriptNameAttribute)
            {
                AZ::AttributeReader scopeAttributeReader(nullptr, scriptNameAttribute);
                scopeAttributeReader.Read<AZStd::string>(baseName);
            }
        }
    }

    namespace Module
    {
        using PackageMapType = AZStd::unordered_map<AZStd::string, pybind11::module>;

        //! Finds or creates a sub-module to add a base parent module; create all the sub-modules as well
        //! @param modulePackageMap keeps track of the known modules
        //! @param moduleName can be a dot separated string such as "mygen.mypackage.mymodule"
        //! @param parentModule the module to add new sub-modules
        //! @param fallbackModule the module to add new sub-modules
        //! @param alertUsingFallback issue a warning if using the fallback module
        //! @return the new submodule
        pybind11::module DeterminePackageModule(PackageMapType& modulePackageMap, AZStd::string_view moduleName, pybind11::module parentModule, pybind11::module fallbackModule, bool alertUsingFallback);

        inline AZStd::optional<AZStd::string_view> GetName(const AZ::AttributeArray& attributes)
        {
            AZ::Attribute* moduleAttribute = AZ::FindAttribute(AZ::Script::Attributes::Module, attributes);
            if (moduleAttribute)
            {
                const char* moduleName = nullptr;
                AZ::AttributeReader scopeAttributeReader(nullptr, moduleAttribute);
                scopeAttributeReader.Read<const char*>(moduleName);
                if (moduleName)
                {
                    return { moduleName };
                }
            }
            return {};
        }
    }

    namespace Convert
    {
        // allocation pattern for BehaviorValueParameters being stored in the stack and needs to be cleaned at the end of a block
        using VariableDeleter = AZStd::function<void()>;

        struct StackVariableAllocator final 
            : public AZStd::static_buffer_allocator<256, 16>
        {
        public:
            ~StackVariableAllocator();
            void StoreVariableDeleter(VariableDeleter&& deleter);

        private:
            AZStd::vector<VariableDeleter> m_cleanUpItems;
        };

        //! Converts a behavior value parameter to a Python object
        //! @param behaviorValue is a parameter that came from a result or some prepared behavior value
        //! @param stackVariableAllocator manages the allocated parameter while in scope
        //! @param error if not null, receives why the value could not be converted; a null pointer converts to None without an error
        //! @return a valid Python object or None if no conversion was possible
        pybind11::object BehaviorValueParameterToPython(
            AZ::BehaviorArgument& behaviorValue, Convert::StackVariableAllocator& stackVariableAllocator, AZStd::string* error = nullptr);

        //! Converts Python object to a behavior value parameter using an existing behaviorArgument from a Behavior Method
        //! @param behaviorArgument the stored argument slot from a Behavior Method to match with the pyObj to covert in the parameter
        //! @param parameter is the output of the conversion from Python to a Behavior value
        //! @param stackVariableAllocator manages the allocated parameter while in scope
        //! @return true if the conversion happened
        bool PythonToBehaviorValueParameter(const AZ::BehaviorParameter& behaviorArgument, pybind11::object pyObj, AZ::BehaviorArgument& parameter, Convert::StackVariableAllocator& stackVariableAllocator);

        //! Converts Python object to a PythonProxyObject, if possible
        //! @param behaviorArgument A stored PythonProxyObject in Python, returns FALSE if the Python object does not point to a
        //! PythonProxyObject
        //! @param parameter is the output of the conversion from Python to a Behavior value
        //! @return true if the conversion happened
        bool PythonProxyObjectToBehaviorValueParameter(const AZ::BehaviorParameter& behaviorArgument, pybind11::object pyObj, AZ::BehaviorArgument& parameter);

        //! Gets a readable type name for the Python object; this will unwrap a PythonProxyObject to find its underlying type name
        //! @param pyObj any valid Python object value
        //! @return text form of the Python object value type
        AZStd::string GetPythonTypeName(pybind11::object pyObj);
    }

    namespace PythonProxyObjectManagement
    {
        //! Creates a Python object that owns a copy of the value at data, made with the class's reflected cloner.
        //! When data is null, the proxy holds a default constructed value of the type instead.
        //! Returns None and sets a Python TypeError if the type is not a reflected class.
        //! The caller must hold the Python interpreter lock.
        pybind11::object CreatePythonProxyObject(const AZ::TypeId& typeId, void* data);

        //! Checks if function can be reflected as a class member method
        bool IsMemberLike(const AZ::BehaviorMethod& method, const AZ::TypeId& typeId);
    } // namespace PythonProxyObjectManagement

    namespace Call
    {
        //! The Python value a structured call returns, or a message saying why the call failed
        using StructuredResult = AZ::Outcome<pybind11::object, AZStd::string>;

        //! Calls a BehaviorMethod with the same marshaling as StaticMethod and ClassMethod, but reports a wrong
        //! argument count, a failed argument conversion, a failed call or a result that cannot be marshaled as an error
        //! instead of returning None.
        //! @param target None for a non-member function; otherwise a PythonProxyObject that becomes the first argument
        //! @param didInvoke if not null, set to true once the native method has run, even when the call then fails
        //! The caller must hold the Python interpreter lock.
        StructuredResult StructuredMethod(
            AZ::BehaviorMethod* method, pybind11::object target, pybind11::args args, bool* didInvoke = nullptr);

        //! Calls a member BehaviorMethod on native storage that no proxy owns yet, such as memory allocated for a constructor.
        //! The call borrows self only for its duration.
        StructuredResult StructuredMethod(
            AZ::BehaviorMethod* method, AZ::BehaviorObject self, pybind11::args args, bool* didInvoke = nullptr);

        //! Constructs a reflected class into a new PythonProxyObject that Python owns.
        //! @param constructorIndex an index into behaviorClass.m_constructors, or empty to use the default constructor
        //! Fails if the class is not flagged for editor automation or the index is out of range.
        StructuredResult StructuredConstruct(
            const AZ::BehaviorClass& behaviorClass, AZStd::optional<size_t> constructorIndex, pybind11::args args);

        //! Calls a BehaviorMethod with a tuple of arguments for non-member functions
        pybind11::object StaticMethod(AZ::BehaviorMethod* behaviorMethod, pybind11::args args);

        //! Calls a BehaviorMethod with a tuple of arguments for member class level functions
        pybind11::object ClassMethod(AZ::BehaviorMethod* behaviorMethod, AZ::BehaviorObject self, pybind11::args args);
    } // namespace Call

    namespace Text
    {
        //! Reads a string or const char* attribute, such as a tooltip; returns an empty string if it is not present
        AZStd::string ReadStringAttribute(const AZ::AttributeArray& attributes, const AZ::Crc32& attribute);

        class PythonBehaviorDescription
        {
        public:
            //! Get Python type for behavior typeId.
            AZStd::string_view FetchPythonTypeAndTraits(const AZ::TypeId& typeId, AZ::u32 traits);
            AZStd::string FetchPythonTypeName(const AZ::BehaviorParameter& param);
            AZStd::string FetchOutcomeType(const AZ::TypeId& typeId);

            //! Creates a string containing bus events and documentation.
            AZStd::string BusDefinition(const AZStd::string_view& busName, const AZ::BehaviorEBus* behaviorEBus);

            //! Creates a string with class or global method definition and documentation.
            //! @param methodName the name of the method
            //! @param behaviorMethod the reflected behavior method to document
            //! @param behaviorClass the class that the method belongs to (nullptr for non member functions)
            //! @param defineTooltip if true, the tooltip will be included in the definition
            //! @param defineDebugDescription if true, the debug description will be included in the definition
            AZStd::string MethodDefinition(
                const AZStd::string_view& methodName,
                const AZ::BehaviorMethod& behaviorMethod,
                const AZ::BehaviorClass* behaviorClass = nullptr,
                bool defineTooltip = false,
                bool defineDebugDescription = false);

            //! Creates a string with class definition and documentation.
            //! @param behaviorClass the reflected behavior class to document
            //! @param className the name of the class
            //! @param defineProperties if true, the properties will be included in the definition
            //! @param defineMethods if true, the methods will be included in the definition
            //! @param defineTooltip if true, the tooltip will be included in the definition
            AZStd::string ClassDefinition(
                const AZ::BehaviorClass* behaviorClass,
                const AZStd::string_view& className,
                bool defineProperties = true,
                bool defineMethods = true,
                bool defineTooltip = false);

            //! Creates a property definition
            AZStd::string PropertyDefinition(
                AZStd::string_view propertyName, int level, const AZ::BehaviorProperty& property, const AZ::BehaviorClass* behaviorClass);

            AZStd::string GlobalPropertyDefinition(
                const AZStd::string_view& moduleName,
                const AZStd::string_view& propertyName,
                const AZ::BehaviorProperty& behaviorProperty,
                bool needsHeader = true);

        private:
            AZStd::string FetchListType(const AZ::TypeId& typeId);
            AZStd::string FetchMapType(const AZ::TypeId& typeId);

            using TypeMap = AZStd::unordered_map<AZ::TypeId, AZStd::string>;
            TypeMap m_typeCache;
        };
    } // namespace Text
} // namespace EditorPythonBindings
