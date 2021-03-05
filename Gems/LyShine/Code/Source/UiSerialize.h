/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#pragma once

#include <LyShine/IDraw2d.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <LyShine/Bus/UiTransform2dBus.h>
#include <LyShine/Bus/UiLayoutBus.h>

namespace UiSerialize
{
    //! Define the Cry and UI types for the AZ Serialize system
    void ReflectUiTypes(AZ::ReflectContext* context);

    //! Wrapper class for prefab file. This allows us to make changes to what the top
    //! level objects are in the prefab file and do some conversion
    //! NOTE: This is only used for old pre-slices UI prefabs
    class PrefabFileObject
    {
    public:
        virtual ~PrefabFileObject() { }
        AZ_CLASS_ALLOCATOR(PrefabFileObject, AZ::SystemAllocator, 0);
        AZ_RTTI(PrefabFileObject, "{C264CC6F-E50C-4813-AAE6-F7AB0B1774D0}");
        static bool VersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);
        AZ::EntityId m_rootEntityId;
        AZStd::vector<AZ::Entity*> m_entities;
    };

    //! Wrapper class for animation system data file. This allows us to use the old Cry
    //! serialize for the animation data
    class AnimationData
    {
    public:
        virtual ~AnimationData() { }
        AZ_CLASS_ALLOCATOR(AnimationData, AZ::SystemAllocator, 0);
        AZ_RTTI(AnimationData, "{FDC58CF7-8109-48F2-8D5D-BCBAF774ABB7}");
        AZStd::string m_serializeData;
    };

    //! Helper function for version conversion
    bool MoveToInteractableStateActions(
        AZ::SerializeContext& context,
        AZ::SerializeContext::DataElementNode& srcClassElement,
        const char* stateActionsElementName,
        const char* colorElementName,
        const char* alphaElementName,
        const char* spriteElementName);

    //! Setters for anchors
    void SetAnchorLeft(UiTransform2dInterface::Anchors* anchor, float left);
    void SetAnchorTop(UiTransform2dInterface::Anchors* anchor, float top);
    void SetAnchorRight(UiTransform2dInterface::Anchors* anchor, float right);
    void SetAnchorBottom(UiTransform2dInterface::Anchors* anchor, float bottom);
    void SetAnchors(UiTransform2dInterface::Anchors* anchor, float left, float top, float right, float bottom);

    //! Setters for offsets
    void SetOffsetLeft(UiTransform2dInterface::Offsets* offset, float left);
    void SetOffsetTop(UiTransform2dInterface::Offsets* offset, float top);
    void SetOffsetRight(UiTransform2dInterface::Offsets* offset, float right);
    void SetOffsetBottom(UiTransform2dInterface::Offsets* offset, float bottom);
    void SetOffsets(UiTransform2dInterface::Offsets* offset, float left, float top, float right, float bottom);

    //! Setters for padding
    void SetPaddingLeft(UiLayoutInterface::Padding* anchor, int left);
    void SetPaddingTop(UiLayoutInterface::Padding* anchor, int top);
    void SetPaddingRight(UiLayoutInterface::Padding* anchor, int right);
    void SetPaddingBottom(UiLayoutInterface::Padding* anchor, int bottom);
    void SetPadding(UiLayoutInterface::Padding* anchor, int left, int top, int right, int bottom);
}
