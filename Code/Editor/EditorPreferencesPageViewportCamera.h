/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "Include/IPreferencesPage.h"

#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/RTTI/TypeInfoSimple.h>
#include <AzCore/RTTI/RTTIMacros.h>

#include <QIcon>

namespace AZ
{
    class SerializeContext;
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

        static void Reflect(AZ::SerializeContext& serialize);

        AZ::Vector3 m_defaultPosition;
        AZ::Vector2 m_defaultPitchYaw;
        float m_speedScale;
        float m_translateSpeed;
        float m_rotateSpeed;
        float m_scrollSpeed;
        float m_dollySpeed;
        float m_panSpeed;
        float m_boostMultiplier;
        float m_rotateSmoothness;
        float m_translateSmoothness;
        float m_defaultOrbitDistance;
        float m_goToPositionDuration;
        bool m_captureCursorLook;
        bool m_orbitYawRotationInverted;
        bool m_panInvertedX;
        bool m_panInvertedY;
        bool m_rotateSmoothing;
        bool m_translateSmoothing;
        bool m_goToPositionInstantly;

        bool GoToPositionDurationReadOnly() const
        {
            return m_goToPositionInstantly;
        }

        bool RotateSmoothingReadOnly() const
        {
            return !m_rotateSmoothing;
        }

        bool TranslateSmoothingReadOnly() const
        {
            return !m_translateSmoothing;
        }

        void Reset();
        void Initialize();

    private:
        bool m_resetButton = false; // required for positioning in edit context, otherwise unused
    };

    struct CameraInputSettings
    {
        AZ_TYPE_INFO(struct CameraInputSettings, "{A250FAD4-662E-4896-B030-D4ED03679377}")

        static void Reflect(AZ::SerializeContext& serialize);

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

        void Reset();
        void Initialize();

    private:
        bool m_resetButton = false; // required for positioning in edit context, otherwise unused
    };

    CameraMovementSettings m_cameraMovementSettings;
    CameraInputSettings m_cameraInputSettings;

    QIcon m_icon;
};
