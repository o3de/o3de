/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ShineModule.h"
#include "ShineSystemComponent.h"
#if defined(SHINE_EDITOR)
#include "ShineEditorSystemComponent.h"
#endif
#include "ShineLoadScreen.h"

#include "UiCanvasComponent.h"
#include "UiElementComponent.h"
#include "UiTransform2dComponent.h"
#include "UiImageComponent.h"
#include "UiImageSequenceComponent.h"
#include "UiTextComponent.h"
#include "UiButtonComponent.h"
#include "UiMarkupButtonComponent.h"
#include "UiCheckboxComponent.h"
#include "UiDraggableComponent.h"
#include "UiDropTargetComponent.h"
#include "UiDropdownComponent.h"
#include "UiDropdownOptionComponent.h"
#include "UiSliderComponent.h"
#include "UiTextInputComponent.h"
#include "UiScrollBarComponent.h"
#include "UiScrollBoxComponent.h"
#include "UiFaderComponent.h"
#include "UiFlipbookAnimationComponent.h"
#include "UiLayoutFitterComponent.h"
#include "UiMaskComponent.h"
#include "UiLayoutCellComponent.h"
#include "UiLayoutColumnComponent.h"
#include "UiLayoutRowComponent.h"
#include "UiLayoutGridComponent.h"
#include "UiParticleEmitterComponent.h"
#include "UiRadioButtonComponent.h"
#include "UiRadioButtonGroupComponent.h"
#include "UiTooltipComponent.h"
#include "UiTooltipDisplayComponent.h"
#include "UiDynamicLayoutComponent.h"
#include "UiDynamicScrollBoxComponent.h"
#include "UiSpawnerComponent.h"
#include "UiHierarchyInteractivityToggleComponent.h"

#include "World/UiCanvasAssetRefComponent.h"
#include "World/UiCanvasProxyRefComponent.h"
#include "World/UiCanvasOnMeshComponent.h"

#if defined(SHINE_BUILDER)
#include "Pipeline/ShineBuilder/ShineBuilderComponent.h"
#endif // SHINE_BUILDER

#include <CryCommon/LoadScreenBus.h>

namespace Shine
{
    ShineModule::ShineModule()
        : CryHooksModule()
    {
        // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
        m_descriptors.insert(m_descriptors.end(), {
                ShineSystemComponent::CreateDescriptor(),
#if defined(SHINE_EDITOR)
                ShineEditor::ShineEditorSystemComponent::CreateDescriptor(),
#endif
                UiCanvasAssetRefComponent::CreateDescriptor(),
                UiCanvasProxyRefComponent::CreateDescriptor(),
                UiCanvasOnMeshComponent::CreateDescriptor(),
                UiCanvasComponent::CreateDescriptor(),
                UiElementComponent::CreateDescriptor(),
                UiTransform2dComponent::CreateDescriptor(),
                UiImageComponent::CreateDescriptor(),
                UiImageSequenceComponent::CreateDescriptor(),
                UiTextComponent::CreateDescriptor(),
                UiButtonComponent::CreateDescriptor(),
                UiMarkupButtonComponent::CreateDescriptor(),
                UiCheckboxComponent::CreateDescriptor(),
                UiDraggableComponent::CreateDescriptor(),
                UiDropTargetComponent::CreateDescriptor(),
                UiDropdownComponent::CreateDescriptor(),
                UiDropdownOptionComponent::CreateDescriptor(),
                UiSliderComponent::CreateDescriptor(),
                UiTextInputComponent::CreateDescriptor(),
                UiScrollBoxComponent::CreateDescriptor(),
                UiScrollBarComponent::CreateDescriptor(),
                UiFaderComponent::CreateDescriptor(),
                UiFlipbookAnimationComponent::CreateDescriptor(),
                UiLayoutFitterComponent::CreateDescriptor(),
                UiMaskComponent::CreateDescriptor(),
                UiLayoutCellComponent::CreateDescriptor(),
                UiLayoutColumnComponent::CreateDescriptor(),
                UiLayoutRowComponent::CreateDescriptor(),
                UiLayoutGridComponent::CreateDescriptor(),
                UiTooltipComponent::CreateDescriptor(),
                UiTooltipDisplayComponent::CreateDescriptor(),
                UiDynamicLayoutComponent::CreateDescriptor(),
                UiDynamicScrollBoxComponent::CreateDescriptor(),
                UiSpawnerComponent::CreateDescriptor(),
                UiRadioButtonComponent::CreateDescriptor(),
                UiRadioButtonGroupComponent::CreateDescriptor(),
                UiParticleEmitterComponent::CreateDescriptor(),
                UiHierarchyInteractivityToggleComponent::CreateDescriptor(),
    #if defined(SHINE_BUILDER)
                // Builder
                ShineBuilder::ShineBuilderComponent::CreateDescriptor(),
    #endif
    #if AZ_LOADSCREENCOMPONENT_ENABLED
                ShineLoadScreenComponent::CreateDescriptor(),
    #endif
            });

        // This is so the metrics system knows which component Shine is registering
        ShineSystemComponent::SetShineComponentDescriptors(&m_descriptors);
    }

    /**
     * Add required SystemComponents to the SystemEntity.
     */
    AZ::ComponentTypeList ShineModule::GetRequiredSystemComponents() const
    {
        return AZ::ComponentTypeList{
                   azrtti_typeid<ShineSystemComponent>(),
    #if defined(SHINE_EDITOR)
                   azrtti_typeid<ShineEditor::ShineEditorSystemComponent>(),
    #endif
    #if AZ_LOADSCREENCOMPONENT_ENABLED
                   azrtti_typeid<ShineLoadScreenComponent>(),
    #endif
        };
    }
}

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), Shine::ShineModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_Shine, Shine::ShineModule)
#endif
