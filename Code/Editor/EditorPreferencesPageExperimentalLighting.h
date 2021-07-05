/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include "Include/IPreferencesPage.h"
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/RTTI/RTTI.h>
#include <QIcon>


class CEditorPreferencesPage_ExperimentalLighting
    : public IPreferencesPage
{
public:
    AZ_RTTI(CEditorPreferencesPage_ExperimentalLighting, "{5D65D6A2-22B3-4CB7-A3F7-DC2B5034C9C2}", IPreferencesPage)

    static void Reflect(AZ::SerializeContext& serialize);

    CEditorPreferencesPage_ExperimentalLighting();
    virtual ~CEditorPreferencesPage_ExperimentalLighting() = default;

    virtual const char* GetCategory() override { return "Experimental Features"; }
    virtual const char* GetTitle() override;
    virtual QIcon& GetIcon() override;
    virtual void OnApply() override;
    virtual void OnCancel() override {}
    virtual bool OnQueryCancel() override { return true; }


private:
    void InitializeSettings();

    struct Options
    {
        AZ_TYPE_INFO(Options, "{ED7400E6-3978-4C92-B366-7369E05760FD}")

        bool m_totalIlluminationEnabled;
    };

    Options m_options;
    QIcon m_icon;
};

