/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "BehaviorContextUtils.h"

#include <AzCore/std/containers/map.h>
#include <AzCore/RTTI/BehaviorContextUtilities.h>
#include <ScriptCanvas/Core/GraphData.h>
#include <ScriptCanvas/Libraries/Core/Method.h>

namespace ScriptCanvas
{
    void* BehaviorContextUtils::ConstructTuple(const AZ::TypeId& typeID)
    {
        return ConstructTupleGetContext(typeID).first;
    }

    AZStd::pair<void*, AZ::BehaviorContext*> BehaviorContextUtils::ConstructTupleGetContext(const AZ::TypeId& typeID)
    {
        using namespace AZ::ScriptCanvasAttributes;
        AZStd::pair<const AZ::BehaviorClass*, AZ::BehaviorContext*> classAndContext = AZ::BehaviorContextHelper::GetClassAndContext(typeID);

        if (const AZ::BehaviorClass* bcClass = classAndContext.first)
        {
            if (AZ::Attribute* attribute = AZ::FindAttribute(TupleConstructorFunction, bcClass->m_attributes))
            {
                TupleConstructorHolder holder;
                AZ::AttributeReader attributeReader(nullptr, attribute);
                attributeReader.Read<TupleConstructorHolder>(holder);
                return { AZStd::invoke(holder.m_function), classAndContext.second };
            }
        }

        return { nullptr, nullptr };
    }

    AZStd::vector<AZ::TypeId> BehaviorContextUtils::GetUnpackedTypes(const AZ::TypeId& typeID)
    {
        using namespace AZ::ScriptCanvasAttributes;

        if (const AZ::BehaviorClass* bcClass = AZ::BehaviorContextHelper::GetClass(typeID))
        {
            if (AZ::Attribute* attribute = AZ::FindAttribute(ReturnValueTypesFunction, bcClass->m_attributes))
            {
                GetUnpackedReturnValueTypesHolder holder;
                AZ::AttributeReader attributeReader(nullptr, attribute);
                attributeReader.Read<GetUnpackedReturnValueTypesHolder>(holder);
                return AZStd::invoke(holder.m_function);
            }
        }

        return { typeID };
    }

    bool BehaviorContextUtils::FindClass(const AZ::BehaviorMethod*& outMethod, const AZ::BehaviorClass*& outClass, [[maybe_unused]] AZStd::string_view className, [[maybe_unused]] AZStd::string_view methodName, PropertyStatus propertyStatus, [[maybe_unused]] AZStd::string* outPrettyClassName, [[maybe_unused]] bool warnOnMissing)
    {
        AZ::BehaviorContext* behaviorContext(nullptr);
        AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationRequests::GetBehaviorContext);
        if (!behaviorContext)
        {
            AZ_Warning("Script Canvas", false, "BehaviorContext is required!");
            return false;
        }

        auto classIter(behaviorContext->m_classes.find(className));
        if (classIter == behaviorContext->m_classes.end())
        {
            AZ_Warning("Script Canvas", !warnOnMissing, "No class by name of %s found in the BehaviorContext!", className.data());
            return false;
        }

        const AZ::BehaviorClass* behaviorClass(classIter->second);
        AZ_Assert(behaviorClass, "BehaviorContext Class entry %s has no class pointer", className.data());


        AZ::BehaviorMethod* method{};

        if (propertyStatus == PropertyStatus::None)
        {
            const auto methodIter(behaviorClass->m_methods.find(methodName.data()));
            if (methodIter != behaviorClass->m_methods.end())
            {
                method = methodIter->second;
                propertyStatus = PropertyStatus::None;
            }
            else
            {
                AZ_Warning("Script Canvas", !warnOnMissing, "No method by name of %s found in BehaviorContext class %s", methodName.data(), className.data());
            }
        }
        else
        {
            const auto propertyIter(behaviorClass->m_properties.find(methodName.data()));
            if (propertyIter == behaviorClass->m_properties.end())
            {
                AZ_Warning("Script Canvas", !warnOnMissing, "No property by name of %s found in BehaviorContext class %s", methodName.data(), className.data());
                return false;
            }

            method = propertyStatus == PropertyStatus::Getter ? propertyIter->second->m_getter : propertyIter->second->m_setter;
        }

        // this argument is the first argument...so perhaps remove the distinction between class and member functions, since it probably won't follow polymorphism
        // if it will, keep the distinction, and add the first argument separately
        if (!method)
        {
            AZ_Warning("Script Canvas", !warnOnMissing, "BehaviorContext Method entry %s has no method pointer", methodName.data());
            return false;
        }

        if (outPrettyClassName)
        {
            *outPrettyClassName = className;

            if (AZ::Attribute* prettyNameAttribute = AZ::FindAttribute(AZ::ScriptCanvasAttributes::PrettyName, behaviorClass->m_attributes))
            {
                AZ::AttributeReader(nullptr, prettyNameAttribute).Read<AZStd::string>(*outPrettyClassName, *behaviorContext);
            }
        }

        outClass = behaviorClass;
        outMethod = method;
        return true;
    }

    bool BehaviorContextUtils::FindEBus(const AZ::BehaviorEBus*& outEBus, [[maybe_unused]] AZStd::string_view ebusName, [[maybe_unused]] bool warnOnMissing)
    {
        AZ::BehaviorContext* behaviorContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationRequests::GetBehaviorContext);
        if (!behaviorContext)
        {
            AZ_Warning("Script Canvas", false, "BehaviorContext is required!");
            return false;
        }

        const auto& ebusIterator = behaviorContext->m_ebuses.find(ebusName);
        if (ebusIterator == behaviorContext->m_ebuses.end())
        {
            AZ_Warning("Script Canvas", !warnOnMissing, "No ebus by name of %s found in the BehaviorContext!", ebusName.data());
            return false;
        }

        outEBus = ebusIterator->second;
        AZ_Assert(outEBus, "ebus == nullptr in %s", ebusName.data());
        return true;
    }

    bool BehaviorContextUtils::FindExplicitOverload(const AZ::BehaviorMethod*& outMethod, const AZ::BehaviorClass*& outClass, AZStd::string_view /*className*/, AZStd::string_view methodName, AZStd::string* /*outPrettyClassName*/)
    {
        AZ::BehaviorContext* behaviorContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationRequests::GetBehaviorContext);
        AZ_Assert(behaviorContext, "Behavior Context is required");
        if (behaviorContext)
        {
            AZ::ExplicitOverloadInfo explicitOverloadInfo;
            explicitOverloadInfo.m_name = methodName;

            auto iter = behaviorContext->m_explicitOverloads.find(explicitOverloadInfo);
            if (iter != behaviorContext->m_explicitOverloads.end())
            {
                outMethod = iter->m_overloads[0].first;
                outClass = iter->m_overloads[0].second;
                return true;
            }
        }

        return false;
    }

    AZStd::string BehaviorContextUtils::FindExposedMethodName(const AZ::BehaviorMethod& method, const AZ::BehaviorClass* behaviorClass)
    {
        if (behaviorClass)
        {
            for (auto candidate : behaviorClass->m_methods)
            {
                if (&method == candidate.second)
                {
                    return candidate.first;
                }
 
                for (auto overload = candidate.second->m_overload; overload != nullptr; overload = overload->m_overload)
                {
                    if (&method == overload)
                    {
                        return candidate.first;
                    }
                }
            }
        }
        else
        {
            // BehaviorClass is null, lookup in global methods
            AZ::BehaviorContext* behaviorContext(nullptr);
            AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationRequests::GetBehaviorContext);
            if (behaviorContext)
            {
                for (auto candidate : behaviorContext->m_methods)
                {
                    if (&method == candidate.second)
                    {
                        return candidate.first;
                    }
                }
            }
        }

        return method.m_name;
    }

    bool BehaviorContextUtils::FindEvent(const AZ::BehaviorMethod*& outMethod, AZStd::string_view ebusName, AZStd::string_view eventName, EventType* outEventType, [[maybe_unused]] bool warnOnMissing)
    {
        const AZ::BehaviorEBus* ebus{};
        if (!FindEBus(ebus, ebusName, warnOnMissing))
        {
            return false;
        }

        return FindEvent(outMethod, ebus, eventName, outEventType, warnOnMissing);
    }

    bool BehaviorContextUtils::FindEvent(const AZ::BehaviorMethod*& outMethod, const AZ::BehaviorEBus* const ebus, AZStd::string_view eventName, EventType* outEventType, [[maybe_unused]] bool warnOnMissing)
    {
        if (!ebus)
        {
            AZ_Warning("Script Canvas", !warnOnMissing, "event by name of %s found has no ebus to search in", eventName.data());
            return false;
        }

        const auto& sender = ebus->m_events.find(eventName);
        if (sender == ebus->m_events.end())
        {
            AZ_Warning("Script Canvas", !warnOnMissing, "No event by name of %s found in the ebus %s", eventName.data(), ebus->m_name.c_str());
            return false;
        }

        AZ::BehaviorMethod* method = GetEventMethod(*ebus, sender->second);
        if (!method)
        {
            AZ_Warning("Script Canvas", !warnOnMissing, "Queue function mismatch in %s-%s", eventName.data(), ebus->m_name.c_str());
            return false;
        }

        if (outEventType)
        {
            *outEventType = GetEventType(*ebus);
        }

        outMethod = method;
        return true;
    }

    bool BehaviorContextUtils::FindFree(const AZ::BehaviorMethod*& outMethod, AZStd::string_view methodName, [[maybe_unused]] bool warnOnMissing)
    {
        AZ::BehaviorContext* behaviorContext(nullptr);
        AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationRequests::GetBehaviorContext);
        if (!behaviorContext)
        {
            AZ_Warning("Script Canvas", false, "BehaviorContext is required!");
            return false;
        }

        AZ::BehaviorMethod* method{};
        const auto methodIter(behaviorContext->m_methods.find(methodName));
        if (methodIter != behaviorContext->m_methods.end())
        {
            method = methodIter->second;
            if (!method)
            {
                AZ_Warning("Script Canvas", !warnOnMissing, "BehaviorContext Method entry %.*s has no method pointer",
                    aznumeric_cast<int>(methodName.size()), methodName.data());
                return false;
            }
        }
        else
        {
            AZStd::string propertyName(methodName);
            AZ::RemovePropertyNameArtifacts(propertyName);

            auto iter = behaviorContext->m_properties.find(propertyName);
            if (iter != behaviorContext->m_properties.end())
            {
                method = iter->second->m_getter ? iter->second->m_getter : iter->second->m_setter;
            }

            if (!method)
            {
                AZ_Warning("Script Canvas", !warnOnMissing, "No method by name of %.*s found in the BehaviorContext!",
                    aznumeric_cast<int>(methodName.size()), methodName.data());
                return false;
            }
        }

        outMethod = method;
        return true;
    }

    size_t BehaviorContextUtils::GenerateFingerprintForBehaviorContext()
    {
        size_t fingerprint = 0;

        AZ::BehaviorContext* behaviorContext(nullptr);
        AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationRequests::GetBehaviorContext);
        if (!behaviorContext)
        {
            AZ_Warning("Script Canvas", false, "BehaviorContext is required!");
            return fingerprint;
        }

        if (behaviorContext)
        {
            HashCombineMethods(fingerprint, &(behaviorContext->m_methods));
            HashCombineProperties(fingerprint, &(behaviorContext->m_properties));
            HashCombineClasses(fingerprint, &(behaviorContext->m_classes));
            HashCombineEBuses(fingerprint, &(behaviorContext->m_ebuses));
        }

        // Include the base node version in the hash, so when it changes, script canvas jobs are reprocessed.
        AZStd::hash_combine(fingerprint, Node::GetNodeVersion());

        return fingerprint;
    }

    size_t BehaviorContextUtils::GenerateFingerprintForMethod(const MethodType& methodType, const AZStd::string& className, const AZStd::string& methodName)
    {
        size_t hash = 0;

        const AZ::BehaviorMethod* dependentMethod;
        switch (methodType)
        {
        case MethodType::Event:
            if (methodName.empty())
            {
                const AZ::BehaviorEBus* dependentEBus;
                if (ScriptCanvas::BehaviorContextUtils::FindEBus(dependentEBus, className))
                {
                    ScriptCanvas::BehaviorContextUtils::HashCombineEvents(hash, dependentEBus);
                }
            }
            else
            {
                if (ScriptCanvas::BehaviorContextUtils::FindEvent(dependentMethod, className, methodName))
                {
                    ScriptCanvas::BehaviorContextUtils::HashCombineMethodSignature(hash, dependentMethod);
                }
            }
            break;
        case MethodType::Free:
            if (ScriptCanvas::BehaviorContextUtils::FindFree(dependentMethod, methodName))
            {
                ScriptCanvas::BehaviorContextUtils::HashCombineMethodSignature(hash, dependentMethod);
            }
            break;
        case MethodType::Member:
            const AZ::BehaviorClass* dependentClass;
            if (ScriptCanvas::BehaviorContextUtils::FindClass(dependentMethod, dependentClass, className, methodName))
            {
                ScriptCanvas::BehaviorContextUtils::HashCombineMethodSignature(hash, dependentMethod);
            }
            break;
        }

        return hash;
    }

    AZStd::pair<const AZ::BehaviorMethod*, const AZ::BehaviorClass*> BehaviorContextUtils::GetCheck(const AZ::BehaviorMethod& method)
    {
        AZ::BehaviorContext* behaviorContext(nullptr);
        AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationRequests::GetBehaviorContext);
        if (!behaviorContext)
        {
            AZ_Warning("Script Canvas", false, "BehaviorContext is required!");
            return {};
        }

        auto iter = behaviorContext->m_checksByOperations.find(&method);
        if (iter != behaviorContext->m_checksByOperations.end())
        {
            return iter->second;
        }
        else
        {
            return {};
        }
    }

    AZ::EBusAddressPolicy BehaviorContextUtils::GetEBusAddressPolicy(const AZ::BehaviorEBus& ebus)
    {
        return GetEBusAddressPolicyByUuid(ebus.m_idParam.m_typeId);
    }

    AZ::EBusAddressPolicy BehaviorContextUtils::GetEBusAddressPolicyByUuid(AZ::Uuid addressTypeUuid)
    {
        return (addressTypeUuid.IsNull() || addressTypeUuid == AZ::AzTypeInfo<void>::Uuid())
            ? AZ::EBusAddressPolicy::Single : AZ::EBusAddressPolicy::ById;
    }

    AZ::BehaviorMethod* BehaviorContextUtils::GetEventMethod(const AZ::BehaviorEBus& ebus, const AZ::BehaviorEBusEventSender& ebusEventSender)
    {
        const auto addressPolicy = GetEBusAddressPolicy(ebus);
        return ebus.m_queueFunction
            ? (addressPolicy == AZ::EBusAddressPolicy::ById ? ebusEventSender.m_queueEvent : ebusEventSender.m_queueBroadcast)
            : (addressPolicy == AZ::EBusAddressPolicy::ById ? ebusEventSender.m_event : ebusEventSender.m_broadcast);
    }

    EventType BehaviorContextUtils::GetEventType(const AZ::BehaviorEBus& ebus)
    {
        const auto addressPolicy = GetEBusAddressPolicy(ebus);
        return ebus.m_queueFunction
            ? (addressPolicy == AZ::EBusAddressPolicy::ById ? EventType::EventQueue : EventType::BroadcastQueue)
            : (addressPolicy == AZ::EBusAddressPolicy::ById ? EventType::Event : EventType::Broadcast);
    }

    void BehaviorContextUtils::HashCombineClasses(size_t& outHash, const AZStd::unordered_map<AZStd::string, AZ::BehaviorClass*>* const unsortedClasses)
    {
        if (!unsortedClasses)
        {
            return;
        }

        const AZStd::map<AZStd::string, AZ::BehaviorClass*> sortedClasses(unsortedClasses->begin(), unsortedClasses->end());
        for (auto classIter = sortedClasses.begin(); classIter != sortedClasses.end(); ++classIter)
        {
            if (!classIter->second)
            {
                continue;
            }

            AZStd::hash_combine(outHash, classIter->second->m_name);
            AZStd::hash_combine(outHash, classIter->second->m_typeId);
            HashCombineProperties(outHash, &(classIter->second->m_properties));
            HashCombineMethods(outHash, &(classIter->second->m_methods));
        }
    }

    void BehaviorContextUtils::HashCombineEBuses(size_t& outHash, const AZStd::unordered_map<AZStd::string, AZ::BehaviorEBus*>* const unsortedEBuses)
    {
        if (!unsortedEBuses)
        {
            return;
        }

        const AZStd::map<AZStd::string, AZ::BehaviorEBus*> sortedEbus(unsortedEBuses->begin(), unsortedEBuses->end());
        for (auto ebusIter = sortedEbus.begin(); ebusIter != sortedEbus.end(); ++ebusIter)
        {
            if (!ebusIter->second)
            {
                continue;
            }

            AZStd::hash_combine(outHash, ebusIter->second->m_name);
            HashCombineEvents(outHash, ebusIter->second);
        }
    }

    void BehaviorContextUtils::HashCombineEvents(size_t& outHash, const AZ::BehaviorEBus* const ebus)
    {
        if (!ebus)
        {
            return;
        }

        const AZStd::map<AZStd::string, AZ::BehaviorEBusEventSender> sortedEvents(ebus->m_events.begin(), ebus->m_events.end());
        for (auto eventIter = sortedEvents.begin(); eventIter != sortedEvents.end(); ++eventIter)
        {
            AZ::BehaviorMethod* method = GetEventMethod(*ebus, eventIter->second);

            AZStd::hash_combine(outHash, eventIter->first);
            HashCombineMethodSignature(outHash, method);
        }
    }

    void BehaviorContextUtils::HashCombineMethods(size_t& outHash, const AZStd::unordered_map<AZStd::string, AZ::BehaviorMethod*>* const unsortedMethods)
    {
        if (!unsortedMethods)
        {
            return;
        }

        const AZStd::map<AZStd::string, AZ::BehaviorMethod*> sortedMethods(unsortedMethods->begin(), unsortedMethods->end());
        for (auto methodIter = sortedMethods.begin(); methodIter != sortedMethods.end(); ++methodIter)
        {
            HashCombineMethodSignature(outHash, methodIter->second);
        }
    }

    void BehaviorContextUtils::HashCombineMethodSignature(size_t& outHash, const AZ::BehaviorMethod* const behaviorMethod)
    {
        if (!behaviorMethod)
        {
            // Soft handle this case:
            // 1. This case should fail much earlier when BC gets initialized
            // 2. Hash value won't be affected even if this case is consistent
            return;
        }

        // Hash return type
        if (behaviorMethod->HasResult())
        {
            AZStd::hash_combine(outHash, behaviorMethod->GetResult()->m_typeId);
        }
        // Hash method name
        AZStd::hash_combine(outHash, behaviorMethod->m_name);
        // Hash arguments type
        for (size_t i = 0; i < behaviorMethod->GetNumArguments(); ++i)
        {
            AZStd::hash_combine(outHash, behaviorMethod->GetArgument(i)->m_typeId);
        }
    }

    void BehaviorContextUtils::HashCombineProperties(size_t& outHash, const AZStd::unordered_map<AZStd::string, AZ::BehaviorProperty*>* const behaviorProperties)
    {
        if (!behaviorProperties)
        {
            return;
        }

        const AZStd::map<AZStd::string, AZ::BehaviorProperty*> sortedProperties(behaviorProperties->begin(), behaviorProperties->end());
        for (auto propertyIter = sortedProperties.begin(); propertyIter != sortedProperties.end(); ++propertyIter)
        {
            if (!propertyIter->second)
            {
                continue;
            }

            AZStd::hash_combine(outHash, propertyIter->second->m_name);
            HashCombineMethodSignature(outHash, propertyIter->second->m_getter);
            HashCombineMethodSignature(outHash, propertyIter->second->m_setter);
        }
    }

    bool BehaviorContextUtils::IsSameDataType(const AZ::BehaviorParameter* const parameter, Data::Type dataType)
    {
        return parameter && Data::GetBehaviorParameterDataType(*parameter) == dataType;
    }
}
