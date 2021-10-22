/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "Include/IPreferencesPage.h"
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <QIcon>

inline AZ::Crc32 EditorPropertyVisibility(const bool enabled)
{
    return enabled ? AZ::Edit::PropertyVisibility::Show : AZ::Edit::PropertyVisibility::Hide;
}

class CEditorPreferencesPage_ViewportCamera : public IPreferencesPage
{
public:
    AZ_RTTI(CEditorPreferencesPage_ViewportCamera, "{BC593332-7EAF-4171-8A35-1C5DE5B40909}", IPreferencesPage)

    static void Reflect(AZ::SerializeContext& serialize);

    CEditorPreferencesPage_ViewportCamera();
    virtual ~CEditorPreferencesPage_ViewportCamera() = default;

    const char* GetCategory() override;
    const char* GetTitle() override;
    QIcon& GetIcon() override;
    void OnApply() override;
    void OnCancel() override;
    bool OnQueryCancel() override;

private:
    void InitializeSettings();

    struct CameraMovementSettings
    {
        AZ_TYPE_INFO(CameraMovementSettings, "{60B8C07E-5F48-4171-A50B-F45558B5CCA1}")

        float m_translateSpeed;
        float m_rotateSpeed;
        float m_scrollSpeed;
        float m_dollySpeed;
        float m_panSpeed;
        float m_boostMultiplier;
        float m_rotateSmoothness;
        bool m_rotateSmoothing;
        float m_translateSmoothness;
        bool m_translateSmoothing;
        bool m_captureCursorLook;
        bool m_orbitYawRotationInverted;
        bool m_panInvertedX;
        bool m_panInvertedY;
        float m_defaultCameraPositionX;
        float m_defaultCameraPositionY;
        float m_defaultCameraPositionZ;

        AZ::Crc32 RotateSmoothingVisibility() const
        {
            return EditorPropertyVisibility(m_rotateSmoothing);
        }

        AZ::Crc32 TranslateSmoothingVisibility() const
        {
            return EditorPropertyVisibility(m_translateSmoothing);
        }
    };

    struct CameraInputSettings
    {
        AZ_TYPE_INFO(struct CameraInputSettings, "{A250FAD4-662E-4896-B030-D4ED03679377}")

        AZStd::string m_translateForwardChannelId;
        AZStd::string m_translateBackwardChannelId;
        AZStd::string m_translateLeftChannelId;
        AZStd::string m_translateRightChannelId;
        AZStd::string m_translateUpChannelId;
        AZStd::string m_translateDownChannelId;
        AZStd::string m_boostChannelId;
        AZStd::string m_orbitChannelId;
        AZStd::string m_freeLookChannelId;
        AZStd::string m_freePanChannelId;
        AZStd::string m_orbitLookChannelId;
        AZStd::string m_orbitDollyChannelId;
        AZStd::string m_orbitPanChannelId;
        AZStd::string m_focusChannelId;
    };

    CameraMovementSettings m_cameraMovementSettings;
    CameraInputSettings m_cameraInputSettings;

    QIcon m_icon;
};
