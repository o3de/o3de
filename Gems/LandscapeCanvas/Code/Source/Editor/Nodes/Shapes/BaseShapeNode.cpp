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

// Qt
#include <QObject>

// AZ
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

// Graph Model
#include <GraphModel/Integration/Helpers.h>
#include <GraphModel/Model/Slot.h>

// Landscape Canvas
#include "BaseShapeNode.h"
#include <Editor/Core/GraphContext.h>

namespace LandscapeCanvas
{
    const QString BaseShapeNode::SHAPE_CATEGORY_TITLE = QObject::tr("Shape");
    const QString BaseShapeNode::BOUNDS_SLOT_LABEL = QObject::tr("Bounds");
    const QString BaseShapeNode::BOUNDS_OUTPUT_SLOT_DESCRIPTION = QObject::tr("Bounds output slot");
    const char* BaseShapeNode::BOUNDS_SLOT_ID = "Bounds";

    void BaseShapeNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<BaseShapeNode, BaseNode>()
                ->Version(0)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<BaseShapeNode>("BaseShapeNode", "")->
                    ClassElement(AZ::Edit::ClassElements::EditorData, "")->
                    Attribute(GraphModelIntegration::Attributes::TitlePaletteOverride, "ShapeNodeTitlePalette")
                    ;
            }
        }
    }

    BaseShapeNode::BaseShapeNode(GraphModel::GraphPtr graph)
        : BaseNode(graph)
    {
        RegisterSlots();
        CreateSlotData();
    }

    void BaseShapeNode::RegisterSlots()
    {
        CreateEntityNameSlot();

        GraphModel::DataTypePtr dataType = GraphContext::GetInstance()->GetDataType(LandscapeCanvasDataTypeEnum::Bounds);
        RegisterSlot(GraphModel::SlotDefinition::CreateOutputData(
            BOUNDS_SLOT_ID,
            BOUNDS_SLOT_LABEL.toUtf8().constData(),
            dataType,
            BOUNDS_OUTPUT_SLOT_DESCRIPTION.toUtf8().constData()));
    }
} // namespace LandscapeCanvas
