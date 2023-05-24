/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/std/string/string.h>
#include <AzCore/Outcome/Outcome.h>

#include <ScriptCanvas/Core/GraphScopedTypes.h>
#include <ScriptCanvas/Variable/GraphVariable.h>

namespace ScriptCanvas
{
    class VariableData;

    //! Bus Interface for adding, removing and finding exposed Variable datums associated with a ScriptCanvas Graph
    class VariableRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = GraphScopedVariableId;

        using AllocatorType = AZStd::allocator;

        virtual GraphVariable* GetVariable() = 0;
        virtual const GraphVariable* GetVariableConst() const = 0;

        //! Returns the type associated with the specified variable.
        virtual Data::Type GetType() const = 0;

        //! Looks up the variable name that the variable id is associated with in the handler of the bus
        virtual AZStd::string_view GetName() const = 0;


        //! Changes the name of the variable with the specified @variableId within the handler
        //! returns an AZ::Outcome to indicate if the variable was able to be succesfully or an error message to indicate
        //! why the rename failed
        virtual AZ::Outcome<void, AZStd::string> RenameVariable(AZStd::string_view newVarName) = 0;
    };

    using VariableRequestBus = AZ::EBus<VariableRequests>;

    class CopiedVariableData
    {
    public:

        AZ_RTTI(CopiedVariableData, "{84548415-DD9E-4943-8D1E-3E1CC49ADACB}");
        AZ_CLASS_ALLOCATOR(CopiedVariableData, AZ::SystemAllocator);

        virtual ~CopiedVariableData() = default;

        static const char* k_variableKey;
        GraphVariableMapping m_variableMapping;
    };

    enum class GraphVariableValidationErrorCode
    {
        Duplicate,
        Invalid,

        Unknown
    };

    using VariableValidationOutcome = AZ::Outcome<void, GraphVariableValidationErrorCode>;

    class GraphVariableManagerRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using MutexType = AZStd::recursive_mutex;
        using BusIdType = ScriptCanvasId;

        using AllocatorType = AZStd::allocator;

        //! Adds a variable that is keyed by the string and maps to a type that can be storedAZStd::any(any type with a AzTypeInfo specialization)
        //! returns an AZ::Outcome which on success contains the VariableId and on Failure contains a string with error information
        virtual AZ::Outcome<VariableId, AZStd::string> CloneVariable(const GraphVariable& baseVariable) = 0;
        virtual AZ::Outcome<VariableId, AZStd::string> RemapVariable(const GraphVariable& variableConfiguration) = 0;
        virtual AZ::Outcome<VariableId, AZStd::string> AddVariable(AZStd::string_view key, const Datum& value, bool functionScope) = 0;
        virtual AZ::Outcome<VariableId, AZStd::string> AddVariablePair(const AZStd::pair<AZStd::string_view, Datum>& keyValuePair) = 0;

        virtual VariableValidationOutcome IsNameValid(AZStd::string_view variableName) = 0;

        //! Adds properties from the range [first, last)
        //! returns vector of AZ::Outcome which for successful outcomes contains the VariableId and for failing outcome
        //! contains string detailing the reason for failing to add the variable
        template<typename InputIt>
        AZStd::vector<AZ::Outcome<VariableId, AZStd::string>> AddVariables(InputIt first, InputIt last)
        {
            static_assert(AZStd::is_same<typename AZStd::iterator_traits<InputIt>::value_type, AZStd::pair<AZStd::string_view, Datum>>::value, "Only iterators to pair<string_view, any> are supported");

            AZStd::vector<AZ::Outcome<VariableId, AZStd::string>> addVariableOutcomes;
            for (; first != last; ++first)
            {
                addVariableOutcomes.push_back(AddVariablePair(*first));
            }

            return addVariableOutcomes;
        }

        //! Remove a single variable which matches the specified variable id
        //! returns true if the variable with the variable id was removed
        virtual bool RemoveVariable(const VariableId&) = 0;
        //! Removes properties which matches the specified string name
        //! returns the number of properties removed
        virtual AZStd::size_t RemoveVariableByName(AZStd::string_view) = 0;
        //! Removes properties which matches the specified variable ids
        //! returns the number of properties removed
        template<typename InputIt>
        AZStd::size_t RemoveVariables(InputIt first, InputIt last)
        {
            AZStd::size_t removedVariableCount = 0U;
            static_assert(AZStd::is_same<typename AZStd::iterator_traits<InputIt>::value_type, VariableId>::value, "Only input iterators to VariableId are supported");
            for (; first != last; ++first)
            {
                removedVariableCount += RemoveVariable(*first) ? 1 : 0;
            }

            return removedVariableCount;
        }

        //! Searches for a variable with the specified name
        //! returns pointer to the first variable with the specified name or nullptr
        virtual GraphVariable* FindVariable(AZStd::string_view propName) = 0;        

        //! Searches for a variable by VariableId
        //! returns a pair of <variable datum pointer, variable name> with the supplied id
        //! The variable datum pointer is non-null if the variable has been found
        virtual GraphVariable* FindVariableById(const VariableId& varId) = 0;

        virtual GraphVariable* FindFirstVariableWithType(const Data::Type& dataType, const AZStd::unordered_set< ScriptCanvas::VariableId >& excludedVariableIds) = 0;

        //! Returns the type associated with the specified variable.
        virtual Data::Type GetVariableType(const VariableId& variableId) = 0;

        //! Retrieves all properties stored by the Handler
        //! returns variable container
        virtual const GraphVariableMapping* GetVariables() const = 0;

        //! Looks up the variable name that the variable data is associated with in the handler of the bus
        virtual AZStd::string_view GetVariableName(const VariableId&) const = 0;
        //! Changes the name of the variable with the specified @variableId within the handler
        //! returns an AZ::Outcome to indicate if the variable was able to be succesfully or an error message to indicate
        //! why the rename failed
        virtual AZ::Outcome<void, AZStd::string> RenameVariable(const VariableId& variableId, AZStd::string_view newVarName) = 0;

        virtual bool IsRemappedId(const VariableId& remappedId) const = 0;

        virtual const VariableData* GetVariableDataConst() const = 0;
        virtual VariableData* GetVariableData() = 0;

        //! Sets the VariableData and connects the variables ids to the VariableRequestBus for this handler
        virtual void SetVariableData(const VariableData& variableData) = 0;

        //! Deletes oldVariableData and sends out GraphVariableManagerNotifications for each deleted variable
        virtual void DeleteVariableData(const VariableData& variableData) = 0;

        // <Deprecated>
        bool IsNameAvailable(AZStd::string_view key)
        {
            return IsNameValid(key).IsSuccess();
        }
        // </Deprecated>
    };

    using GraphVariableManagerRequestBus = AZ::EBus<GraphVariableManagerRequests>;

    class VariableNodeRequests
    {
    public:
        // Sets the VariableId on a node that interfaces with a variable(i.e the GetVariable and SetVariable node)
        virtual void SetId(const VariableId& variableId) = 0;
        // Retrieves the VariableId on a node that interfaces with a variable(i.e the GetVariable and SetVariable node)
        virtual const VariableId& GetId() const = 0;
    };

    class ScriptEventNodeRequests
    {
    public:
        virtual void UpdateVersion() {}
    };

    struct RequestByNodeIdTraits : public AZ::EBusTraits
    {
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;
    };
    
    using VariableNodeRequestBus = AZ::EBus<VariableNodeRequests, RequestByNodeIdTraits>;
    using ScriptEventNodeRequestBus = AZ::EBus<ScriptEventNodeRequests, RequestByNodeIdTraits>;


    class GraphVariableManagerNotifications
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = ScriptCanvasId;

        // Invoked when after a variable has been added to the handler
        virtual void OnVariableAddedToGraph(const ScriptCanvas::VariableId& /*variableId*/, AZStd::string_view /*variableName*/) {}
        // Invoked after a variable has been removed from the handler
        virtual void OnVariableRemovedFromGraph(const ScriptCanvas::VariableId& /*variableId*/, AZStd::string_view /*variableName*/) {}
        // Invoked after a variable has been renamed
        virtual void OnVariableNameChangedInGraph(const ScriptCanvas::VariableId& /*variableId*/, AZStd::string_view /*variableName*/) {}
        // Invoked after the variable data has been set on the variable handler
        virtual void OnVariableDataSet() {}
    };

    using GraphVariableManagerNotificationBus = AZ::EBus<GraphVariableManagerNotifications>;

    class VariableNotifications
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = GraphScopedVariableId;

        // Invoked before a variable is erased from the Variable Bus Handler
        virtual void OnVariableRemoved() {}

        // Invoked after a variable is renamed
        virtual void OnVariableRenamed(AZStd::string_view /*newVariableName*/) {}

        virtual void OnVariableScopeChanged() {}

        virtual void OnVariableInitialValueSourceChanged() {}

        virtual void OnVariablePriorityChanged() {}

        virtual void OnVariableValueChanged() {}
    };

    using VariableNotificationBus = AZ::EBus<VariableNotifications>;    

    class VariableNodeNotifications
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;
        // Invoked after the variable id has been changed on the SetVariable/GetVariableNode
        virtual void OnVariableIdChanged(const VariableId& /*oldVariableId*/, const VariableId& /*newVariableId*/) {}
        // Invoked after the variable has been removed from the GraphVariableManagerRequestBus
        virtual void OnVariableRemovedFromNode(const VariableId& /*removedVariableId*/) {}
    };

    using VariableNodeNotificationBus = AZ::EBus<VariableNodeNotifications>;
}
