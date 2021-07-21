/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// AZ
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

// Graph Model
#include <GraphModel/Integration/Helpers.h>

// Landscape Canvas
#include "BaseAreaModifierNode.h"

namespace LandscapeCanvas
{
    const QString BaseAreaModifierNode::INBOUND_GRADIENT_X_SLOT_LABEL = QObject::tr("Inbound Gradient X");
    const QString BaseAreaModifierNode::INBOUND_GRADIENT_Y_SLOT_LABEL = QObject::tr("Inbound Gradient Y");
    const QString BaseAreaModifierNode::INBOUND_GRADIENT_Z_SLOT_LABEL = QObject::tr("Inbound Gradient Z");
    const QString BaseAreaModifierNode::INBOUND_GRADIENT_X_INPUT_SLOT_DESCRIPTION = QObject::tr("Inbound Gradient X input slot");
    const QString BaseAreaModifierNode::INBOUND_GRADIENT_Y_INPUT_SLOT_DESCRIPTION = QObject::tr("Inbound Gradient Y input slot");
    const QString BaseAreaModifierNode::INBOUND_GRADIENT_Z_INPUT_SLOT_DESCRIPTION = QObject::tr("Inbound Gradient Z input slot");
    const char* BaseAreaModifierNode::INBOUND_GRADIENT_X_SLOT_ID = "InboundGradientX";
    const char* BaseAreaModifierNode::INBOUND_GRADIENT_Y_SLOT_ID = "InboundGradientY";
    const char* BaseAreaModifierNode::INBOUND_GRADIENT_Z_SLOT_ID = "InboundGradientZ";

    void BaseAreaModifierNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<BaseAreaModifierNode, BaseNode>()
                ->Version(0)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<BaseAreaModifierNode>("BaseAreaModifierNode", "")->
                    ClassElement(AZ::Edit::ClassElements::EditorData, "")->
                    Attribute(GraphModelIntegration::Attributes::TitlePaletteOverride, "VegetationAreaNodeTitlePalette")
                    ;
            }
        }
    }

    BaseAreaModifierNode::BaseAreaModifierNode(GraphModel::GraphPtr graph)
        : BaseNode(graph)
    {
    }
} // namespace LandscapeCanvas
