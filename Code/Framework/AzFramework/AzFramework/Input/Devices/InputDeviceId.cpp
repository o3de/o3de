/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Input/Devices/InputDeviceId.h>

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Script/ScriptContext.h>
#include <AzCore/ScriptCanvas/ScriptCanvasAttributes.h>
#include <AzCore/std/string/string.h>

namespace
{
    void InputDeviceIdScriptConstructor(AzFramework::InputDeviceId* thisPtr, AZ::ScriptDataContext& scriptDataContext)
    {
        new (thisPtr) AzFramework::InputDeviceId();

        const int argumentCount = scriptDataContext.GetNumArguments();
        if (argumentCount == 0)
        {
            return;
        }

        if ((argumentCount == 1 || argumentCount == 2)
            && scriptDataContext.IsString(0)
            && (argumentCount == 1 || scriptDataContext.IsNumber(1)))
        {
            const char* name = nullptr;
            scriptDataContext.ReadArg(0, name);

            AZ::u32 index = 0;
            if (argumentCount == 2)
            {
                scriptDataContext.ReadArg(1, index);
            }

            *thisPtr = AzFramework::InputDeviceId(name, index);
            return;
        }

        scriptDataContext.GetScriptContext()->Error(
            AZ::ScriptContext::ErrorType::Error,
            true,
            "InputDeviceId expects zero arguments, or a string name and an optional numeric index");
    }
} // namespace

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceId::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            const AZ::BehaviorParameterOverrides nameParameter(
                "Name", "Name of the input device, such as gamepad, keyboard, or mouse");
            const AZ::BehaviorParameterOverrides indexParameter(
                "Index", "Zero-based index used to address a specific device of the same type");

            behaviorContext->Class<InputDeviceId>()
                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Input")
                ->Attribute(AZ::ScriptCanvasAttributes::AllowInternalCreation, true)
                ->Attribute(AZ::ScriptCanvasAttributes::VariableCreationForbidden, true)
                ->Constructor<const char*>()
                ->Constructor<const char*, AZ::u32>()
                ->Attribute(AZ::Script::Attributes::ConstructorOverride, &InputDeviceIdScriptConstructor)
                ->Method(
                    "Create",
                    [](const AZStd::string& name, AZ::u32 index)
                    {
                        return InputDeviceId(name, index);
                    },
                    { nameParameter, indexParameter },
                    "Creates an input device id for use as the source of addressed input requests")
                ->Property("name", [](InputDeviceId* thisPtr) { return thisPtr->GetName(); }, nullptr)
                ->Property("index", BehaviorValueProperty(&InputDeviceId::m_index))
            ;
        }
    }
} // namespace AzFramework
