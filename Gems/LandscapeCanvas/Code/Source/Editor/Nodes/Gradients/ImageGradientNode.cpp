/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// Qt
#include <QObject>

// AZ
#include <AzCore/Serialization/SerializeContext.h>

// Landscape Canvas
#include "ImageGradientNode.h"
#include <Editor/Core/GraphContext.h>

namespace LandscapeCanvas
{
    void ImageGradientNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<ImageGradientNode, BaseGradientNode>()
                ->Version(0)
                ;
        }
    }

    const QString ImageGradientNode::TITLE = QObject::tr("Image");

    ImageGradientNode::ImageGradientNode(GraphModel::GraphPtr graph)
        : BaseGradientNode(graph)
    {
        RegisterSlots();
        CreateSlotData();
    }

    void ImageGradientNode::RegisterSlots()
    {
        BaseGradientNode::RegisterSlots();

        // Image Gradient has an additional input slot for the image asset
        GraphModel::DataTypePtr pathDataType = GetGraphContext()->GetDataType(LandscapeCanvasDataTypeEnum::Path);

        RegisterSlot(GraphModel::SlotDefinition::CreateInputData(
            IMAGE_ASSET_SLOT_ID,
            IMAGE_ASSET_SLOT_LABEL.toUtf8().constData(),
            { pathDataType },
            pathDataType->GetDefaultValue(),
            IMAGE_ASSET_SLOT_DESCRIPTION.toUtf8().constData()));
    }
} // namespace LandscapeCanvas
