/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/Memory.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/optional.h>

#include <Source/PythonCommon.h>
#include <pybind11/pybind11.h>

namespace EditorPythonBindings
{
    //! Wraps an instance of a Behavior Class that is flagged for 'Editor'
    class PythonProxyObject final
    {
    public:
        AZ_TYPE_INFO(PythonProxyObject, "{448A4480-CCA8-4F14-9F17-41B0491F9FD1}");
        AZ_CLASS_ALLOCATOR(PythonProxyObject, AZ::SystemAllocator, 0);

        PythonProxyObject() = default;
        explicit PythonProxyObject(const AZ::TypeId& typeId);
        explicit PythonProxyObject(const char* typeName);
        explicit PythonProxyObject(const AZ::BehaviorObject& object);

        ~PythonProxyObject();

        //! Gets the AZ RTTI type of the BehaviorObject
        AZStd::optional<AZ::TypeId> GetWrappedType() const;

        //! Returns the wrapped behavior object pointer if it is valid
        AZStd::optional<AZ::BehaviorObject*> GetBehaviorObject();

        //! Gets the name of the type of the wrapped BehaviorObject
        const char* GetWrappedTypeName() const;
        
        //! Assigns a value to a property (by name) a value; the types must match
        void SetPropertyValue(const char* propertyName, pybind11::object value);
        
        //! Gets the value or callable held by a property of a wrapped BehaviorObject
        pybind11::object GetPropertyValue(const char* attributeName);

        //! Creates a default constructed instance a 'typeName'
        bool SetByTypeName(const char* typeName);

        //! Invokes a method by name on a wrapped BehaviorObject
        pybind11::object Invoke(const char* methodName, pybind11::args pythonArgs);

        //! Constructs a BehaviorClass using Python arguments 
        pybind11::object Construct(const AZ::BehaviorClass& behaviorClass, pybind11::args args);

        //! Performs an equality operation to compare this object with another object
        bool DoEqualityEvaluation(pybind11::object pythonOther);

        pybind11::object ToJson();

        //! Perform a comparison of a Python operator
        enum class Comparison
        {
            LessThan,
            LessThanOrEquals,
            GreaterThan,
            GreaterThanOrEquals
        };
        bool DoComparisonEvaluation(pybind11::object pythonOther, Comparison comparison);

        //! Gets the wrapped object's __repr__
        pybind11::object GetWrappedObjectRepr();

        //! Gets the wrapped object's __str__
        pybind11::object GetWrappedObjectStr();

    protected:
        void PrepareWrappedObject(const AZ::BehaviorClass& behaviorClass);
        void ReleaseWrappedObject();
        bool CreateDefault(const AZ::BehaviorClass* behaviorClass);
        void PopulateMethodsAndProperties(const AZ::BehaviorClass& behaviorClass);
        void PopulateComparisonOperators(const AZ::BehaviorClass& behaviorClass);
        bool CanConvertPythonToBehaviorValue(const AZ::BehaviorParameter& behaviorArg, pybind11::object pythonArg) const;
        

    private:
        enum class Ownership
        {
            None,
            Owned,
            Released
        };

        AZ::BehaviorObject m_wrappedObject;
        AZStd::string m_wrappedObjectTypeName;
        AZStd::string m_wrappedObjectCachedRepr;
        Ownership m_ownership = Ownership::None;
        AZStd::unordered_map<AZ::Crc32, AZ::BehaviorMethod*> m_methods;
        AZStd::unordered_map<AZ::Crc32, AZ::BehaviorProperty*> m_properties;
    };

    namespace PythonProxyObjectManagement
    {
        //! Creates the 'azlmbr.object' module so that Python script developers can manage proxy objects
        void CreateSubmodule(pybind11::module parentModule, pybind11::module defaultModule);

        //! Creates a Python object storing a BehaviorObject backed by a BehaviorClass
        pybind11::object CreatePythonProxyObject(const AZ::TypeId& typeId, void* data);

        //! Checks if function can be reflected as a class member method
        bool IsMemberLike(const AZ::BehaviorMethod& method, const AZ::TypeId& typeId);
    }
}

namespace pybind11 
{
    namespace detail 
    {
        //! Type caster specialization PythonProxyObject to convert between Python <-> AZ Reflection
        template <> 
        struct type_caster<EditorPythonBindings::PythonProxyObject> 
            : public type_caster_base<EditorPythonBindings::PythonProxyObject>
        {
        public:

            // Conversion (Python -> C++)
            bool load(handle src, bool convert)
            {
                return type_caster_base<EditorPythonBindings::PythonProxyObject>::load(src, convert);
            }

            // Conversion (C++ -> Python)
            static handle cast(const EditorPythonBindings::PythonProxyObject* src, return_value_policy policy, handle parent)
            {
                return type_caster_base<EditorPythonBindings::PythonProxyObject>::cast(src, policy, parent);
            }
        };
    }
}
