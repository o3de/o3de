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
    inline const GraphCanvas::EditorId LANDSCAPE_CANVAS_EDITOR_ID = AZ_CRC_CE("LandscapeCanvasEditor");

    inline constexpr const char* SYSTEM_NAME = "Landscape Canvas";
    inline constexpr const char* MODULE_FILE_EXTENSION = ".landscapecanvas";
    inline constexpr const char* MIME_EVENT_TYPE = "landscapecanvas/node-palette-mime-event";
    inline constexpr const char* SAVE_IDENTIFIER = "LandscapeCanvas";
    inline constexpr const char* CONTEXT_MENU_SAVE_IDENTIFIER = "LandscapeCanvas_ContextMenu";

    // Connection slot names (not translated, only used internally)
    inline constexpr const char* PREVIEW_BOUNDS_SLOT_ID = "PreviewBounds";
    inline constexpr const char* PLACEMENT_BOUNDS_SLOT_ID = "PlacementBounds";
    inline constexpr const char* INBOUND_GRADIENT_SLOT_ID = "InboundGradient";
    inline constexpr const char* OUTBOUND_GRADIENT_SLOT_ID = "OutboundGradient";
    inline constexpr const char* INBOUND_AREA_SLOT_ID = "InboundArea";
    inline constexpr const char* OUTBOUND_AREA_SLOT_ID = "OutboundArea";
    inline constexpr const char* INBOUND_SHAPE_SLOT_ID = "InboundShape";
    inline constexpr const char* INPUT_BOUNDS_SLOT_ID = "InputBounds";
    inline constexpr const char* PIN_TO_SHAPE_SLOT_ID = "PinToShape";
    inline constexpr const char* ENTITY_NAME_SLOT_ID = "EntityName";
    inline constexpr const char* IMAGE_ASSET_SLOT_ID = "ImageAsset";
    inline constexpr const char* OUTPUT_IMAGE_SLOT_ID = "OutputImage";

    // Category title labels
    inline constexpr const char* GRADIENT_TITLE = "Gradient";
    inline constexpr const char* GRADIENT_GENERATOR_TITLE = "Gradient";
    inline constexpr const char* GRADIENT_MODIFIER_TITLE = "Gradient Modifier";
    inline constexpr const char* VEGETATION_AREA_TITLE = "Vegetation Area";
    inline constexpr const char* TERRAIN_TITLE = "Terrain";

    // Connection slot labels
    inline const QString PREVIEW_BOUNDS_SLOT_LABEL = QObject::tr("Preview Bounds");
    inline const QString PLACEMENT_BOUNDS_SLOT_LABEL = QObject::tr("Placement Bounds");
    inline const QString INBOUND_GRADIENT_SLOT_LABEL = QObject::tr("Inbound Gradient");
    inline const QString OUTBOUND_GRADIENT_SLOT_LABEL = QObject::tr("Outbound Gradient");
    inline const QString INBOUND_AREA_SLOT_LABEL = QObject::tr("Inbound Area");
    inline const QString OUTBOUND_AREA_SLOT_LABEL = QObject::tr("Outbound Area");
    inline const QString INBOUND_SHAPE_SLOT_LABEL = QObject::tr("Inbound Shape");
    inline const QString INPUT_BOUNDS_SLOT_LABEL = QObject::tr("Input Bounds");
    inline const QString PIN_TO_SHAPE_SLOT_LABEL = QObject::tr("Pin To Shape");
    inline const QString ENTITY_NAME_SLOT_LABEL = QObject::tr("Entity Name");
    inline const QString IMAGE_ASSET_SLOT_LABEL = QObject::tr("Image Asset");
    inline const QString OUTPUT_IMAGE_SLOT_LABEL = QObject::tr("Output Image");

    // Connection slot descriptions
    inline const QString PREVIEW_BOUNDS_INPUT_SLOT_DESCRIPTION = QObject::tr("Preview Bounds input slot");
    inline const QString PLACEMENT_BOUNDS_INPUT_SLOT_DESCRIPTION = QObject::tr("Placement Bounds input slot");
    inline const QString INBOUND_GRADIENT_INPUT_SLOT_DESCRIPTION = QObject::tr("Inbound Gradient input slot");
    inline const QString OUTBOUND_GRADIENT_OUTPUT_SLOT_DESCRIPTION = QObject::tr("Outbound Gradient output slot");
    inline const QString INBOUND_AREA_INPUT_SLOT_DESCRIPTION = QObject::tr("Inbound Area input slot");
    inline const QString OUTBOUND_AREA_OUTPUT_SLOT_DESCRIPTION = QObject::tr("Outbound Area output slot");
    inline const QString INBOUND_SHAPE_INPUT_SLOT_DESCRIPTION = QObject::tr("Inbound Shape input slot");
    inline const QString INPUT_BOUNDS_INPUT_SLOT_DESCRIPTION = QObject::tr("Input Bounds input slot");
    inline const QString PIN_TO_SHAPE_INPUT_SLOT_DESCRIPTION = QObject::tr("Pin To Shape input slot");
    inline const QString ENTITY_NAME_SLOT_DESCRIPTION = QObject::tr("Vegetation entity name");
    inline const QString IMAGE_ASSET_SLOT_DESCRIPTION = QObject::tr("Path to input Image Asset");
    inline const QString OUTPUT_IMAGE_SLOT_DESCRIPTION = QObject::tr("Output path to Image Asset");
}
