/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/RTTI/TypeInfo.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
//! Common types used across the LyShine UI system
namespace LyShine
{
    // Uuid strings for all component types define by LyShine.
    // This can be used to create components at runtime. E.g.:
    //     entity->CreateComponent(LyShine::UiTransform2dComponentUuid);
    inline constexpr AZ::TypeId UiButtonComponentUuid{ "{7329DFE8-0F3C-4629-B395-78B2CF646B96}" };
    inline constexpr AZ::TypeId UiCanvasComponentUuid{ "{50B8CF6C-B19A-4D86-AFE9-96EFB820D422}" };
    inline constexpr AZ::TypeId UiCheckboxComponentUuid{ "{68D62281-B360-4426-AACA-E8BDE8BFEB3A}" };
    inline constexpr AZ::TypeId UiDraggableComponentUuid{ "{C96B1EEF-033A-479B-829B-ED3555D0F33A}" };
    inline constexpr AZ::TypeId UiDropTargetComponentUuid{ "{E4B8ACE0-FCE7-42D8-9836-942F910168B4}" };
    inline constexpr AZ::TypeId UiDropdownComponentUuid{ "{BFC4D5A3-2F7C-4079-B19C-C2A06E93F343}" };
    inline constexpr AZ::TypeId UiDropdownOptionComponentUuid{ "{554E16E4-CBA1-4E57-B326-9731AA963BAA}" };
    inline constexpr AZ::TypeId UiDynamicLayoutComponentUuid{ "{690BEC14-3642-4247-BD96-FE414CCB7DE7}" };
    inline constexpr AZ::TypeId UiDynamicScrollBoxComponentUuid{ "{6982C200-4D32-43CC-A7F6-F54FA50FCFF5}" };
    inline constexpr AZ::TypeId UiElementComponentUuid{ "{4A97D63E-CE7A-45B6-AAE4-102DB4334688}" };
    inline constexpr AZ::TypeId UiFaderComponentUuid{ "{CD01FF77-2249-4ED8-BFFB-33A66A47E17C}" };
    inline constexpr AZ::TypeId UiFlipbookAnimationComponentUuid{ "{AACCFFA1-B694-4F8B-A289-391A56404E0C}" };
    inline constexpr AZ::TypeId UiImageComponentUuid{ "{BDBEFD23-DBB4-4726-A32D-4FEAC24E51F6}" };
    inline constexpr AZ::TypeId UiImageSequenceComponentUuid{ "{BFAC7909-3A56-4980-AF7C-261A99D6371B}" };
    inline constexpr AZ::TypeId UiLayoutColumnComponentUuid{ "{4BC2E786-360B-4426-8D9C-9B254C5EA21F}" };
    inline constexpr AZ::TypeId UiLayoutCellComponentUuid{ "{A0568E58-4382-47F8-8B88-77C64B99AC80}" };
    inline constexpr AZ::TypeId UiLayoutFitterComponentUuid{ "{96C55390-9D03-4C47-BB0D-17D3BD5219E3}" };
    inline constexpr AZ::TypeId UiLayoutGridComponentUuid{ "{ADDA3AE5-B9AB-44B7-A462-8B89B398A837}" };
    inline constexpr AZ::TypeId UiLayoutRowComponentUuid{ "{7B2820C4-7FC7-4F02-B777-6727EB4BAC13}" };
    inline constexpr AZ::TypeId UiMarkupButtonComponentUuid{ "{D7EB1706-75E8-4C94-A266-61913F42431B}" };
    inline constexpr AZ::TypeId UiMaskComponentUuid{ "{2279AA38-271D-4D4F-A472-E42B984088AC}" };
    inline constexpr AZ::TypeId UiParticleEmitterComponentUuid{ "{E8B1288C-CB49-4CAD-B366-AF1B40C47E74}" };
    inline constexpr AZ::TypeId UiRadioButtonComponentUuid{ "{773A0F4B-F9F9-4B82-9951-BBD73A737DE4}" };
    inline constexpr AZ::TypeId UiRadioButtonGroupComponentUuid{ "{F89D5EF2-37C4-4CA7-9AA2-23DC3867EC9D}" };
    inline constexpr AZ::TypeId UiScrollBarComponentUuid{ "{6B283F90-3519-47DA-A1DD-65A79CE119CF}" };
    inline constexpr AZ::TypeId UiScrollBoxComponentUuid{ "{2F539588-AEAB-4341-A6A6-AF645D129693}" };
    inline constexpr AZ::TypeId UiSliderComponentUuid{ "{2913D76B-36A0-45E0-A104-33C668EB612D}" };
    inline constexpr AZ::TypeId UiTextComponentUuid{ "{5B3FB2A7-5DC4-4033-A970-001CEC85B6C4}" };
    inline constexpr AZ::TypeId UiTextInputComponentUuid{ "{2CB3872B-D2B4-4DDB-B39A-97492310AE11}" };
    inline constexpr AZ::TypeId UiTooltipComponentUuid{ "{493EBF89-C299-4722-829D-4DFAB926795B}" };
    inline constexpr AZ::TypeId UiTooltipDisplayComponentUuid{ "{18CEE6A7-3CBC-4638-9F4D-87E8D53DDF1A}" };
    inline constexpr AZ::TypeId UiTransform2dComponentUuid{ "{2751A5A5-3291-4A4D-9FC0-9CB0EB8D1DE6}" };

    // The Uuid of the LyShine system component. Can be used from tools to activate this component.
    inline constexpr AZ::TypeId lyShineSystemComponentUuid{ "{B0C78B8D-1E5B-47D7-95D0-EC69C0513804}" };
};
