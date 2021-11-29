/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "Include/IPreferencesPage.h"
#include <AzCore/Math/Color.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <QIcon>

class CEditorPreferencesPage_ViewportGeneral : public IPreferencesPage
{
public:
    AZ_RTTI(CEditorPreferencesPage_ViewportGeneral, "{8511FF7F-F774-47E1-A99B-3DE3A867E403}", IPreferencesPage)

    static void Reflect(AZ::SerializeContext& serialize);

    CEditorPreferencesPage_ViewportGeneral();
    virtual ~CEditorPreferencesPage_ViewportGeneral() = default;

    virtual const char* GetCategory() override;
    virtual const char* GetTitle() override;
    virtual QIcon& GetIcon() override;
    virtual void OnApply() override;
    virtual void OnCancel() override;
    virtual bool OnQueryCancel() override;

private:
    void InitializeSettings();

    struct General
    {
        AZ_TYPE_INFO(General, "{3BAC6F1C-E954-4EDD-95B0-90741B1010E0}")

        bool m_sync2DViews;
        float m_defaultFOV;
        float m_defaultAspectRatio;
        bool m_contextMenuEnabled;
        bool m_stickySelectEnabled;
    };

    struct Display
    {
        AZ_TYPE_INFO(Display, "{F0376933-FA0B-4B58-9DD9-6F6EBC7386CA}")

        bool m_showSafeFrame;
        bool m_highlightSelGeom;
        bool m_highlightSelVegetation;
        bool m_highlightOnMouseOver;
        bool m_hideMouseCursorWhenCaptured;
        int m_dragSquareSize;
        bool m_displayLinks;
        bool m_displayTracks;
        bool m_alwaysShowRadii;
        bool m_showBBoxes;
        bool m_drawEntityLabels;
        bool m_showTriggerBounds;
        bool m_showIcons;
        bool m_distanceScaleIcons;
        bool m_showFrozenHelpers;
        bool m_fillSelectedShapes;
        bool m_showGridGuide;
        bool m_displayDimension;
    };

    struct MapViewport
    {
        AZ_TYPE_INFO(MapViewport, "{FF49D39D-9A92-4605-B20A-C1E72934DD16}")

        bool m_swapXY;
        int m_resolution;
    };

    struct TextLabels
    {
        AZ_TYPE_INFO(TextLabels, "{DD3972ED-5120-4552-8D55-3D3967A29360}")

        bool m_labelsOn;
        float m_labelsDistance;
    };

    struct SelectionPreviewColor
    {
        AZ_TYPE_INFO(SelectionPreviewColor, "{801A5252-B238-4832-BDA4-EAE0D0F98850}")

        AZ::Color m_colorGroupBBox;
        AZ::Color m_colorEntityBBox;
        float m_fBBoxAlpha;
        AZ::Color m_geometryHighlightColor;
        AZ::Color m_solidBrushGeometryColor;
        float m_fgeomAlpha;
        float m_childObjectGeomAlpha;
    };

    General m_general;
    Display m_display;
    MapViewport m_map;
    TextLabels m_textLabels;
    SelectionPreviewColor m_selectionPreviewColor;
    QIcon m_icon;
};
