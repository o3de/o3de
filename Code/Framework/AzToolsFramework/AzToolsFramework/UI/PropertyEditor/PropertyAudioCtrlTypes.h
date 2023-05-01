/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/string/string.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>

namespace AZ
{
    class ReflectContext;
}

namespace AzToolsFramework
{
    //=========================================================================
    enum class AudioPropertyType : AZ::u32
    {
        Trigger = 0,
        Rtpc,
        Switch,
        SwitchState,
        Environment,
        Preload,
        NumTypes,
    };

    //=========================================================================
    class CReflectedVarAudioControl
    {
    public:
        AZ_RTTI(CReflectedVarAudioControl, "{00016E8C-06FB-48D2-B482-1848343094D3}");
        AZ_CLASS_ALLOCATOR(CReflectedVarAudioControl, AZ::SystemAllocator);

        CReflectedVarAudioControl() = default;
        virtual ~CReflectedVarAudioControl() = default;

        AZStd::string m_controlName;
        AudioPropertyType m_propertyType = AudioPropertyType::NumTypes;

        static void Reflect(AZ::ReflectContext* context);
    };

} // namespace AzToolsFramework
