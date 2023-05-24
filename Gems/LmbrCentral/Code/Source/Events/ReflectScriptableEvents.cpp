/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "Events/ReflectScriptableEvents.h"
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Script/ScriptContext.h>
#include <AzCore/Script/ScriptContextAttributes.h>
#include <LmbrCentral/Scripting/GameplayNotificationBus.h>
#include <AzCore/Math/Vector3.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Quaternion.h>


namespace LmbrCentral
{
    /// BahaviorContext forwarder for FloatGameplayNotificationBus
    class BehaviorGameplayNotificationBusHandler : public AZ::GameplayNotificationBus::Handler, public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(BehaviorGameplayNotificationBusHandler, "{227DCFE6-B527-4FED-8A4D-5D723B07EAA5}", AZ::SystemAllocator,
            OnEventBegin, OnEventUpdating, OnEventEnd);

        void OnEventBegin(const AZStd::any& value) override
        {
            Call(FN_OnEventBegin, value);
        }

        void OnEventUpdating(const AZStd::any& value) override
        {
            Call(FN_OnEventUpdating, value);
        }

        void OnEventEnd(const AZStd::any& value) override
        {
            Call(FN_OnEventEnd, value);
        }
    };

    class MathUtils
    {
    public:
        AZ_TYPE_INFO(MathUtils, "{BB7F7465-B355-4435-BB9D-44D8F586EE8B}");
        AZ_CLASS_ALLOCATOR(MathUtils, AZ::SystemAllocator);

        MathUtils() = default;
        ~MathUtils() = default;
    };

    class AxisWrapper
    {
    public:
        AZ_TYPE_INFO(AxisWrapper, "{86817913-7D0C-4883-8EDC-2B0DE643392B}");
        AZ_CLASS_ALLOCATOR(AxisWrapper, AZ::SystemAllocator);

        AxisWrapper() = default;
        ~AxisWrapper() = default;
    };

    void GameplayEventIdNonIntrusiveConstructor(AZ::GameplayNotificationId* outData, AZ::ScriptDataContext& dc)
    {
        static const int s_channelIndex = 0;
        static const int s_actionNameIndex = 1;
        static const int s_payloadTypeIndex = 2;
        static const int s_defaultConstructorArgCount = 0;
        static const int s_deprecatedConstructorArgCount = 2;
        static const int s_verboseConstructorArgCount = 3;
        static const bool s_showCallStack = true;
        if (dc.GetNumArguments() == s_defaultConstructorArgCount)
        {
            // Use defaults.
            outData->m_channel.SetInvalid();
            outData->m_actionNameCrc = 0;
            outData->m_payloadTypeId = AZ::Uuid::CreateNull();
        }
        else if (dc.GetNumArguments() == s_deprecatedConstructorArgCount 
            && dc.IsClass<AZ::EntityId>(s_channelIndex) 
            && dc.IsString(s_actionNameIndex))
        {
            AZ::EntityId channel(0);
            dc.ReadArg(s_channelIndex, channel);
            outData->m_channel = channel;
            const char* actionName = nullptr;
            dc.ReadArg(s_actionNameIndex, actionName);
            outData->m_actionNameCrc = AZ_CRC(actionName);
            outData->m_payloadTypeId = AZ::Uuid::CreateNull();
            AZ::ScriptContext::FromNativeContext(dc.GetNativeContext())->Error(
                AZ::ScriptContext::ErrorType::Warning, 
                s_showCallStack,
                "This constructor has been deprecated.  Please add the name of the type you wish to send/receive, example \"float\""
            );
        }
        else if (dc.GetNumArguments() == s_verboseConstructorArgCount 
            && dc.IsClass<AZ::EntityId>(s_channelIndex)
            && (dc.IsString(s_actionNameIndex) || dc.IsClass<AZ::Crc32>(s_actionNameIndex))
            && (dc.IsString(s_payloadTypeIndex) || dc.IsClass<AZ::Crc32>(s_payloadTypeIndex) || dc.IsClass<AZ::Uuid>(s_payloadTypeIndex)))
        {
            dc.ReadArg(s_channelIndex, outData->m_channel);
            if (dc.IsString(s_actionNameIndex))
            {
                const char* actionName = nullptr;
                dc.ReadArg(s_actionNameIndex, actionName);
                outData->m_actionNameCrc = AZ_CRC(actionName);
            }
            else
            {
                dc.ReadArg(s_actionNameIndex, outData->m_actionNameCrc);
            }

            if (dc.IsClass<AZ::Uuid>(s_payloadTypeIndex))
            {
                dc.ReadArg(s_payloadTypeIndex, outData->m_payloadTypeId);
            }
            else
            {
                AZ::BehaviorContext* behaviorContext = nullptr;
                AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationBus::Events::GetBehaviorContext);
                const char* payloadClassName = nullptr;
                if (dc.IsString(s_payloadTypeIndex))
                {
                    dc.ReadArg(s_payloadTypeIndex, payloadClassName);
                    // Specifically handling float for gameplay event bus support.
                    // LuaNumber is a double, so typeid(1) will return double, and fundamental numerics are not exposed to BehaviorContext.
                    // This is case sensitive since it is possible to have float and Float and FlOaT, etc.
                    if (strcmp(payloadClassName, "float") == 0)
                    {
                        outData->m_payloadTypeId = AZ::AzTypeInfo<float>::Uuid();
                    }
                    else
                    {
                        auto&& behaviorClassIterator = behaviorContext->m_classes.find(payloadClassName);
                        if (behaviorClassIterator != behaviorContext->m_classes.end())
                        {
                            outData->m_payloadTypeId = behaviorClassIterator->second->m_typeId;
                        }
                        else
                        {
                            AZ::ScriptContext::FromNativeContext(dc.GetNativeContext())->Error(
                                AZ::ScriptContext::ErrorType::Warning,
                                s_showCallStack,
                                "Class \"%s\" not found in behavior context.  Ensure your type is reflected to behavior context or consider using typeid(type).",
                                payloadClassName
                            );
                        }
                    }
                }
                else
                {
                    AZ::Crc32 requestedCrc;
                    dc.ReadArg(s_payloadTypeIndex, requestedCrc);
                    AZ::ScriptContext::FromNativeContext(dc.GetNativeContext())->Error(
                        AZ::ScriptContext::ErrorType::Warning, 
                        s_showCallStack,
                        "Constructing a GameplayNotificationId with a Crc32 for payload type is expensive. Consider using string name or typeid instead."
                    );
                    for (auto&& behaviorClassPair : behaviorContext->m_classes)
                    {
                        AZ::Crc32 currentBehaviorClassCrc = AZ::Crc32(behaviorClassPair.first.c_str());
                        if (currentBehaviorClassCrc == requestedCrc)
                        {
                            outData->m_payloadTypeId = behaviorClassPair.second->m_typeId;
                        }
                    }
                }
            }
        }
        else
        {
            AZ::ScriptContext::FromNativeContext(dc.GetNativeContext())->Error(
                AZ::ScriptContext::ErrorType::Error, 
                s_showCallStack,
                "The GameplayNotificationId takes 3 arguments: an entityId representing the channel, a string or crc representing the action's name, and a string or uuid for the type"
            );
        }
    }

    void ReflectScriptableEvents::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<AZ::GameplayNotificationId>()
                ->Version(1)
                ->Field("Channel", &AZ::GameplayNotificationId::m_channel)
                ->Field("ActionName", &AZ::GameplayNotificationId::m_actionNameCrc)
                ->Field("PayloadType", &AZ::GameplayNotificationId::m_payloadTypeId)
            ;
        }

        ShapeComponentConfig::Reflect(context);

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<AZ::GameplayNotificationId>("GameplayNotificationId")
                ->Attribute(AZ::Script::Attributes::Deprecated, true)
                ->Constructor<AZ::EntityId, AZ::Crc32>()
                    ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                    ->Attribute(AZ::Script::Attributes::ConstructorOverride, &GameplayEventIdNonIntrusiveConstructor)
                ->Property("actionNameCrc", BehaviorValueProperty(&AZ::GameplayNotificationId::m_actionNameCrc))
                ->Property("channel", BehaviorValueProperty(&AZ::GameplayNotificationId::m_channel))
                ->Property("payloadTypeId", BehaviorValueProperty(&AZ::GameplayNotificationId::m_payloadTypeId))
                ->Method("ToString", &AZ::GameplayNotificationId::ToString)
                    ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::ToString)
                ->Method("Equal", &AZ::GameplayNotificationId::operator==)
                    ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Equal)
                ->Method("Clone", &AZ::GameplayNotificationId::Clone);

            behaviorContext->EBus<AZ::GameplayNotificationBus>("GameplayNotificationBus")
                ->Attribute(AZ::Script::Attributes::Deprecated, true)
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::List)
                ->Handler<BehaviorGameplayNotificationBusHandler>()
                ->Event("OnEventBegin", &AZ::GameplayNotificationBus::Events::OnEventBegin)
                ->Event("OnEventUpdating", &AZ::GameplayNotificationBus::Events::OnEventUpdating)
                ->Event("OnEventEnd", &AZ::GameplayNotificationBus::Events::OnEventEnd);

            behaviorContext->Class<AxisWrapper>("AxisType")
                ->Constant("XPositive", BehaviorConstant(AZ::Transform::Axis::XPositive))
                ->Constant("XNegative", BehaviorConstant(AZ::Transform::Axis::XNegative))
                ->Constant("YPositive", BehaviorConstant(AZ::Transform::Axis::YPositive))
                ->Constant("YNegative", BehaviorConstant(AZ::Transform::Axis::YNegative))
                ->Constant("ZPositive", BehaviorConstant(AZ::Transform::Axis::ZPositive))
                ->Constant("ZNegative", BehaviorConstant(AZ::Transform::Axis::ZNegative));

            behaviorContext->Class<MathUtils>("MathUtils")
                ->Method("ConvertTransformToEulerDegrees", &AZ::ConvertTransformToEulerDegrees)
                ->Method("ConvertTransformToEulerRadians", &AZ::ConvertTransformToEulerRadians)
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Method("ConvertEulerDegreesToTransform", &AZ::ConvertEulerDegreesToTransform)
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Method("ConvertEulerDegreesToTransformPrecise", &AZ::ConvertEulerDegreesToTransform)
                ->Method("ConvertQuaternionToEulerDegrees", &AZ::ConvertQuaternionToEulerDegrees)
                ->Method("ConvertQuaternionToEulerRadians", &AZ::ConvertQuaternionToEulerRadians)
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Method("ConvertEulerRadiansToQuaternion", &AZ::ConvertEulerRadiansToQuaternion)
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Method("ConvertEulerDegreesToQuaternion", &AZ::ConvertEulerDegreesToQuaternion)
                ->Method("CreateLookAt", &AZ::Transform::CreateLookAt);

            ShapeComponentGeneric::Reflect(behaviorContext);
        }
    }

}
