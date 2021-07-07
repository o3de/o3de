/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include "Include/IPreferencesPage.h"
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Math/Vector3.h>
#include <QIcon>


class CEditorPreferencesPage_ViewportGizmo
    : public IPreferencesPage
{
public:
    AZ_RTTI(CEditorPreferencesPage_ViewportGizmo, "{14433511-8175-4348-954E-82D903475B06}", IPreferencesPage)

    static void Reflect(AZ::SerializeContext& serialize);

    CEditorPreferencesPage_ViewportGizmo();
    virtual ~CEditorPreferencesPage_ViewportGizmo() = default;

    virtual const char* GetCategory() override { return "Viewports"; }
    virtual const char* GetTitle() override { return "Gizmos"; }
    virtual QIcon& GetIcon() override;
    virtual void OnApply() override;
    virtual void OnCancel() override {}
    virtual bool OnQueryCancel() override { return true; }

private:
    void InitializeSettings();

    struct AxisGizmo
    {
        AZ_TYPE_INFO(AxisGizmo, "{7D90D60E-996B-4F54-8748-B26EFA781EE2}")

        float m_size;
        bool m_text;
        int m_maxCount;
    };

    struct Helpers
    {
        AZ_TYPE_INFO(Helpers, "{EC99922E-F61C-4AA0-9A51-630E09AB55AA}")

        float m_helpersGlobalScale;
        float m_tagpointScaleMulti;
        float m_rulerSphereScale;
        float m_rulerSphereTrans;
    };

    AxisGizmo m_axisGizmo;
    Helpers m_helpers;
    QIcon m_icon;
};


