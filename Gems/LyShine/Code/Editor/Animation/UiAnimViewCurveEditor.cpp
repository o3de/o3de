/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"
#include "Editor/Resource.h"
#include "UiEditorAnimationBus.h"
#include "UiAnimViewCurveEditor.h"

#include "UiAnimViewDialog.h"
#include "UiAnimViewTrack.h"
#include "AnimationContext.h"

#include <Editor/Animation/ui_UiAnimViewCurveEditor.h>

#include <QtUtil.h>
#if defined(Q_OS_WIN)
#include <QtWinExtras/QtWin>
#endif


#define IDC_TRACKVIEWGRAPH_CURVE 1
#define IDC_TIMELINE             2

#define IDC_HORIZON_SLIDER      3
#define IDC_VERTICAL_SLIDER     4

//! It's for mapping from a slider control range to a real zoom range, and vice versa.
#define SLIDER_MULTIPLIER       100.f
#define SLIDERRANGE_TO_ZOOM(SLIDERVALUE)    (float)SLIDERVALUE / SLIDER_MULTIPLIER
#define ZOOMRANGE_TO_SLIDER(ZOOMVALUE)      (int)(ZOOMVALUE * SLIDER_MULTIPLIER)


//////////////////////////////////////////////////////////////////////////
UiAnimViewCurveEditorDialog::UiAnimViewCurveEditorDialog(QWidget* parent)
    : QWidget(parent)
{
    m_widget = new CUiAnimViewCurveEditor(this);
    QVBoxLayout* l = new QVBoxLayout;
    l->setMargin(0);
    l->addWidget(m_widget);
    setLayout(l);
}

//////////////////////////////////////////////////////////////////////////
CUiAnimViewCurveEditor::CUiAnimViewCurveEditor(QWidget* parent)
    : QWidget(parent)
    , m_ui(new Ui::UiAnimViewCurveEditor)
{
    m_ui->setupUi(this);
    m_hasActiveCanvas = false;
    GetIEditor()->RegisterNotifyListener(this);

    CUiAnimationContext* pAnimationContext = nullptr;
    UiEditorAnimationBus::BroadcastResult(pAnimationContext, &UiEditorAnimationBus::Events::GetAnimationContext);
    pAnimationContext->AddListener(this);

    m_timelineCtrl.SetTimeRange(Range(0, 1));
    m_timelineCtrl.SetTicksTextScale(1.0f);

    m_ui->m_wndSpline->SetTimelineCtrl(&m_timelineCtrl);

    connect(m_ui->m_horizontalScrollBar, &QAbstractSlider::valueChanged, this, &CUiAnimViewCurveEditor::OnHorizontalScrollBarChange);
    connect(m_ui->m_verticalScrollBar, &QAbstractSlider::valueChanged, this, &CUiAnimViewCurveEditor::OnVerticalScrollBarChange);
    connect(&m_timelineCtrl, &TimelineWidget::change, this, &CUiAnimViewCurveEditor::OnTimelineChange);
    connect(m_ui->m_wndSpline, &SplineWidget::scrollZoomRequested, this, &CUiAnimViewCurveEditor::OnSplineScrollZoom);
    connect(m_ui->m_wndSpline, &SplineWidget::change, this, &CUiAnimViewCurveEditor::OnSplineChange);

    connect(m_ui->buttonTangentAuto,      &QToolButton::clicked, this, [&]() {OnSplineCmd(ID_TANGENT_AUTO); });
    connect(m_ui->buttonTangentInZero,    &QToolButton::clicked, this, [&]() {OnSplineCmd(ID_TANGENT_IN_ZERO); });
    connect(m_ui->buttonTangentInStep,    &QToolButton::clicked, this, [&]() {OnSplineCmd(ID_TANGENT_IN_STEP); });
    connect(m_ui->buttonTangentInLinear,  &QToolButton::clicked, this, [&]() {OnSplineCmd(ID_TANGENT_IN_LINEAR); });
    connect(m_ui->buttonTangentOutZero,   &QToolButton::clicked, this, [&]() {OnSplineCmd(ID_TANGENT_OUT_ZERO); });
    connect(m_ui->buttonTangentOutStep,   &QToolButton::clicked, this, [&]() {OnSplineCmd(ID_TANGENT_OUT_STEP); });
    connect(m_ui->buttonTangentOutLinear, &QToolButton::clicked, this, [&]() {OnSplineCmd(ID_TANGENT_OUT_LINEAR); });
    connect(m_ui->buttonSplineFitX,       &QToolButton::clicked, this, [&]() {OnSplineCmd(ID_SPLINE_FIT_X); });
    connect(m_ui->buttonSplineFitY,       &QToolButton::clicked, this, [&]() {OnSplineCmd(ID_SPLINE_FIT_Y); });
    connect(m_ui->buttonSplineSnapGridX,  &QToolButton::clicked, this, [&]() {OnSplineCmd(ID_SPLINE_SNAP_GRID_X); });
    connect(m_ui->buttonSplineSnapGridY,  &QToolButton::clicked, this, [&]() {OnSplineCmd(ID_SPLINE_SNAP_GRID_Y); });
    connect(m_ui->buttonTangentUnify,     &QToolButton::clicked, this, [&]() {OnSplineCmd(ID_TANGENT_UNIFY); });
    connect(m_ui->buttonFreezeKeys,       &QToolButton::clicked, this, [&]() {OnSplineCmd(ID_FREEZE_KEYS); });
    connect(m_ui->buttonFreezeTangents,   &QToolButton::clicked, this, [&]() {OnSplineCmd(ID_FREEZE_TANGENTS); });

    m_ui->buttonTangentAuto->setIcon(QPixmap(QStringLiteral(":/splines/spline_edit_bar_00.png")));
    m_ui->buttonTangentInZero->setIcon(QPixmap(QStringLiteral(":/splines/spline_edit_bar_01.png")));
    m_ui->buttonTangentInStep->setIcon(QPixmap(QStringLiteral(":/splines/spline_edit_bar_02.png")));
    m_ui->buttonTangentInLinear->setIcon(QPixmap(QStringLiteral(":/splines/spline_edit_bar_03.png")));
    m_ui->buttonTangentOutZero->setIcon(QPixmap(QStringLiteral(":/splines/spline_edit_bar_04.png")));
    m_ui->buttonTangentOutStep->setIcon(QPixmap(QStringLiteral(":/splines/spline_edit_bar_05.png")));
    m_ui->buttonTangentOutLinear->setIcon(QPixmap(QStringLiteral(":/splines/spline_edit_bar_06.png")));
    m_ui->buttonSplineFitX->setIcon(QPixmap(QStringLiteral(":/splines/spline_edit_bar_07.png")));
    m_ui->buttonSplineFitY->setIcon(QPixmap(QStringLiteral(":/splines/spline_edit_bar_08.png")));
    m_ui->buttonSplineSnapGridX->setIcon(QPixmap(QStringLiteral(":/splines/spline_edit_bar_09.png")));
    m_ui->buttonSplineSnapGridY->setIcon(QPixmap(QStringLiteral(":/splines/spline_edit_bar_10.png")));
    m_ui->buttonTangentUnify->setIcon(QPixmap(QStringLiteral(":/splines/spline_edit_bar_11.png")));
    m_ui->buttonFreezeKeys->setIcon(QPixmap(QStringLiteral(":/splines/spline_edit_bar_12.png")));
    m_ui->buttonFreezeTangents->setIcon(QPixmap(QStringLiteral(":/splines/spline_edit_bar_13.png")));

    ResetScrollBarRange();

    UiEditorAnimListenerBus::Handler::BusConnect();

    // There may already be an active canvas when we open the UI animation window. If so, set our flag so
    // we know there is an active canvas. NOTE: The sequence manager will return null from GetAnimationSystem
    // if there is no active canvas.
    IUiAnimationSystem* animationSystem = CUiAnimViewSequenceManager::GetSequenceManager()->GetAnimationSystem();
    if (animationSystem)
    {
        m_hasActiveCanvas = true;
    }
}

//////////////////////////////////////////////////////////////////////////
CUiAnimViewCurveEditor::~CUiAnimViewCurveEditor()
{
    CUiAnimationContext* pAnimationContext = nullptr;
    UiEditorAnimationBus::BroadcastResult(pAnimationContext, &UiEditorAnimationBus::Events::GetAnimationContext);

    pAnimationContext->RemoveListener(this);
    GetIEditor()->UnregisterNotifyListener(this);

    UiEditorAnimListenerBus::Handler::BusDisconnect();
}



// CUiAnimViewGraph message handlers

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewCurveEditor::OnSequenceChanged([[maybe_unused]] CUiAnimViewSequence* pSequence)
{
    m_ui->m_wndSpline->RemoveAllSplines();
    UpdateSplines();
    update();
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewCurveEditor::OnEditorNotifyEvent([[maybe_unused]] EEditorNotifyEvent event)
{
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewCurveEditor::OnActiveCanvasChanged()
{
    m_ui->m_wndSpline->RemoveAllSplines();
    IUiAnimationSystem* animationSystem = CUiAnimViewSequenceManager::GetSequenceManager()->GetAnimationSystem();
    m_hasActiveCanvas = animationSystem ? true : false;
    UpdateSplines();
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewCurveEditor::UpdateSplines()
{
    CUiAnimViewSequence* pSequence = nullptr;
    UiEditorAnimationBus::BroadcastResult(pSequence, &UiEditorAnimationBus::Events::GetCurrentSequence);

    if (!pSequence || !m_hasActiveCanvas)
    {
        return;
    }

    CUiAnimViewTrackBundle selectedTracks;
    selectedTracks = pSequence->GetSelectedTracks();

    std::set<CUiAnimViewTrack*> oldTracks;
    for (auto iter = m_ui->m_wndSpline->GetTracks().begin(); iter != m_ui->m_wndSpline->GetTracks().end(); ++iter)
    {
        CUiAnimViewTrack* pTrack = *iter;
        oldTracks.insert(pTrack);
    }

    std::set<CUiAnimViewTrack*> newTracks;
    if (selectedTracks.AreAllOfSameType())
    {
        for (unsigned int i = 0; i < selectedTracks.GetCount(); i++)
        {
            CUiAnimViewTrack* pTrack = selectedTracks.GetTrack(i);

            if (pTrack->IsCompoundTrack())
            {
                unsigned int numChildTracks = pTrack->GetChildCount();
                for (unsigned int ii = 0; ii < numChildTracks; ++ii)
                {
                    CUiAnimViewTrack* pChildTrack = static_cast<CUiAnimViewTrack*>(pTrack->GetChild(ii));
                    newTracks.insert(pChildTrack);
                }
            }
            else
            {
                newTracks.insert(pTrack);
            }
        }
    }

    if (oldTracks == newTracks)
    {
        return;
    }

    m_ui->m_wndSpline->RemoveAllSplines();
    for (auto iter = newTracks.begin(); iter != newTracks.end(); ++iter)
    {
        AddSpline(*iter);
    }

    UpdateTimeRange(pSequence);

    // If it is a rotation track, adjust the default value range properly to accommodate some degree values.
    if (selectedTracks.HasRotationTrack())
    {
        m_ui->m_wndSpline->SetDefaultValueRange(Range(-180.0f, 180.0f));
    }
    else
    {
        m_ui->m_wndSpline->SetDefaultValueRange(Range(-1.1f, 1.1f));
    }

    ResetSplineCtrlZoomLevel();

    return;
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewCurveEditor::AddSpline(CUiAnimViewTrack* pTrack)
{
    if (!pTrack->GetSpline())
    {
        return;
    }

    const int subTrackIndex = pTrack->GetSubTrackIndex();

    if (subTrackIndex >= 0)
    {
        QColor trackColor = QColor(255, 0, 0);
        switch (subTrackIndex)
        {
        case 0:
            trackColor = QColor(255, 0, 0);
            break;
        case 1:
            trackColor = QColor(0, 255, 0);
            break;
        case 2:
            trackColor = QColor(0, 0, 255);
            break;
        case 3:
            trackColor = QColor(255, 255, 0);
            break;
        }

        m_ui->m_wndSpline->AddSpline(pTrack->GetSpline(), pTrack, trackColor);
    }
    else
    {
        QColor    afColorArray[4];
        afColorArray[0] = QColor(255, 0, 0);
        afColorArray[1] = QColor(0, 255, 0);
        afColorArray[2] = QColor(0, 0, 255);
        afColorArray[3] = QColor(255, 0, 255); //Pink... so you know it's wrong if you see it.

        m_ui->m_wndSpline->AddSpline(pTrack->GetSpline(), pTrack, afColorArray);
    }
}

void CUiAnimViewCurveEditor::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    OnSplineCmdUpdateUI();
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewCurveEditor::OnSplineChange()
{
    CUiAnimViewSequence* pSequence = nullptr;
    UiEditorAnimationBus::BroadcastResult(pSequence, &UiEditorAnimationBus::Events::GetCurrentSequence);
    if (pSequence)
    {
        pSequence->OnKeysChanged();
    }

    // In the end, focus this again in order to properly catch 'KeyDown' messages.
    m_ui->m_wndSpline->setFocus();
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewCurveEditor::OnSplineCmd(UINT cmd)
{
    m_ui->m_wndSpline->OnUserCommand(cmd);
    OnSplineCmdUpdateUI();
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewCurveEditor::OnSplineCmdUpdateUI()
{
    CUiAnimViewSequence* pSequence = nullptr;
    UiEditorAnimationBus::BroadcastResult(pSequence, &UiEditorAnimationBus::Events::GetCurrentSequence);

    if (!m_hasActiveCanvas || !pSequence)
    {
        return;
    }

    m_ui->buttonSplineSnapGridX->setChecked(m_ui->m_wndSpline->IsSnapTime());
    m_ui->buttonSplineSnapGridY->setChecked(m_ui->m_wndSpline->IsSnapValue());
    m_ui->buttonTangentUnify->setChecked(m_ui->m_wndSpline->IsUnifiedKeyCurrentlySelected());
    m_ui->buttonFreezeKeys->setChecked(m_ui->m_wndSpline->IsKeysFrozen());
    m_ui->buttonFreezeTangents->setChecked(m_ui->m_wndSpline->IsTangentsFrozen());
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewCurveEditor::OnTimeChanged(float newTime)
{
    m_ui->m_wndSpline->SetTimeMarker(newTime);
    m_ui->m_wndSpline->update();
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewCurveEditor::SetEditLock(bool bLock)
{
    m_ui->m_wndSpline->SetEditLock(bLock);
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewCurveEditor::OnTimelineChange()
{
    float fTime = m_timelineCtrl.GetTimeMarker();

    CUiAnimationContext* pAnimationContext = nullptr;
    UiEditorAnimationBus::BroadcastResult(pAnimationContext, &UiEditorAnimationBus::Events::GetAnimationContext);

    pAnimationContext->SetTime(fTime);
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewCurveEditor::OnHorizontalScrollBarChange()
{
    int pos = m_ui->m_horizontalScrollBar->value();
    Vec2 zoom = m_ui->m_wndSpline->GetZoom();

    // Zero value is not acceptable.
    zoom.x = max(SLIDERRANGE_TO_ZOOM(pos), 1.f / SLIDER_MULTIPLIER);
    m_ui->m_wndSpline->SetZoom(zoom);
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewCurveEditor::OnVerticalScrollBarChange()
{
    int pos = m_ui->m_verticalScrollBar->value();
    Vec2 zoom = m_ui->m_wndSpline->GetZoom();

    // Zero value is not acceptable.
    zoom.y = max(SLIDERRANGE_TO_ZOOM(pos), 1.f / SLIDER_MULTIPLIER);
    m_ui->m_wndSpline->SetZoom(zoom);
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewCurveEditor::OnSplineScrollZoom()
{
    ResetScrollBarRange();
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewCurveEditor::ResetScrollBarRange()
{
    Vec2 zoom = m_ui->m_wndSpline->GetZoom();
    Vec2 minValue = zoom / 2.f;
    Vec2 maxValue = zoom * 2.f;

    const QSignalBlocker sb1(m_ui->m_horizontalScrollBar);
    const QSignalBlocker sb2(m_ui->m_verticalScrollBar);
    m_ui->m_horizontalScrollBar->setRange(ZOOMRANGE_TO_SLIDER(minValue.x), ZOOMRANGE_TO_SLIDER(maxValue.x));
    m_ui->m_horizontalScrollBar->setValue(ZOOMRANGE_TO_SLIDER((minValue.x + maxValue.x) / 2.f));

    m_ui->m_verticalScrollBar->setRange(ZOOMRANGE_TO_SLIDER(minValue.y), ZOOMRANGE_TO_SLIDER(maxValue.y));
    m_ui->m_verticalScrollBar->setValue(ZOOMRANGE_TO_SLIDER((minValue.y + maxValue.y) / 2.f));
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewCurveEditor::SetFPS(float fps)
{
    m_timelineCtrl.SetFPS(fps);
}

//////////////////////////////////////////////////////////////////////////
float CUiAnimViewCurveEditor::GetFPS() const
{
    return m_timelineCtrl.GetFPS();
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewCurveEditor::SetTickDisplayMode(EUiAVTickMode mode)
{
    if (mode == eUiAVTickMode_InFrames)
    {
        m_timelineCtrl.SetMarkerStyle(TimelineWidget::MARKER_STYLE_FRAMES);
        m_ui->m_wndSpline->SetTooltipValueScale(GetFPS(), 1.0f);
    }
    else if (mode == eUiAVTickMode_InSeconds)
    {
        m_timelineCtrl.SetMarkerStyle(TimelineWidget::MARKER_STYLE_SECONDS);
        m_ui->m_wndSpline->SetTooltipValueScale(1.0f, 1.0f);
    }

    m_timelineCtrl.update();
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewCurveEditor::ResetSplineCtrlZoomLevel()
{
    m_ui->m_wndSpline->FitSplineToViewHeight();
    m_ui->m_wndSpline->FitSplineToViewWidth();
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewCurveEditor::OnKeysChanged([[maybe_unused]] CUiAnimViewSequence* pSequence)
{
    m_ui->m_wndSpline->update();
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewCurveEditor::OnKeySelectionChanged([[maybe_unused]] CUiAnimViewSequence* pSequence)
{
    m_ui->m_wndSpline->update();
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewCurveEditor::OnNodeChanged([[maybe_unused]] CUiAnimViewNode* pNode, ENodeChangeType type)
{
    if (isVisible() && type == IUiAnimViewSequenceListener::eNodeChangeType_Removed)
    {
        UpdateSplines();
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewCurveEditor::OnNodeSelectionChanged([[maybe_unused]] CUiAnimViewSequence* pSequence)
{
    if (isVisible())
    {
        UpdateSplines();
    }
}

void CUiAnimViewCurveEditor::OnSequenceSettingsChanged(CUiAnimViewSequence* pSequence)
{
    if (isVisible())
    {
        UpdateTimeRange(pSequence);
        m_timelineCtrl.update();
        m_ui->m_wndSpline->update();
    }
}

void CUiAnimViewCurveEditor::UpdateTimeRange(CUiAnimViewSequence* pSequence)
{
    Range timeRange =   pSequence->GetTimeRange();
    m_ui->m_wndSpline->SetTimeRange(timeRange);
    m_timelineCtrl.SetTimeRange(timeRange);
    m_ui->m_wndSpline->SetValueRange(Range(-2000.0f, 2000.0f));
}

void CUiAnimViewCurveEditor::SetPlayCallback(const std::function<void()>& callback)
{
    m_ui->m_wndSpline->SetPlayCallback(callback);
    m_timelineCtrl.SetPlayCallback(callback);
}

CUiAnimViewSplineCtrl& CUiAnimViewCurveEditor::GetSplineCtrl()
{
    return *m_ui->m_wndSpline;
}
