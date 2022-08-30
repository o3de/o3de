/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

    static constexpr const char* SYSTEM_NAME = "Landscape Canvas";
    static constexpr const char* MODULE_FILE_EXTENSION = ".landscapecanvas";
    static constexpr const char* MIME_EVENT_TYPE = "landscapecanvas/node-palette-mime-event";
    static constexpr const char* SAVE_IDENTIFIER = "LandscapeCanvas";
    static constexpr const char* CONTEXT_MENU_SAVE_IDENTIFIER = "LandscapeCanvas_ContextMenu";

    // Connection slot names (not translated, only used internally)
    static constexpr const char* PREVIEW_BOUNDS_SLOT_ID = "PreviewBounds";
    static constexpr const char* PLACEMENT_BOUNDS_SLOT_ID = "PlacementBounds";
    static constexpr const char* INBOUND_GRADIENT_SLOT_ID = "InboundGradient";
    static constexpr const char* OUTBOUND_GRADIENT_SLOT_ID = "OutboundGradient";
    static constexpr const char* INBOUND_AREA_SLOT_ID = "InboundArea";
    static constexpr const char* OUTBOUND_AREA_SLOT_ID = "OutboundArea";
    static constexpr const char* INBOUND_SHAPE_SLOT_ID = "InboundShape";
    static constexpr const char* INPUT_BOUNDS_SLOT_ID = "InputBounds";
    static constexpr const char* PIN_TO_SHAPE_SLOT_ID = "PinToShape";
    static constexpr const char* ENTITY_NAME_SLOT_ID = "EntityName";
    static constexpr const char* IMAGE_ASSET_SLOT_ID = "ImageAsset";
    static constexpr const char* OUTPUT_IMAGE_SLOT_ID = "OutputImage";

    // Category title labels
    static const QString GRADIENT_TITLE = QObject::tr("Gradient");
    static const QString GRADIENT_GENERATOR_TITLE = QObject::tr("Gradient");
    static const QString GRADIENT_MODIFIER_TITLE = QObject::tr("Gradient Modifier");
    static const QString VEGETATION_AREA_TITLE = QObject::tr("Vegetation Area");
    static const QString TERRAIN_TITLE = QObject::tr("Terrain");

    // Connection slot labels
    static const QString PREVIEW_BOUNDS_SLOT_LABEL = QObject::tr("Preview Bounds");
    static const QString PLACEMENT_BOUNDS_SLOT_LABEL = QObject::tr("Placement Bounds");
    static const QString INBOUND_GRADIENT_SLOT_LABEL = QObject::tr("Inbound Gradient");
    static const QString OUTBOUND_GRADIENT_SLOT_LABEL = QObject::tr("Outbound Gradient");
    static const QString INBOUND_AREA_SLOT_LABEL = QObject::tr("Inbound Area");
    static const QString OUTBOUND_AREA_SLOT_LABEL = QObject::tr("Outbound Area");
    static const QString INBOUND_SHAPE_SLOT_LABEL = QObject::tr("Inbound Shape");
    static const QString INPUT_BOUNDS_SLOT_LABEL = QObject::tr("Input Bounds");
    static const QString PIN_TO_SHAPE_SLOT_LABEL = QObject::tr("Pin To Shape");
    static const QString ENTITY_NAME_SLOT_LABEL = QObject::tr("Entity Name");
    static const QString IMAGE_ASSET_SLOT_LABEL = QObject::tr("Image Asset");
    static const QString OUTPUT_IMAGE_SLOT_LABEL = QObject::tr("Output Image");

    // Connection slot descriptions
    static const QString PREVIEW_BOUNDS_INPUT_SLOT_DESCRIPTION = QObject::tr("Preview Bounds input slot");
    static const QString PLACEMENT_BOUNDS_INPUT_SLOT_DESCRIPTION = QObject::tr("Placement Bounds input slot");
    static const QString INBOUND_GRADIENT_INPUT_SLOT_DESCRIPTION = QObject::tr("Inbound Gradient input slot");
    static const QString OUTBOUND_GRADIENT_OUTPUT_SLOT_DESCRIPTION = QObject::tr("Outbound Gradient output slot");
    static const QString INBOUND_AREA_INPUT_SLOT_DESCRIPTION = QObject::tr("Inbound Area input slot");
    static const QString OUTBOUND_AREA_OUTPUT_SLOT_DESCRIPTION = QObject::tr("Outbound Area output slot");
    static const QString INBOUND_SHAPE_INPUT_SLOT_DESCRIPTION = QObject::tr("Inbound Shape input slot");
    static const QString INPUT_BOUNDS_INPUT_SLOT_DESCRIPTION = QObject::tr("Input Bounds input slot");
    static const QString PIN_TO_SHAPE_INPUT_SLOT_DESCRIPTION = QObject::tr("Pin To Shape input slot");
    static const QString ENTITY_NAME_SLOT_DESCRIPTION = QObject::tr("Vegetation entity name");
    static const QString IMAGE_ASSET_SLOT_DESCRIPTION = QObject::tr("Path to input Image Asset");
    static const QString OUTPUT_IMAGE_SLOT_DESCRIPTION = QObject::tr("Output path to Image Asset");
}
