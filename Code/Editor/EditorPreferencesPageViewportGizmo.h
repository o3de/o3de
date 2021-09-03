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
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <QIcon>

class CEditorPreferencesPage_ViewportManipulator : public IPreferencesPage
{
public:
    AZ_RTTI(CEditorPreferencesPage_ViewportManipulator, "{14433511-8175-4348-954E-82D903475B06}", IPreferencesPage)

    static void Reflect(AZ::SerializeContext& serialize);

    CEditorPreferencesPage_ViewportManipulator();
    virtual ~CEditorPreferencesPage_ViewportManipulator() = default;

    virtual const char* GetCategory() override
    {
        return "Viewports";
    }

    virtual const char* GetTitle() override
    {
        return "Manipulators";
    }

    virtual QIcon& GetIcon() override;
    virtual void OnApply() override;
    virtual void OnCancel() override
    {
    }

    virtual bool OnQueryCancel() override
    {
        return true;
    }

private:
    void InitializeSettings();

    struct Manipulators
    {
        AZ_TYPE_INFO(Manipulators, "{2974439C-4839-41F6-B526-F317999B9DB9}")

        float m_manipulatorLineBoundWidth;
        float m_manipulatorCircleBoundWidth;
    };

    Manipulators m_manipulators;
    QIcon m_icon;
};
