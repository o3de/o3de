/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// Description : CViewportTitleDlg implementation file

#if !defined(Q_MOC_RUN)
#include "EditorDefs.h"

#include "ViewportTitleDlg.h"

// Qt
#include <QCheckBox>
#include <QLabel>
#include <QInputDialog>

#include <AtomLyIntegration/AtomViewportDisplayInfo/AtomViewportInfoDisplayBus.h>

// Editor
#include "CustomAspectRatioDlg.h"
#include "CustomResolutionDlg.h"
#include "DisplaySettings.h"
#include "EditorViewportSettings.h"
#include "GameEngine.h"
#include "MainWindow.h"
#include "MathConversion.h"
#include "Settings.h"
#include "UsedResources.h"
#include "ViewPane.h"

#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/std/algorithm.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzToolsFramework/ActionManager/Menu/MenuManagerInterface.h>
#include <AzToolsFramework/ActionManager/Menu/MenuManagerInternalInterface.h>
#include <AzToolsFramework/Editor/ActionManagerUtils.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>
#include <AzToolsFramework/Viewport/ViewportSettings.h>
#include <AzToolsFramework/ViewportSelection/EditorTransformComponentSelectionRequestBus.h>
#include <AzQtComponents/Components/Widgets/CheckBox.h>
#include <Editor/EditorSettingsAPIBus.h>
#include <EditorModeFeedback/EditorStateRequestsBus.h>

#include <LmbrCentral/Audio/AudioSystemComponentBus.h>

AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
#include "ui_ViewportTitleDlg.h"
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
#endif //! defined(Q_MOC_RUN)

// CViewportTitleDlg dialog

namespace
{
    class CViewportTitleDlgDisplayInfoHelper
        : public QObject
        , public AZ::AtomBridge::AtomViewportInfoDisplayNotificationBus::Handler
    {
        Q_OBJECT

    public:
        CViewportTitleDlgDisplayInfoHelper(CViewportTitleDlg* parent)
            : QObject(parent)
        {
            AZ::AtomBridge::AtomViewportInfoDisplayNotificationBus::Handler::BusConnect();
        }

    signals:
        void ViewportInfoStatusUpdated(int newIndex);

    private:
        void OnViewportInfoDisplayStateChanged(AZ::AtomBridge::ViewportInfoDisplayState state) override
        {
            emit ViewportInfoStatusUpdated(aznumeric_cast<int>(state));
        }
    };
} // end anonymous namespace

CViewportTitleDlg::CViewportTitleDlg(QWidget* pParent)
    : QWidget(pParent)
    , m_ui(new Ui::ViewportTitleDlg)
{
    m_prevMoveSpeedScale = 0;
    m_pViewPane = nullptr;

    if (gSettings.bMuteAudio)
    {
        LmbrCentral::AudioSystemComponentRequestBus::Broadcast(&LmbrCentral::AudioSystemComponentRequestBus::Events::GlobalMuteAudio);
    }
    else
    {
        LmbrCentral::AudioSystemComponentRequestBus::Broadcast(&LmbrCentral::AudioSystemComponentRequestBus::Events::GlobalUnmuteAudio);
    }

    OnInitDialog();
}

CViewportTitleDlg::~CViewportTitleDlg()
{
}

//////////////////////////////////////////////////////////////////////////
void CViewportTitleDlg::SetViewPane(CLayoutViewPane* pViewPane)
{
    if (m_pViewPane)
    {
        m_pViewPane->disconnect(this);
    }

    m_pViewPane = pViewPane;

    if (m_pViewPane)
    {
        connect(this, &QWidget::customContextMenuRequested, m_pViewPane, &CLayoutViewPane::ShowTitleMenu);
    }
}

//////////////////////////////////////////////////////////////////////////
void CViewportTitleDlg::OnInitDialog()
{
}

//////////////////////////////////////////////////////////////////////////
void CViewportTitleDlg::SetTitle(const QString& title)
{
    m_title = title;
}

//////////////////////////////////////////////////////////////////////////
void CViewportTitleDlg::OnMaximize()
{
    if (m_pViewPane)
    {
        m_pViewPane->ToggleMaximize();
    }
}

void CViewportTitleDlg::SetNoViewportInfo()
{
    AZ::AtomBridge::AtomViewportInfoDisplayRequestBus::Broadcast(
        &AZ::AtomBridge::AtomViewportInfoDisplayRequestBus::Events::SetDisplayState, AZ::AtomBridge::ViewportInfoDisplayState::NoInfo);
}

void CViewportTitleDlg::SetNormalViewportInfo()
{
    AZ::AtomBridge::AtomViewportInfoDisplayRequestBus::Broadcast(
        &AZ::AtomBridge::AtomViewportInfoDisplayRequestBus::Events::SetDisplayState, AZ::AtomBridge::ViewportInfoDisplayState::NormalInfo);
}

void CViewportTitleDlg::SetFullViewportInfo()
{
    AZ::AtomBridge::AtomViewportInfoDisplayRequestBus::Broadcast(
        &AZ::AtomBridge::AtomViewportInfoDisplayRequestBus::Events::SetDisplayState, AZ::AtomBridge::ViewportInfoDisplayState::FullInfo);
}

void CViewportTitleDlg::SetCompactViewportInfo()
{
    AZ::AtomBridge::AtomViewportInfoDisplayRequestBus::Broadcast(
        &AZ::AtomBridge::AtomViewportInfoDisplayRequestBus::Events::SetDisplayState, AZ::AtomBridge::ViewportInfoDisplayState::CompactInfo);
}

//////////////////////////////////////////////////////////////////////////
void CViewportTitleDlg::OnToggleDisplayInfo()
{
    AZ::AtomBridge::ViewportInfoDisplayState state = AZ::AtomBridge::ViewportInfoDisplayState::NoInfo;
    AZ::AtomBridge::AtomViewportInfoDisplayRequestBus::BroadcastResult(
        state, &AZ::AtomBridge::AtomViewportInfoDisplayRequestBus::Events::GetDisplayState);
    state = aznumeric_cast<AZ::AtomBridge::ViewportInfoDisplayState>(
        (aznumeric_cast<int>(state) + 1) % aznumeric_cast<int>(AZ::AtomBridge::ViewportInfoDisplayState::Invalid));
    // SetDisplayState will fire OnViewportInfoDisplayStateChanged and notify us, no need to call UpdateDisplayInfo.
    AZ::AtomBridge::AtomViewportInfoDisplayRequestBus::Broadcast(
        &AZ::AtomBridge::AtomViewportInfoDisplayRequestBus::Events::SetDisplayState, state);
}

//////////////////////////////////////////////////////////////////////////
void CViewportTitleDlg::OnMenuFOVCustom()
{
    bool ok;
    int fovDegrees = QInputDialog::getInt(this, tr("Custom FOV"), QString(), 60, 1, 120, 1, &ok);

    if (ok)
    {
        m_pViewPane->SetViewportFOV(aznumeric_cast<float>(fovDegrees));
        // Update the custom presets.
        const QString text = QString::number(fovDegrees);
        UpdateCustomPresets(text, m_customFOVPresets);
        SaveCustomPresets("FOVPresets", "FOVPreset", m_customFOVPresets);
    }
}

//////////////////////////////////////////////////////////////////////////
void CViewportTitleDlg::OnMenuAspectRatioCustom()
{
    const QRect viewportRect = m_pViewPane->GetViewport()->rect();
    const unsigned int width = viewportRect.width();
    const unsigned int height = viewportRect.height();

    int whGCD = AZ::GetGCD(width, height);
    CCustomAspectRatioDlg aspectRatioInputDialog(width / whGCD, height / whGCD, this);

    if (aspectRatioInputDialog.exec() == QDialog::Accepted)
    {
        const unsigned int aspectX = aspectRatioInputDialog.GetX();
        const unsigned int aspectY = aspectRatioInputDialog.GetY();

        m_pViewPane->SetAspectRatio(aspectX, aspectY);

        // Update the custom presets.
        UpdateCustomPresets(QString::fromLatin1("%1:%2").arg(aspectX).arg(aspectY), m_customAspectRatioPresets);
        SaveCustomPresets("AspectRatioPresets", "AspectRatioPreset", m_customAspectRatioPresets);
    }
}

//////////////////////////////////////////////////////////////////////////
void CViewportTitleDlg::OnMenuResolutionCustom()
{
    const QRect rectViewport = m_pViewPane->GetViewport()->rect();
    CCustomResolutionDlg resDlg(rectViewport.width(), rectViewport.height(), parentWidget());
    if (resDlg.exec() == QDialog::Accepted)
    {
        m_pViewPane->ResizeViewport(resDlg.GetWidth(), resDlg.GetHeight());
        // Update the custom presets.
        const QString text = QString::fromLatin1("%1 x %2").arg(resDlg.GetWidth()).arg(resDlg.GetHeight());
        UpdateCustomPresets(text, m_customResPresets);
        SaveCustomPresets("ResPresets", "ResPreset", m_customResPresets);
    }
}

void CViewportTitleDlg::LoadCustomPresets(const QString& section, const QString& keyName, QStringList& outCustompresets)
{
    QSettings settings("O3DE", "O3DE"); // Temporary solution until we have the global Settings class.
    settings.beginGroup(section);
    outCustompresets = settings.value(keyName).toStringList();
    settings.endGroup();
}

void CViewportTitleDlg::SaveCustomPresets(const QString& section, const QString& keyName, const QStringList& custompresets)
{
    QSettings settings("O3DE", "O3DE"); // Temporary solution until we have the global Settings class.
    settings.beginGroup(section);
    settings.setValue(keyName, custompresets);
    settings.endGroup();
}

void CViewportTitleDlg::UpdateCustomPresets(const QString& text, QStringList& custompresets)
{
    custompresets.removeAll(text);
    custompresets.push_front(text);
    if (custompresets.size() > MAX_NUM_CUSTOM_PRESETS)
    {
        custompresets.erase(custompresets.begin() + MAX_NUM_CUSTOM_PRESETS, custompresets.end()); // QList doesn't have resize()
    }
}

bool CViewportTitleDlg::eventFilter(QObject* object, QEvent* event)
{
    bool consumeEvent = false;

    // These events are forwarded from the toolbar that took ownership of our widgets
    if (event->type() == QEvent::MouseButtonDblClick)
    {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton)
        {
            OnMaximize();
            consumeEvent = true;
        }
    }
    else if (event->type() == QEvent::FocusIn)
    {
        parentWidget()->setFocus();
    }

    return QWidget::eventFilter(object, event) || consumeEvent;
}

void CViewportTitleDlg::OnBnClickedGotoPosition()
{
    emit ActionTriggered(ID_DISPLAY_GOTOPOSITION);
}

void CViewportTitleDlg::OnBnClickedMuteAudio()
{
    gSettings.bMuteAudio = !gSettings.bMuteAudio;
    if (gSettings.bMuteAudio)
    {
        LmbrCentral::AudioSystemComponentRequestBus::Broadcast(&LmbrCentral::AudioSystemComponentRequestBus::Events::GlobalMuteAudio);
    }
    else
    {
        LmbrCentral::AudioSystemComponentRequestBus::Broadcast(&LmbrCentral::AudioSystemComponentRequestBus::Events::GlobalUnmuteAudio);
    }
}

inline double Round(double fVal, double fStep)
{
    if (fStep > 0.f)
    {
        fVal = int_round(fVal / fStep) * fStep;
    }
    return fVal;
}

 namespace
{
    void PyToggleHelpers()
    {
        AzToolsFramework::SetHelpersVisible(!AzToolsFramework::HelpersVisible());
        AzToolsFramework::EditorSettingsAPIBus::Broadcast(&AzToolsFramework::EditorSettingsAPIBus::Events::SaveSettingsRegistryFile);
    }

    bool PyIsHelpersShown()
    {
        return AzToolsFramework::HelpersVisible();
    }

    void PyToggleIcons()
    {
        AzToolsFramework::SetIconsVisible(!AzToolsFramework::IconsVisible());
        AzToolsFramework::EditorSettingsAPIBus::Broadcast(&AzToolsFramework::EditorSettingsAPIBus::Events::SaveSettingsRegistryFile);
    }

    bool PyIsIconsShown()
    {
        return AzToolsFramework::IconsVisible();
    }
} // namespace

namespace AzToolsFramework
{
    void ViewportTitleDlgPythonFuncsHandler::Reflect(AZ::ReflectContext* context)
    {
        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            // this will put these methods into the 'azlmbr.legacy.general' module
            auto addLegacyGeneral = [](AZ::BehaviorContext::GlobalMethodBuilder methodBuilder)
            {
                methodBuilder->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Category, "Legacy/Editor")
                    ->Attribute(AZ::Script::Attributes::Module, "legacy.general");
            };

            addLegacyGeneral(behaviorContext->Method("toggle_helpers", PyToggleHelpers, nullptr, "Toggles the display of helpers."));
            addLegacyGeneral(behaviorContext->Method("is_helpers_shown", PyIsHelpersShown, nullptr, "Gets the display state of helpers."));
            addLegacyGeneral(behaviorContext->Method("toggle_icons", PyToggleIcons, nullptr, "Toggles the display of icons."));
            addLegacyGeneral(behaviorContext->Method("is_icons_shown", PyIsIconsShown, nullptr, "Gets the display state of icons."));
        }
    }
} // namespace AzToolsFramework

#include "ViewportTitleDlg.moc"
#include <moc_ViewportTitleDlg.cpp>
