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

#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>

#include <LyShine/UiBase.h>
#include <LyShine/Bus/UiTransform2dBus.h>

#include "UIVectorPropertyHandlerBase.h"

class PropertyHandlerOffset
    : public UIVectorPropertyHandlerBase< UiTransform2dInterface::Offsets>
{
public:
    AZ_CLASS_ALLOCATOR(PropertyHandlerOffset, AZ::SystemAllocator, 0);

    PropertyHandlerOffset()
        : UIVectorPropertyHandlerBase(4, 2)
    {
    }

    AZ::u32 GetHandlerName(void) const override
    {
        return AZ_CRC("Offset", 0x590acad0);
    }

    bool IsDefaultHandler() const override
    {
        return true;
    }

    void ConsumeAttribute(AzQtComponents::VectorInput* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;
    void WriteGUIValuesIntoProperty(size_t index, AzQtComponents::VectorInput* GUI, UiTransform2dInterface::Offsets& instance, AzToolsFramework::InstanceDataNode* node) override;
    bool ReadValuesIntoGUI(size_t index, AzQtComponents::VectorInput* GUI, const UiTransform2dInterface::Offsets& instance, AzToolsFramework::InstanceDataNode* node)  override;

    void GetLabels(UiTransform2dInterface::Anchors& anchors, AZStd::string* labelsOut);
    void SetLabels(AzQtComponents::VectorInput* ctrl,
        UiTransform2dInterface::Anchors& anchors);
    AZ::EntityId GetParentEntityId(AzToolsFramework::InstanceDataNode* node, size_t index);

    UiTransform2dInterface::Offsets InternalOffsetToDisplayedOffset(UiTransform2dInterface::Offsets internalOffset,
        const UiTransform2dInterface::Anchors& anchors,
        const AZ::Vector2& pivot);
    UiTransform2dInterface::Offsets DisplayedOffsetToInternalOffset(UiTransform2dInterface::Offsets displayedOffset,
        const UiTransform2dInterface::Anchors& anchors,
        const AZ::Vector2& pivot);

    static void Register();
};
