/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include "Include/IPreferencesPage.h"
#include <AzCore/RTTI/TypeInfoSimple.h>
#include <AzCore/RTTI/RTTIMacros.h>
#include <QIcon>

namespace AZ
{
    class SerializeContext;
}

class CEditorPreferencesPage_ViewportDebug
    : public IPreferencesPage
{
public:
    AZ_RTTI(CEditorPreferencesPage_ViewportDebug, "{BD98FC0D-9F07-46AF-A123-BE34EC33E454}", IPreferencesPage)

    static void Reflect(AZ::SerializeContext& serialize);

    CEditorPreferencesPage_ViewportDebug();
    virtual ~CEditorPreferencesPage_ViewportDebug() = default;

    virtual const char* GetCategory() override { return "Viewports"; }
    virtual const char* GetTitle() override { return "Debug"; }
    virtual QIcon& GetIcon() override;
    virtual void OnApply() override;
    virtual void OnCancel() override {}
    virtual bool OnQueryCancel() override { return true; }

private:
    void InitializeSettings();

    struct Profiling
    {
        AZ_TYPE_INFO(Profiling, "{AF413B5A-DDF3-4635-9D8B-1E1A3DF60287}")

        bool m_showMeshStatsOnMouseOver;
    };

    struct Warnings
    {
        AZ_TYPE_INFO(Warnings, "{6CC8A276-24A4-4100-8F7F-7695ABAF6905}")

        float m_warningIconsDrawDistance;
        bool m_showScaleWarnings;
        bool m_showRotationWarnings;
    };

    Profiling m_profiling;
    Warnings m_warnings;
    QIcon m_icon;
};

