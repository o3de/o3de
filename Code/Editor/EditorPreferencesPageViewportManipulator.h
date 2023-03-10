/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "Include/IPreferencesPage.h"
#include <AzCore/Math/Vector3.h>
#include <AzCore/RTTI/TypeInfoSimple.h>
#include <AzCore/RTTI/RTTIMacros.h>
#include <QIcon>

namespace AZ
{
    class SerializeContext;
}

class CEditorPreferencesPage_ViewportManipulator : public IPreferencesPage
{
public:
    AZ_RTTI(CEditorPreferencesPage_ViewportManipulator, "{14433511-8175-4348-954E-82D903475B06}", IPreferencesPage)

    static void Reflect(AZ::SerializeContext& serialize);

    CEditorPreferencesPage_ViewportManipulator();
    virtual ~CEditorPreferencesPage_ViewportManipulator() = default;

    const char* GetCategory() override;
    const char* GetTitle() override;
    QIcon& GetIcon() override;
    void OnApply() override;
    void OnCancel() override;
    bool OnQueryCancel() override;

private:
    void InitializeSettings();

    struct Manipulators
    {
        AZ_TYPE_INFO(Manipulators, "{2974439C-4839-41F6-B526-F317999B9DB9}")

        float m_manipulatorLineBoundWidth = 0.0f;
        float m_manipulatorCircleBoundWidth = 0.0f;
        float m_linearManipulatorAxisLength = 0.0f;
        float m_planarManipulatorAxisLength = 0.0f;
        float m_surfaceManipulatorRadius = 0.0f;
        float m_surfaceManipulatorOpacity = 0.0f;
        float m_linearManipulatorConeLength = 0.0f;
        float m_linearManipulatorConeRadius = 0.0f;
        float m_scaleManipulatorBoxHalfExtent = 0.0f;
        float m_rotationManipulatorRadius = 0.0f;
        float m_manipulatorViewBaseScale = 0.0f;
        bool m_flipManipulatorAxesTowardsView = false;
    };

    Manipulators m_manipulators;
    QIcon m_icon;
};
