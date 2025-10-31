/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "UiSerialize.h"

#include <LyShine/UiAssetTypes.h>
#include <LyShine/IDraw2d.h>
#include <LyShine/UiBase.h>
#include "UiInteractableComponent.h"
#include <LyShine/UiSerializeHelpers.h>

#include <LyShine/Bus/UiParticleEmitterBus.h>
#include <LyShine/Bus/UiImageBus.h>
#include <LyShine/Bus/UiTransform2dBus.h>

#include <UiElementComponent.h>
#include <UiCanvasComponent.h>
#include <UiLayoutGridComponent.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Script/ScriptContext.h>
#include <AzCore/Slice/SliceComponent.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
// NAMESPACE FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

namespace UiSerialize
{
    //////////////////////////////////////////////////////////////////////////
    void UiOffsetsScriptConstructor(UiTransform2dInterface::Offsets* thisPtr, AZ::ScriptDataContext& dc)
    {
        int numArgs = dc.GetNumArguments();

        const int noArgsGiven = 0;
        const int allArgsGiven = 4;

        switch (numArgs)
        {
        case noArgsGiven:
        {
            *thisPtr = UiTransform2dInterface::Offsets();
        }
        break;

        case allArgsGiven:
        {
            if (dc.IsNumber(0) && dc.IsNumber(1) && dc.IsNumber(2) && dc.IsNumber(3))
            {
                float left = 0;
                float top = 0;
                float right = 0;
                float bottom = 0;
                dc.ReadArg(0, left);
                dc.ReadArg(1, top);
                dc.ReadArg(2, right);
                dc.ReadArg(3, bottom);
                *thisPtr = UiTransform2dInterface::Offsets(left, top, right, bottom);
            }
            else
            {
                dc.GetScriptContext()->Error(AZ::ScriptContext::ErrorType::Error, true, "When providing 4 arguments to UiOffsets(), all must be numbers!");
            }
        }
        break;

        default:
        {
            dc.GetScriptContext()->Error(AZ::ScriptContext::ErrorType::Error, true, "UiOffsets() accepts only 0 or 4 arguments, not %d!", numArgs);
        }
        break;
        }
    }

    //////////////////////////////////////////////////////////////////////////
    void UiAnchorsScriptConstructor(UiTransform2dInterface::Anchors* thisPtr, AZ::ScriptDataContext& dc)
    {
        int numArgs = dc.GetNumArguments();

        const int noArgsGiven = 0;
        const int allArgsGiven = 4;

        switch (numArgs)
        {
        case noArgsGiven:
        {
            *thisPtr = UiTransform2dInterface::Anchors();
        }
        break;

        case allArgsGiven:
        {
            if (dc.IsNumber(0) && dc.IsNumber(1) && dc.IsNumber(2) && dc.IsNumber(3))
            {
                float left = 0;
                float top = 0;
                float right = 0;
                float bottom = 0;
                dc.ReadArg(0, left);
                dc.ReadArg(1, top);
                dc.ReadArg(2, right);
                dc.ReadArg(3, bottom);
                *thisPtr = UiTransform2dInterface::Anchors(left, top, right, bottom);
            }
            else
            {
                dc.GetScriptContext()->Error(AZ::ScriptContext::ErrorType::Error, true, "When providing 4 arguments to UiAnchors(), all must be numbers!");
            }
        }
        break;

        default:
        {
            dc.GetScriptContext()->Error(AZ::ScriptContext::ErrorType::Error, true, "UiAnchors() accepts only 0 or 4 arguments, not %d!", numArgs);
        }
        break;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void SetAnchorLeft(UiTransform2dInterface::Anchors* anchor, float left)
    {
        if (anchor)
        {
            anchor->m_left = left;
        }
        else
        {
            AZ_ErrorOnce("Script Canvas", false, "UI Script tried to set left on null anchor.")
        }
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void SetAnchorTop(UiTransform2dInterface::Anchors* anchor, float top)
    {
        if (anchor)
        {
            anchor->m_top = top;
        }
        else
        {
            AZ_ErrorOnce("Script Canvas", false, "UI Script tried to set top on null anchor.")
        }
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void SetAnchorRight(UiTransform2dInterface::Anchors* anchor, float right)
    {
        if (anchor)
        {
            anchor->m_right = right;
        }
        else
        {
            AZ_ErrorOnce("Script Canvas", false, "UI Script tried to set right on null anchor.")
        }
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void SetAnchorBottom(UiTransform2dInterface::Anchors* anchor, float bottom)
    {
        if (anchor)
        {
            anchor->m_bottom = bottom;
        }
        else
        {
            AZ_ErrorOnce("Script Canvas", false, "UI Script tried to set bottom on null anchor.")
        }
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void SetAnchors(UiTransform2dInterface::Anchors* anchor, float left, float top, float right, float bottom)
    {
        if (anchor)
        {
            anchor->m_left = left;
            anchor->m_top = top;
            anchor->m_right = right;
            anchor->m_bottom = bottom;
        }
        else
        {
            AZ_ErrorOnce("Script Canvas", false, "UI Script tried to set values on null anchor.")
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void SetOffsetLeft(UiTransform2dInterface::Offsets* offset, float left)
    {
        if (offset)
        {
            offset->m_left = left;
        }
        else
        {
            AZ_ErrorOnce("Script Canvas", false, "UI Script tried to set left on null offset.")
        }
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void SetOffsetTop(UiTransform2dInterface::Offsets* offset, float top)
    {
        if (offset)
        {
            offset->m_top = top;
        }
        else
        {
            AZ_ErrorOnce("Script Canvas", false, "UI Script tried to set top on null offset.")
        }
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void SetOffsetRight(UiTransform2dInterface::Offsets* offset, float right)
    {
        if (offset)
        {
            offset->m_right = right;
        }
        else
        {
            AZ_ErrorOnce("Script Canvas", false, "UI Script tried to set right on null offset.")
        }
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void SetOffsetBottom(UiTransform2dInterface::Offsets* offset, float bottom)
    {
        if (offset)
        {
            offset->m_bottom = bottom;
        }
        else
        {
            AZ_ErrorOnce("Script Canvas", false, "UI Script tried to set bottom on null offset.")
        }
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void SetOffsets(UiTransform2dInterface::Offsets* offset, float left, float top, float right, float bottom)
    {
        if (offset)
        {
            offset->m_left = left;
            offset->m_top = top;
            offset->m_right = right;
            offset->m_bottom = bottom;
        }
        else
        {
            AZ_ErrorOnce("Script Canvas", false, "UI Script tried to set values on null offset.")
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void SetPaddingLeft(UiLayoutInterface::Padding* padding, int left)
    {
        if (padding)
        {
            padding->m_left = left;
        }
        else
        {
            AZ_ErrorOnce("Script Canvas", false, "UI Script tried to set left on null padding.")
        }
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void SetPaddingTop(UiLayoutInterface::Padding* padding, int top)
    {
        if (padding)
        {
            padding->m_top = top;
        }
        else
        {
            AZ_ErrorOnce("Script Canvas", false, "UI Script tried to set top on null padding.")
        }
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void SetPaddingRight(UiLayoutInterface::Padding* padding, int right)
    {
        if (padding)
        {
            padding->m_right = right;
        }
        else
        {
            AZ_ErrorOnce("Script Canvas", false, "UI Script tried to set right on null padding.")
        }
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void SetPaddingBottom(UiLayoutInterface::Padding* padding, int bottom)
    {
        if (padding)
        {
            padding->m_bottom = bottom;
        }
        else
        {
            AZ_ErrorOnce("Script Canvas", false, "UI Script tried to set bottom on null padding.")
        }
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void SetPadding(UiLayoutInterface::Padding* padding, int left, int top, int right, int bottom)
    {
        if (padding)
        {
            padding->m_left = left;
            padding->m_top = top;
            padding->m_right = right;
            padding->m_bottom = bottom;
        }
        else
        {
            AZ_ErrorOnce("Script Canvas", false, "UI Script tried to set values on null padding.")
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void ReflectUiTypes(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);

        if (serializeContext)
        {
            serializeContext->Class<ColorF>()->
                Field("r", &ColorF::r)->
                Field("g", &ColorF::g)->
                Field("b", &ColorF::b)->
                Field("a", &ColorF::a);

            serializeContext->Class<ColorB>()->
                Field("r", &ColorB::r)->
                Field("g", &ColorB::g)->
                Field("b", &ColorB::b)->
                Field("a", &ColorB::a);
        }

        // Vec2 (still used in UI Animation sequence splines)
        {
            if (serializeContext)
            {
                serializeContext->Class<Vec2>()->
                    Field("x", &Vec2::x)->
                    Field("y", &Vec2::y);
            }
        }

        // Vec3 (possibly no longer used)
        {
            if (serializeContext)
            {
                serializeContext->Class<Vec3>()->
                    Field("x", &Vec3::x)->
                    Field("y", &Vec3::y)->
                    Field("z", &Vec3::z);
            }
        }

        // Anchors
        {
            if (serializeContext)
            {
                serializeContext->Class<UiTransform2dInterface::Anchors>()->
                    Field("left", &UiTransform2dInterface::Anchors::m_left)->
                    Field("top", &UiTransform2dInterface::Anchors::m_top)->
                    Field("right", &UiTransform2dInterface::Anchors::m_right)->
                    Field("bottom", &UiTransform2dInterface::Anchors::m_bottom);
            }

            if (behaviorContext)
            {
                behaviorContext->Class<UiTransform2dInterface::Anchors>("UiAnchors")
                    ->Constructor<>()
                    ->Constructor<float, float, float, float>()
                    ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                    ->Attribute(AZ::Script::Attributes::ConstructorOverride, &UiAnchorsScriptConstructor)
                    ->Property("left", BehaviorValueProperty(&UiTransform2dInterface::Anchors::m_left))
                    ->Property("top", BehaviorValueProperty(&UiTransform2dInterface::Anchors::m_top))
                    ->Property("right", BehaviorValueProperty(&UiTransform2dInterface::Anchors::m_right))
                    ->Property("bottom", BehaviorValueProperty(&UiTransform2dInterface::Anchors::m_bottom))
                    ->Method("SetLeft", SetAnchorLeft)
                    ->Method("SetTop", SetAnchorTop)
                    ->Method("SetRight", SetAnchorRight)
                    ->Method("SetBottom", SetAnchorBottom)
                    ->Method("SetAnchors", SetAnchors);
            }
        }

        // ParticleColorKeyframe
        {
            if (serializeContext)
            {
                serializeContext->Class<UiParticleEmitterInterface::ParticleColorKeyframe>()
                    ->Field("Time", &UiParticleEmitterInterface::ParticleColorKeyframe::time)
                    ->Field("Color", &UiParticleEmitterInterface::ParticleColorKeyframe::color)
                    ->Field("InTangent", &UiParticleEmitterInterface::ParticleColorKeyframe::inTangent)
                    ->Field("OutTangent", &UiParticleEmitterInterface::ParticleColorKeyframe::outTangent);
            }
        }

        // ParticleFloatKeyframe
        {
            if (serializeContext)
            {
                serializeContext->Class<UiParticleEmitterInterface::ParticleFloatKeyframe>()
                    ->Field("Time", &UiParticleEmitterInterface::ParticleFloatKeyframe::time)
                    ->Field("Multiplier", &UiParticleEmitterInterface::ParticleFloatKeyframe::multiplier)
                    ->Field("InTangent", &UiParticleEmitterInterface::ParticleFloatKeyframe::inTangent)
                    ->Field("OutTangent", &UiParticleEmitterInterface::ParticleFloatKeyframe::outTangent);
            }
        }

        // Offsets
        {
            if (serializeContext)
            {
                serializeContext->Class<UiTransform2dInterface::Offsets>()->
                    Field("left", &UiTransform2dInterface::Offsets::m_left)->
                    Field("top", &UiTransform2dInterface::Offsets::m_top)->
                    Field("right", &UiTransform2dInterface::Offsets::m_right)->
                    Field("bottom", &UiTransform2dInterface::Offsets::m_bottom);
            }

            if (behaviorContext)
            {
                behaviorContext->Class<UiTransform2dInterface::Offsets>("UiOffsets")
                    ->Constructor<>()
                    ->Constructor<float, float, float, float>()
                    ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                    ->Attribute(AZ::Script::Attributes::ConstructorOverride, &UiOffsetsScriptConstructor)
                    ->Property("left", BehaviorValueProperty(&UiTransform2dInterface::Offsets::m_left))
                    ->Property("top", BehaviorValueProperty(&UiTransform2dInterface::Offsets::m_top))
                    ->Property("right", BehaviorValueProperty(&UiTransform2dInterface::Offsets::m_right))
                    ->Property("bottom", BehaviorValueProperty(&UiTransform2dInterface::Offsets::m_bottom))
                    ->Method("SetLeft", SetOffsetLeft)
                    ->Method("SetTop", SetOffsetTop)
                    ->Method("SetRight", SetOffsetRight)
                    ->Method("SetBottom", SetOffsetBottom)
                    ->Method("SetOffsets", SetOffsets);
            }
        }

        // Padding
        {
            if (serializeContext)
            {
                serializeContext->Class<UiLayoutInterface::Padding>()->
                    Field("left", &UiLayoutInterface::Padding::m_left)->
                    Field("top", &UiLayoutInterface::Padding::m_top)->
                    Field("right", &UiLayoutInterface::Padding::m_right)->
                    Field("bottom", &UiLayoutInterface::Padding::m_bottom);
            }

            if (behaviorContext)
            {
                behaviorContext->Class<UiLayoutInterface::Padding>("UiPadding")
                    ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                    ->Property("left", BehaviorValueProperty(&UiLayoutInterface::Padding::m_left))
                    ->Property("right", BehaviorValueProperty(&UiLayoutInterface::Padding::m_right))
                    ->Property("top", BehaviorValueProperty(&UiLayoutInterface::Padding::m_top))
                    ->Property("bottom", BehaviorValueProperty(&UiLayoutInterface::Padding::m_bottom))
                    ->Method("SetLeft", SetPaddingLeft)
                    ->Method("SetTop", SetPaddingTop)
                    ->Method("SetRight", SetPaddingRight)
                    ->Method("SetBottom", SetPaddingBottom)
                    ->Method("SetPadding", SetPadding);
            }
        }

        // UiLayout enums
        {
            if (behaviorContext)
            {
                behaviorContext->Enum<(int)UiLayoutInterface::HorizontalOrder::LeftToRight>("eUiHorizontalOrder_LeftToRight")
                    ->Enum<(int)UiLayoutInterface::HorizontalOrder::RightToLeft>("eUiHorizontalOrder_RightToLeft")
                    ->Enum<(int)UiLayoutInterface::VerticalOrder::TopToBottom>("eUiVerticalOrder_TopToBottom")
                    ->Enum<(int)UiLayoutInterface::VerticalOrder::BottomToTop>("eUiVerticalOrder_BottomToTop");
            }
        }

        // IDraw2d enums
        {
            if (behaviorContext)
            {
                behaviorContext->Enum<(int)IDraw2d::HAlign::Left>("eUiHAlign_Left")
                    ->Enum<(int)IDraw2d::HAlign::Center>("eUiHAlign_Center")
                    ->Enum<(int)IDraw2d::HAlign::Right>("eUiHAlign_Right")
                    ->Enum<(int)IDraw2d::VAlign::Top>("eUiVAlign_Top")
                    ->Enum<(int)IDraw2d::VAlign::Center>("eUiVAlign_Center")
                    ->Enum<(int)IDraw2d::VAlign::Bottom>("eUiVAlign_Bottom");
            }
        }

        if (serializeContext)
        {
            serializeContext->Class<AnimationData>()
                ->Version(1)
                ->Field("SerializeString", &AnimationData::m_serializeData);

            // deprecate old classes that no longer exist
            serializeContext->ClassDeprecate("UiCanvasEditor", AZ::Uuid("{65682E87-B573-435B-88CB-B4C12B71EEEE}"));
            serializeContext->ClassDeprecate("ImageAsset", AZ::Uuid("{138E471A-F3AE-404A-9075-EDC7488C97FC}"));

            // Allow loading FontAssets and CanvasAssets with previous Uuid specializations of AZ_TYPE_INFO_SPECIALIZE
            serializeContext->ClassDeprecate("SimpleAssetReference_FontAsset", AZ::Uuid("{D6342379-A5FA-4B18-B890-702C2FE99A5A}"),
                [](AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& rootElement)
            {
                AZStd::vector<AZ::SerializeContext::DataElementNode> childNodeElements;
                for (int index = 0; index < rootElement.GetNumSubElements(); ++index)
                {
                    childNodeElements.push_back(rootElement.GetSubElement(index));
                }
                // Convert the rootElement now, the existing child DataElmentNodes are now removed
                rootElement.Convert<AzFramework::SimpleAssetReference<LyShine::FontAsset>>(context);
                for (AZ::SerializeContext::DataElementNode& childNodeElement : childNodeElements)
                {
                    rootElement.AddElement(AZStd::move(childNodeElement));
                }
                return true;
            });

            AzFramework::SimpleAssetReference<LyShine::FontAsset>::Register(*serializeContext);
            AzFramework::SimpleAssetReference<LyShine::CanvasAsset>::Register(*serializeContext);

            UiInteractableComponent::Reflect(serializeContext);
        }

        if (behaviorContext)
        {
            UiInteractableComponent::Reflect(behaviorContext);

            behaviorContext->EBus<UiLayoutBus>("UiLayoutBus")
                ->Event("GetHorizontalChildAlignment", &UiLayoutBus::Events::GetHorizontalChildAlignment)
                ->Event("SetHorizontalChildAlignment", &UiLayoutBus::Events::SetHorizontalChildAlignment)
                ->Event("GetVerticalChildAlignment", &UiLayoutBus::Events::GetVerticalChildAlignment)
                ->Event("SetVerticalChildAlignment", &UiLayoutBus::Events::SetVerticalChildAlignment)
                ->Event("GetIgnoreDefaultLayoutCells", &UiLayoutBus::Events::GetIgnoreDefaultLayoutCells)
                ->Event("SetIgnoreDefaultLayoutCells", &UiLayoutBus::Events::SetIgnoreDefaultLayoutCells);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Helper function to VersionConverter to move three state actions from the derived interactable
    // to the interactable base class
    bool MoveToInteractableStateActions(
        AZ::SerializeContext& context,
        AZ::SerializeContext::DataElementNode& srcClassElement,
        const char* stateActionsElementName,
        const char* colorElementName,
        const char* alphaElementName,
        const char* spriteElementName)
    {
        // Note, we can assume that srcClassElement will stay in the same place in memory during this function
        // But the base class (and everything in it) will move around in memory as we remove elements from
        // srcClassElement. So it is improtant not to hold only any indicies or references to the base class.
        int interactableBaseClassIndex = srcClassElement.FindElement(AZ_CRC_CE("BaseClass1"));

        // Add a new element for the state actions.
        int stateActionsIndex = srcClassElement.GetSubElement(interactableBaseClassIndex)
                .AddElement<AZStd::vector<UiInteractableStateAction*> >(context, stateActionsElementName);
        if (stateActionsIndex == -1)
        {
            // Error adding the new sub element
            AZ_Error("Serialization", false, "AddElement failed for %s", stateActionsElementName);
            return false;
        }

        {
            interactableBaseClassIndex = srcClassElement.FindElement(AZ_CRC_CE("BaseClass1"));
            AZ::SerializeContext::DataElementNode& dstClassElement = srcClassElement.GetSubElement(interactableBaseClassIndex);

            stateActionsIndex = dstClassElement.FindElement(AZ_CRC(stateActionsElementName));
            AZ::SerializeContext::DataElementNode& stateActionsNode = dstClassElement.GetSubElement(stateActionsIndex);

            int colorIndex = stateActionsNode.AddElement<UiInteractableStateColor*>(context, "element");
            AZ::SerializeContext::DataElementNode& colorNode = stateActionsNode.GetSubElement(colorIndex);

            if (!LyShine::MoveElement(context, srcClassElement, colorNode, colorElementName, "Color"))
            {
                return false;
            }

            {
                // In the latest version of UiInteractableStateColor the color is an AZ::Color but in the
                // version we are converting from (before UiInteractableStateColor existed) colors were stored
                // as Vector3. Since the UiInteractableStateColor we just created will be at the latest version
                // we need to convert the color to an AZ::Color now.
                // Note that indices will have changed since MoveElement was called.
                interactableBaseClassIndex = srcClassElement.FindElement(AZ_CRC_CE("BaseClass1"));
                AZ::SerializeContext::DataElementNode& dstBaseClassElement = srcClassElement.GetSubElement(interactableBaseClassIndex);
                stateActionsIndex = dstBaseClassElement.FindElement(AZ_CRC(stateActionsElementName));
                AZ::SerializeContext::DataElementNode& dstStateActionsNode = dstBaseClassElement.GetSubElement(stateActionsIndex);
                colorIndex = dstStateActionsNode.FindElement(AZ_CRC_CE("element"));
                AZ::SerializeContext::DataElementNode& dstColorNode = dstStateActionsNode.GetSubElement(colorIndex);

                if (!LyShine::ConvertSubElementFromVector3ToAzColor(context, dstColorNode, "Color"))
                {
                    return false;
                }
            }
        }

        {
            interactableBaseClassIndex = srcClassElement.FindElement(AZ_CRC_CE("BaseClass1"));
            AZ::SerializeContext::DataElementNode& dstClassElement = srcClassElement.GetSubElement(interactableBaseClassIndex);

            stateActionsIndex = dstClassElement.FindElement(AZ_CRC(stateActionsElementName));
            AZ::SerializeContext::DataElementNode& stateActionsNode = dstClassElement.GetSubElement(stateActionsIndex);

            int alphaIndex = stateActionsNode.AddElement<UiInteractableStateAlpha*>(context, "element");
            AZ::SerializeContext::DataElementNode& alphaNode = stateActionsNode.GetSubElement(alphaIndex);
            if (!LyShine::MoveElement(context, srcClassElement, alphaNode, alphaElementName, "Alpha"))
            {
                return false;
            }
        }

        {
            interactableBaseClassIndex = srcClassElement.FindElement(AZ_CRC_CE("BaseClass1"));
            AZ::SerializeContext::DataElementNode& dstClassElement = srcClassElement.GetSubElement(interactableBaseClassIndex);

            stateActionsIndex = dstClassElement.FindElement(AZ_CRC(stateActionsElementName));
            AZ::SerializeContext::DataElementNode& stateActionsNode = dstClassElement.GetSubElement(stateActionsIndex);

            int spriteIndex = stateActionsNode.AddElement<UiInteractableStateSprite*>(context, "element");
            AZ::SerializeContext::DataElementNode& spriteNode = stateActionsNode.GetSubElement(spriteIndex);
            if (!LyShine::MoveElement(context, srcClassElement, spriteNode, spriteElementName, "Sprite"))
            {
                return false;
            }
        }

        // if the field did not exist then we do not report an error
        return true;
    }
}
