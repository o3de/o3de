/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/RTTI/BehaviorContext.h>
#include <ScriptCanvas/Core/Core.h>
#include <ScriptCanvas/Core/MethodConfiguration.h>
#include <ScriptCanvas/Data/Data.h>

namespace ScriptCanvas
{
    class BehaviorContextUtils
    {
    public:
        static bool FindClass(const AZ::BehaviorMethod*& outMethod, const AZ::BehaviorClass*& outClass, AZStd::string_view className, AZStd::string_view methodName, PropertyStatus propertyStatus = PropertyStatus::None, AZStd::string* outPrettyClassName = nullptr, bool warnOnMissing = true);
        static bool FindEBus(const AZ::BehaviorEBus*& outEBus, AZStd::string_view ebusName, bool warnOnMissing = true);
        static bool FindExplicitOverload(const AZ::BehaviorMethod*& outMethod, const AZ::BehaviorClass*& outClass, AZStd::string_view className, AZStd::string_view methodName, AZStd::string* outPrettyClassName = nullptr);
        static AZStd::string FindExposedMethodName(const AZ::BehaviorMethod& method, const AZ::BehaviorClass* behaviorClass);
        static bool FindEvent(const AZ::BehaviorMethod*& outMethod, AZStd::string_view ebusName, AZStd::string_view eventName, EventType* outEventType = nullptr, bool warnOnMissing = true);
        static bool FindEvent(const AZ::BehaviorMethod*& outMethod, const AZ::BehaviorEBus* const ebus, AZStd::string_view eventName, EventType* outEventType = nullptr, bool warnOnMissing = true);
        static bool FindFree(const AZ::BehaviorMethod*& outMethod, AZStd::string_view methodName, bool warnOnMissing = true);

        static size_t GenerateFingerprintForBehaviorContext();
        static size_t GenerateFingerprintForMethod(const MethodType& methodType, const AZStd::string& className, const AZStd::string& methodName);

        static AZStd::pair<const AZ::BehaviorMethod*, const AZ::BehaviorClass*> GetCheck(const AZ::BehaviorMethod& method);

        static AZ::EBusAddressPolicy GetEBusAddressPolicy(const AZ::BehaviorEBus& ebus);
        static AZ::EBusAddressPolicy GetEBusAddressPolicyByUuid(AZ::Uuid addressTypeUuid);
        static AZ::BehaviorMethod* GetEventMethod(const AZ::BehaviorEBus& ebus, const AZ::BehaviorEBusEventSender& ebusEventSender);
        static EventType GetEventType(const AZ::BehaviorEBus& ebus);

        static void* ConstructTuple(const AZ::TypeId& typeID);
        static AZStd::pair<void*, AZ::BehaviorContext*> ConstructTupleGetContext(const AZ::TypeId& typeID);
        static AZStd::vector<AZ::TypeId> GetUnpackedTypes(const AZ::TypeId& typeID);

        static void HashCombineClasses(size_t& outHash, const AZStd::unordered_map<AZStd::string, AZ::BehaviorClass*>* const unsortedClasses);
        static void HashCombineEBuses(size_t& outHash, const AZStd::unordered_map<AZStd::string, AZ::BehaviorEBus*>* const unsortedEBuses);
        static void HashCombineEvents(size_t& outHash, const AZ::BehaviorEBus* const ebus);
        static void HashCombineMethods(size_t& outHash, const AZStd::unordered_map<AZStd::string, AZ::BehaviorMethod*>* const unsortedMethods);
        static void HashCombineMethodSignature(size_t& outHash, const AZ::BehaviorMethod* const behaviorMethod);
        static void HashCombineProperties(size_t& outHash, const AZStd::unordered_map<AZStd::string, AZ::BehaviorProperty*>* const behaviorProperties);

        static bool IsSameDataType(const AZ::BehaviorParameter* const parameter, Data::Type dataType);
    };
} 
