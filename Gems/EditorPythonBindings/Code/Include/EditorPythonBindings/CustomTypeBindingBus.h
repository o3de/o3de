/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/RTTI/BehaviorContext.h>

 // forward declare the C typedef of a PyObject*
struct _object;
using PyObject = _object;

namespace EditorPythonBindings
{
    //! A team can define custom generic types to be created for a TypeId
    //! The handler will need to allocate, deallocate, and convert behavior data to Python values
    //! NOTE: if the TypeId is registered with the Behavior Context then that will be used instead of this custom binding
    class CustomTypeBindingNotifications
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::TypeId;
        //////////////////////////////////////////////////////////////////////////

        //! This handle is used to identify the allocations an external module used to prepare the value
        //! After the EPB gem is done with the conversion, this handle will be sent back via the CleanUpValue() notification
        //! to indicate that the module should clean up the allocations for that value conversion
        using ValueHandle = std::intptr_t;

        //! Allocate a default value for the supplied type
        using AllocationHandle = AZStd::optional<AZStd::pair<ValueHandle, AZ::BehaviorObject>>;
        virtual AllocationHandle AllocateDefault() = 0;

        //! This method converts an incoming Python value into a behavior value; it should fill out the outValue fields
        virtual AZStd::optional<ValueHandle> PythonToBehavior(
            PyObject* pyObj,
            AZ::BehaviorParameter::Traits traits,
            AZ::BehaviorValueParameter& outValue) = 0;

        //! This method convert an incoming behavior value into a Python value; it should fill out the outPyObj pointer
        virtual AZStd::optional<ValueHandle> BehaviorToPython(
            const AZ::BehaviorValueParameter& behaviorValue,
            PyObject*& outPyObj) = 0;

        //! This method is used to determine that the behavior value can be processed using the Python object type as input
        //! NOTE: it should not actually do the conversion only detect IF it can be done with the supplied Python type
        virtual bool CanConvertPythonToBehavior(
            AZ::BehaviorParameter::Traits traits,
            PyObject* pyObj) const = 0;

        //! This is used to deallocate the value used by the PythonToBehavior() or BehaviorToPython() methods
        //! The notification module is responsible for mapping the handle to the value's allocation(s)
        virtual void CleanUpValue(ValueHandle handle) = 0;
    };
    using CustomTypeBindingNotificationBus = AZ::EBus<CustomTypeBindingNotifications>;
}
