/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ScriptCanvas/Asset/RuntimeAsset.h>
#include <ScriptCanvas/Core/Nodeable.h>
#include <ScriptCanvas/Execution/ExecutionStateStorage.h>
#include <ScriptCanvas/Execution/Interpreted/ExecutionInterpretedAPI.h>
#include <ScriptCanvas/Execution/Interpreted/ExecutionStateInterpreted.h>
#include <ScriptCanvas/Execution/Interpreted/ExecutionStateInterpretedPerActivation.h>
#include <ScriptCanvas/Execution/Interpreted/ExecutionStateInterpretedPure.h>
#include <ScriptCanvas/Execution/Interpreted/ExecutionStateInterpretedSingleton.h>

#include <ScriptCanvas/Execution/ExecutionContext.h>

namespace ExecutionContextCpp
{
    void CopyTypeInformationOnly(AZ::BehaviorValueParameter& lhs, const AZ::BehaviorValueParameter& rhs)
    {
        lhs.m_typeId = rhs.m_typeId;
        lhs.m_azRtti = rhs.m_azRtti;
    }

    void CopyTypeAndValueSource(AZ::BehaviorValueParameter& lhs, const AZ::BehaviorValueParameter& rhs)
    {
        lhs.m_typeId = rhs.m_typeId;
        lhs.m_azRtti = rhs.m_azRtti;
        lhs.m_value = rhs.m_value;
    }
}

namespace ScriptCanvas
{
    namespace Execution
    {
        ActivationData::ActivationData(const RuntimeDataOverrides& variableOverrides, ActivationInputArray& storage)
            : variableOverrides(variableOverrides)
            , runtimeData(variableOverrides.m_runtimeAsset->m_runtimeData)
            , storage(storage)
        {}

        const void* ActivationData::GetVariableSource(size_t index, size_t& overrideIndexTracker) const
        {
            if (variableOverrides.m_variableIndices[index])
            {
                return AZStd::any_cast<void>(&variableOverrides.m_variables[overrideIndexTracker++].value);
            }
            else
            {
                return runtimeData.m_input.m_variables[index].second.GetAsDanger();
            }
        }

        ActivationInputRange Context::CreateActivateInputRange(ActivationData& activationData)
        {
            const RuntimeData& runtimeData = activationData.runtimeData;
            ActivationInputRange rangeOut = runtimeData.m_activationInputRange;
            rangeOut.inputs = activationData.storage.begin();

            AZ_Assert(rangeOut.totalCount <= activationData.storage.size(), "Too many initial arguments for activation. "
                "Consider increasing size, source of ActivationInputArray, or breaking up the source graph");

            // nodeables - until the optimization is required, every instance gets their own copy
            {
                auto sourceVariableIter = runtimeData.m_activationInputRange.inputs;
                const auto sourceVariableSentinel = runtimeData.m_activationInputRange.inputs + runtimeData.m_activationInputRange.nodeableCount;
                auto destVariableIter = rangeOut.inputs;
                for (; sourceVariableIter != sourceVariableSentinel; ++sourceVariableIter, ++destVariableIter)
                {
                    ExecutionContextCpp::CopyTypeAndValueSource(*destVariableIter, *sourceVariableIter);
                }
            }

            // (possibly overridden) variables, only the overrides are saved in on the component, otherwise they are taken from the runtime asset
            {
                auto sourceVariableIter = runtimeData.m_activationInputRange.inputs + runtimeData.m_activationInputRange.nodeableCount;
                auto destVariableIter = rangeOut.inputs + runtimeData.m_activationInputRange.nodeableCount;

                size_t overrideIndexTracker = 0;
                const size_t sentinel = runtimeData.m_input.m_variables.size();
                for (size_t index = 0; index != sentinel; ++index, ++destVariableIter, ++sourceVariableIter)
                {
                    ExecutionContextCpp::CopyTypeInformationOnly(*destVariableIter, *sourceVariableIter);
                    destVariableIter->m_value = const_cast<void*>(activationData.GetVariableSource(index, overrideIndexTracker));
                }
            }

            // (always overridden) EntityIds
            {
                AZ::BehaviorValueParameter* destVariableIter = rangeOut.inputs
                    + runtimeData.m_activationInputRange.nodeableCount
                    + runtimeData.m_activationInputRange.variableCount;

                const auto entityIdTypeId = azrtti_typeid<Data::EntityIDType>();

                for (auto& entityId : activationData.variableOverrides.m_entityIds)
                {
                    destVariableIter->m_typeId = entityIdTypeId;
                    destVariableIter->m_value = destVariableIter->m_tempData.allocate(sizeof(Data::EntityIDType), AZStd::alignment_of<Data::EntityIDType>::value, 0);
                    auto entityIdValuePtr = reinterpret_cast<AZStd::decay_t<Data::EntityIDType>*>(destVariableIter->m_value);
                    *entityIdValuePtr = entityId;
                    ++destVariableIter;
                }
            }

            return rangeOut;
        }

        void Context::InitializeStaticActivationData(RuntimeData& runtimeData)
        {
            AZ::BehaviorContext* behaviorContext(nullptr);
            AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationRequests::GetBehaviorContext);
            if (!behaviorContext)
            {
                AZ_Error("Behavior Context", false, "A behavior context is required!");
                return;
            }

            // \todo, the stack push functions could be retrieved here
            InitializeStaticCreationFunction(runtimeData);
            InitializeStaticActivationInputs(runtimeData, *behaviorContext);
            InitializeStaticCloners(runtimeData, *behaviorContext);
        }

        void Context::InitializeStaticActivationInputs(RuntimeData& runtimeData, AZ::BehaviorContext& behaviorContext)
        {
            AZStd::vector<AZ::BehaviorValueParameter>& parameters = runtimeData.m_activationInputStorage;
            auto& range = runtimeData.m_activationInputRange;
            range.requiresDependencyConstructionParameters = runtimeData.RequiresDependencyConstructionParameters();
            parameters.reserve(runtimeData.m_input.GetConstructorParameterCount());

            for (const Nodeable* nodeable : runtimeData.m_input.m_nodeables)
            {
                AZ::BehaviorValueParameter bvp;
                bvp.m_typeId = azrtti_typeid(nodeable);

                const auto classIter(behaviorContext.m_typeToClassMap.find(bvp.m_typeId));
                AZ_Assert(classIter != behaviorContext.m_typeToClassMap.end(), "No class by typeID of %s in the behavior context!", bvp.m_typeId.ToString<AZStd::string>().c_str());
                bvp.m_azRtti = classIter->second->m_azRtti;
                bvp.m_value = const_cast<Nodeable*>(nodeable);
                parameters.push_back(bvp);
            }

            for (auto& idDatumPair : runtimeData.m_input.m_variables)
            {
                const Datum* datum = &idDatumPair.second;
                AZ::BehaviorValueParameter bvp;
                bvp.m_typeId = datum->GetType().GetAZType();
                const auto classIter(behaviorContext.m_typeToClassMap.find(bvp.m_typeId));
                bvp.m_azRtti = classIter != behaviorContext.m_typeToClassMap.end() ? classIter->second->m_azRtti : nullptr;
                bvp.m_value = const_cast<void*>(datum->GetAsDanger());
                parameters.push_back(bvp);
            }

            const size_t entityIdSize = runtimeData.m_input.m_entityIds.size();
            for (size_t i = 0; i < entityIdSize; ++i)
            {
                AZ::BehaviorValueParameter bvp;
                bvp.m_typeId = azrtti_typeid<Data::EntityIDType>();
                parameters.push_back(bvp);
            }

            range.inputs = parameters.begin();
            range.nodeableCount = runtimeData.m_input.m_nodeables.size();
            range.variableCount = runtimeData.m_input.m_variables.size();
            range.entityIdCount = runtimeData.m_input.m_entityIds.size();
            range.totalCount = range.nodeableCount + range.variableCount + range.entityIdCount;
        }

        // This does not have to recursively initialize dependent assets, as this is called by asset handler
        void Context::InitializeStaticCloners(RuntimeData& runtimeData, AZ::BehaviorContext& behaviorContext)
        {
            runtimeData.m_cloneSources.reserve(runtimeData.m_input.m_staticVariables.size());

            for (AZStd::pair<VariableId, AZStd::any>& staticSource : runtimeData.m_input.m_staticVariables)
            {
                AZStd::any& anySource = staticSource.second;
                auto bcClass = AZ::BehaviorContextHelper::GetClass(&behaviorContext, anySource.type());
                AZ_Assert(bcClass, "BehaviorContext class for type %s was deleted", anySource.type().ToString<AZStd::string>().c_str());
                runtimeData.m_cloneSources.emplace_back(*bcClass, AZStd::any_cast<void>(&anySource));
            }
        }

        void Context::InitializeStaticCreationFunction(RuntimeData& runtimeData)
        {
            const Grammar::ExecutionStateSelection selection = runtimeData.m_input.m_executionSelection;

            switch (selection)
            {
            case Grammar::ExecutionStateSelection::InterpretedPure:
                runtimeData.m_createExecution = &Execution::CreatePure;
                break;

            case Grammar::ExecutionStateSelection::InterpretedPureOnGraphStart:
                runtimeData.m_createExecution = &Execution::CreatePureOnGraphStart;
                break;

            case Grammar::ExecutionStateSelection::InterpretedObject:
                runtimeData.m_createExecution = &Execution::CreatePerActivation;
                break;

            case Grammar::ExecutionStateSelection::InterpretedObjectOnGraphStart:
                runtimeData.m_createExecution = &Execution::CreatePerActivationOnGraphStart;
                break;

            default:
                SC_RUNTIME_CHECK(false, "Unsupported ScriptCanvas execution selection");
                runtimeData.m_createExecution =
                    [](Execution::StateStorage&, ExecutionStateConfig&)->ExecutionState* { return nullptr; };
                break;
            }
        }

        TypeErasedReference::TypeErasedReference(void* address, const AZ::TypeId& type)
            : m_address(address)
            , m_type(type)
        {
            SC_RUNTIME_CHECK(address, "Null address is not allowed in type erased Reference object");
        }

        void* TypeErasedReference::Address() const
        {
            return m_address;
        }

        const AZ::TypeId& TypeErasedReference::Type() const
        {
            return m_type;
        }
    }
}
