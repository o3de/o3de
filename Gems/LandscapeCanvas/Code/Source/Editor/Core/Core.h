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

// Qt
#include <QObject>
#include <QString>

// Graph Canvas
#include <GraphCanvas/Editor/EditorTypes.h>

namespace LandscapeCanvas
{
    static const GraphCanvas::EditorId LANDSCAPE_CANVAS_EDITOR_ID = AZ_CRC("LandscapeCanvasEditor", 0x57f5535f);
    static const char* SYSTEM_NAME = "Landscape Canvas";
    static const char* MODULE_FILE_EXTENSION = ".landscapecanvas";
    static const char* MIME_EVENT_TYPE = "landscapecanvas/node-palette-mime-event";
    static const char* SAVE_IDENTIFIER = "LandscapeCanvas";
    static const char* CONTEXT_MENU_SAVE_IDENTIFIER = "LandscapeCanvas_ContextMenu";

    // Connection slot names (not translated, only used internally)
    static const char* PREVIEW_BOUNDS_SLOT_ID = "PreviewBounds";
    static const char* PLACEMENT_BOUNDS_SLOT_ID = "PlacementBounds";
    static const char* INBOUND_GRADIENT_SLOT_ID = "InboundGradient";
    static const char* OUTBOUND_GRADIENT_SLOT_ID = "OutboundGradient";
    static const char* INBOUND_AREA_SLOT_ID = "InboundArea";
    static const char* OUTBOUND_AREA_SLOT_ID = "OutboundArea";
    static const char* INBOUND_SHAPE_SLOT_ID = "InboundShape";
    static const char* PIN_TO_SHAPE_SLOT_ID = "PinToShape";
    static const char* ENTITY_NAME_SLOT_ID = "EntityName";

    // Category title labels
    static const QString GRADIENT_TITLE = QObject::tr("Gradient");
    static const QString GRADIENT_GENERATOR_TITLE = QObject::tr("Gradient");
    static const QString GRADIENT_MODIFIER_TITLE = QObject::tr("Gradient Modifier");
    static const QString VEGETATION_AREA_TITLE = QObject::tr("Vegetation Area");

    // Connection slot labels
    static const QString PREVIEW_BOUNDS_SLOT_LABEL = QObject::tr("Preview Bounds");
    static const QString PLACEMENT_BOUNDS_SLOT_LABEL = QObject::tr("Placement Bounds");
    static const QString INBOUND_GRADIENT_SLOT_LABEL = QObject::tr("Inbound Gradient");
    static const QString OUTBOUND_GRADIENT_SLOT_LABEL = QObject::tr("Outbound Gradient");
    static const QString INBOUND_AREA_SLOT_LABEL = QObject::tr("Inbound Area");
    static const QString OUTBOUND_AREA_SLOT_LABEL = QObject::tr("Outbound Area");
    static const QString INBOUND_SHAPE_SLOT_LABEL = QObject::tr("Inbound Shape");
    static const QString PIN_TO_SHAPE_SLOT_LABEL = QObject::tr("Pin To Shape");
    static const QString ENTITY_NAME_SLOT_LABEL = QObject::tr("Entity Name");

    // Connection slot descriptions
    static const QString PREVIEW_BOUNDS_INPUT_SLOT_DESCRIPTION = QObject::tr("Preview Bounds input slot");
    static const QString PLACEMENT_BOUNDS_INPUT_SLOT_DESCRIPTION = QObject::tr("Placement Bounds input slot");
    static const QString INBOUND_GRADIENT_INPUT_SLOT_DESCRIPTION = QObject::tr("Inbound Gradient input slot");
    static const QString OUTBOUND_GRADIENT_OUTPUT_SLOT_DESCRIPTION = QObject::tr("Outbound Gradient output slot");
    static const QString INBOUND_AREA_INPUT_SLOT_DESCRIPTION = QObject::tr("Inbound Area input slot");
    static const QString OUTBOUND_AREA_OUTPUT_SLOT_DESCRIPTION = QObject::tr("Outbound Area output slot");
    static const QString INBOUND_SHAPE_INPUT_SLOT_DESCRIPTION = QObject::tr("Inbound Shape input slot");
    static const QString PIN_TO_SHAPE_INPUT_SLOT_DESCRIPTION = QObject::tr("Pin To Shape input slot");
    static const QString ENTITY_NAME_SLOT_DESCRIPTION = QObject::tr("Vegetation entity name");
}
