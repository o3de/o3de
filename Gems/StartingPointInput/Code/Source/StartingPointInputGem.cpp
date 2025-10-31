/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "InputConfigurationComponent.h"
#include "InputEventBindings.h"
#include "InputEventMap.h"

#include <AzCore/Module/Module.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <AzCore/Module/Environment.h>
#include <AzCore/Component/Component.h>

#include <AzFramework/Asset/GenericAssetHandler.h>

namespace StartingPointInput
{
    static bool ConvertToInputEventMap(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
    {
        // Capture the old values
        AZStd::string deviceType;
        classElement.GetChildData(AZ::Crc32("Input Device Type"), deviceType);

        AZStd::string inputName;
        classElement.GetChildData(AZ::Crc32("Input Name"), inputName);

        float eventValueMultiplier;
        classElement.GetChildData(AZ::Crc32("Event Value Multiplier"), eventValueMultiplier);

        float deadZone;
        classElement.GetChildData(AZ::Crc32("Dead Zone"), deadZone);

        // Convert to the new class
        classElement.Convert(context, AZ::AzTypeInfo<InputEventMap>::Uuid());

        // Add the old values to the new class
        classElement.AddElementWithData(context, "Input Device Type", deviceType);
        classElement.AddElementWithData(context, "Input Name", inputName);
        classElement.AddElementWithData(context, "Event Value Multiplier", eventValueMultiplier);
        classElement.AddElementWithData(context, "Dead Zone", deadZone);

        return true;
    }

    class BehaviorInputEventNotificationBusHandler : public InputEventNotificationBus::Handler, public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(BehaviorInputEventNotificationBusHandler, "{8AAEEB1A-21E2-4D2E-A719-73552D41F506}", AZ::SystemAllocator,
            OnPressed, OnHeld, OnReleased);

        void OnPressed(float value) override
        {
            Call(FN_OnPressed, value);
        }

        void OnHeld(float value) override
        {
            Call(FN_OnHeld, value);
        }

        void OnReleased(float value) override
        {
            Call(FN_OnReleased, value);
        }
    };

    void InputEventNonIntrusiveConstructor(InputEventNotificationId* thisOutPtr, AZ::ScriptDataContext& dc)
    {
        if (dc.GetNumArguments() == 0)
        {
            // Use defaults.
        }
        else if (dc.GetNumArguments() == 1 && dc.IsString(0))
        {
            thisOutPtr->m_localUserId = AzFramework::LocalUserIdAny;
            const char* actionName = nullptr;
            dc.ReadArg(0, actionName);
            thisOutPtr->m_actionNameCrc = AZ::Crc32(actionName);
        }
        else if (dc.GetNumArguments() == 2 && dc.IsClass<AZ::Crc32>(0) && dc.IsString(1))
        {
            AzFramework::LocalUserId localUserId = 0;
            dc.ReadArg(0, localUserId);
            thisOutPtr->m_localUserId = localUserId;
            const char* actionName = nullptr;
            dc.ReadArg(1, actionName);
            thisOutPtr->m_actionNameCrc = AZ::Crc32(actionName);
        }
        else
        {
            AZ_Error("InputEventNotificationId", false, "The InputEventNotificationId takes one or two args. 1 argument: a string representing the input events name (determined by the event group). 2 arguments: a Crc of the profile channel, and a string representing the input event's name");
        }
    }

    class StartingPointInputSystemComponent : public AZ::Component
    {
    public:
        AZ_COMPONENT(StartingPointInputSystemComponent, "{95DE3485-5E51-42A9-899D-433EC3448AA3}");

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC_CE("AssetDatabaseService"));
            required.push_back(AZ_CRC_CE("AssetCatalogService"));
        }

        static void Reflect(AZ::ReflectContext* context)
        {
            InputEventBindingsAsset::Reflect(context);
            InputEventBindings::Reflect(context);
            InputEventGroup::Reflect(context);
            InputEventMap::Reflect(context);
            ThumbstickInputEventMap::Reflect(context);

            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<StartingPointInputSystemComponent, AZ::Component>()
                    ->Version(1)
                    ;
                serializeContext->ClassDeprecate("Input", AZ::Uuid("{546C9EBC-90EF-4F03-891A-0736BE2A487E}"), &ConvertToInputEventMap);

                serializeContext->Class<InputEventNotificationId>()
                    ->Version(1)
                    ->Field("LocalUserId", &InputEventNotificationId::m_localUserId)
                    ->Field("ActionName", &InputEventNotificationId::m_actionNameCrc)
                    ;

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<StartingPointInputSystemComponent>(
                        "Starting point input", "Manages input bindings and events")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Editor")
                        ;
                }
            }

            if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->Class<InputEventNotificationId>("InputEventNotificationId")
                    ->Constructor<const char*>()
                        ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                        ->Attribute(AZ::Script::Attributes::ConstructorOverride, &InputEventNonIntrusiveConstructor)
                    ->Property("actionNameCrc", BehaviorValueProperty(&InputEventNotificationId::m_actionNameCrc))
                    ->Property("localUserId", BehaviorValueProperty(&InputEventNotificationId::m_localUserId))
                    ->Method("ToString", &InputEventNotificationId::ToString)
                        ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::ToString)
                    ->Method("Equal", &InputEventNotificationId::operator==)
                        ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Equal)
                    ->Method("Clone", &InputEventNotificationId::Clone)
                    ->Property("actionName", nullptr, [](InputEventNotificationId* thisPtr, AZStd::string_view value) { *thisPtr = InputEventNotificationId(value.data()); })
                    ->Method("CreateInputEventNotificationId", [](AzFramework::LocalUserId localUserId, AZStd::string_view value) -> InputEventNotificationId { return InputEventNotificationId(localUserId, value.data()); },
                    { { { "localUserId", "Local User ID" },
                        { "actionName", "The name of the Input event action used to create an InputEventNotificationId" } } });

                behaviorContext->EBus<InputEventNotificationBus>("InputEventNotificationBus")
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::List)
                    ->Handler<BehaviorInputEventNotificationBusHandler>()
                    ->Event("OnPressed", &InputEventNotificationBus::Events::OnPressed)
                    ->Event("OnHeld", &InputEventNotificationBus::Events::OnHeld)
                    ->Event("OnReleased", &InputEventNotificationBus::Events::OnReleased);
            }
        }

        void Init() override
        {
        }

        void Activate() override
        {
            // Register asset handlers. Requires "AssetDatabaseService"
            AZ_Assert(AZ::Data::AssetManager::IsReady(), "Asset manager isn't ready!");

            m_inputEventBindingsAssetHandler = aznew AzFramework::GenericAssetHandler<InputEventBindingsAsset>("Input Bindings", "Other", "inputbindings", AZ::AzTypeInfo<InputConfigurationComponent>::Uuid());
            m_inputEventBindingsAssetHandler->Register();
        }

        void Deactivate() override
        {
            delete m_inputEventBindingsAssetHandler;
            m_inputEventBindingsAssetHandler = nullptr;
        }

    private:
        AzFramework::GenericAssetHandler<InputEventBindingsAsset>* m_inputEventBindingsAssetHandler = nullptr;
    };

    class StartingPointInputModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(StartingPointInputModule, "{B30D421E-127D-4C46-90B1-AC3DDF3EC1D9}", AZ::Module);

        StartingPointInputModule()
            : AZ::Module()
        {
            m_descriptors.insert(m_descriptors.end(), {
                InputConfigurationComponent::CreateDescriptor(),
                StartingPointInputSystemComponent::CreateDescriptor(),
            });
        }

        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        { 
            return AZ::ComponentTypeList({ StartingPointInputSystemComponent::RTTI_Type() }); 
        }
    };
}

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), StartingPointInput::StartingPointInputModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_StartingPointInput, StartingPointInput::StartingPointInputModule)
#endif
