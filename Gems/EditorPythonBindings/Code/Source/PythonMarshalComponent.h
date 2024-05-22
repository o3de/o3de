/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <EditorPythonBindings/EditorPythonBindingsSymbols.h>

#include <EditorPythonBindings/PythonCommon.h>
#include <EditorPythonBindings/PythonUtility.h>
#include <pybind11/pybind11.h>

#include <AzCore/Component/Component.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/RTTI/AttributeReader.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/optional.h>
#include <AzCore/std/string/string_view.h>

namespace EditorPythonBindings
{
    //! An abstract to marshal between Behavior and Python type values
    class PythonMarshalTypeRequests
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::TypeId;
        //////////////////////////////////////////////////////////////////////////

        using DeallocateFunction = Convert::VariableDeleter;
        using BehaviorTraits = AZ::BehaviorParameter::Traits;

        //! Marshals a Python value to a BehaviorArgument plus an optional function to deallocate it after usage
        //! @return returns a pair of a flag to indicate success and an function to deallocate
        using BehaviorValueResult = AZStd::pair<bool, DeallocateFunction>;
        virtual AZStd::optional<BehaviorValueResult> PythonToBehaviorValueParameter(BehaviorTraits traits, pybind11::object pyObj, AZ::BehaviorArgument& outValue) = 0;

        //! Marshals a BehaviorArgument to a Python value object
        //! @return returns a pair of a valid Python object and an optional function to deallocate after sent to Python
        using PythonValueResult = AZStd::pair<pybind11::object, DeallocateFunction>;
        virtual AZStd::optional<PythonValueResult> BehaviorValueParameterToPython(AZ::BehaviorArgument& behaviorValue) = 0;

        //! Validates that a particular Python object can convert into a Behavior value parameter type
        virtual bool CanConvertPythonToBehaviorValue(BehaviorTraits traits, pybind11::object pyObj) const = 0;
    };
    using PythonMarshalTypeRequestBus = AZ::EBus<PythonMarshalTypeRequests>;

    //! Handles marshaling of built-in Behavior types like numbers, strings, and lists
    class PythonMarshalComponent
        : public AZ::Component
        , protected PythonMarshalTypeRequestBus::MultiHandler
    {
    public:
        AZ_COMPONENT(PythonMarshalComponent, PythonMarshalComponentTypeId, AZ::Component);

        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

        class TypeConverter
        {
        public:
            virtual AZStd::optional<PythonMarshalComponent::BehaviorValueResult> PythonToBehaviorValueParameter(PythonMarshalTypeRequests::BehaviorTraits traits, pybind11::object pyObj, AZ::BehaviorArgument& outValue) = 0;
            virtual AZStd::optional<PythonMarshalComponent::PythonValueResult> BehaviorValueParameterToPython(AZ::BehaviorArgument& behaviorValue) = 0;
            virtual bool CanConvertPythonToBehaviorValue(BehaviorTraits traits, pybind11::object pyObj) const = 0;
            virtual ~TypeConverter() = default;
        };
        using TypeConverterPointer = AZStd::shared_ptr<TypeConverter>;

        void RegisterTypeConverter(const AZ::TypeId& typeId, TypeConverterPointer typeConverterPointer);

    protected:
        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;

        ////////////////////////////////////////////////////////////////////////
        // PythonMarshalTypeRequestBus interface implementation
        AZStd::optional<PythonMarshalTypeRequests::BehaviorValueResult> PythonToBehaviorValueParameter(PythonMarshalTypeRequests::BehaviorTraits traits, pybind11::object pyObj, AZ::BehaviorArgument& outValue) override;
        AZStd::optional<PythonMarshalTypeRequests::PythonValueResult> BehaviorValueParameterToPython(AZ::BehaviorArgument& behaviorValue) override;
        bool CanConvertPythonToBehaviorValue(BehaviorTraits traits, pybind11::object pyObj) const override;

    private:
        using TypeConverterMap = AZStd::unordered_map<AZ::TypeId, TypeConverterPointer>;
        TypeConverterMap m_typeConverterMap;
    };

    namespace Container
    {
        AZStd::optional<PythonMarshalTypeRequests::PythonValueResult> ProcessBehaviorObject(AZ::BehaviorObject& behaviorObject);
        AZStd::optional<PythonMarshalTypeRequests::BehaviorValueResult> ProcessPythonObject(
            PythonMarshalTypeRequests::BehaviorTraits traits,
            pybind11::object pythonObj,
            const AZ::TypeId& elementTypeId,
            AZ::BehaviorArgument& outValue);

    } // namespace Container
}
