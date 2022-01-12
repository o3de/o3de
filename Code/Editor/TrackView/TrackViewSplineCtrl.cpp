/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "TrackViewSplineCtrl.h"

// Qt
#include <QRubberBand>

// Editor
#include "AnimationContext.h"


class CUndoTrackViewSplineCtrl
    : public ISplineCtrlUndo
{
public:
    CUndoTrackViewSplineCtrl(CTrackViewSplineCtrl* pCtrl, std::vector<ISplineInterpolator*>& splineContainer)
    {
        if (GetIEditor()->GetAnimation()->GetSequence())
        {
            m_sequenceEntityId = GetIEditor()->GetAnimation()->GetSequence()->GetSequenceComponentEntityId();
        }

        m_pCtrl = pCtrl;

        // Loop over all affected splines
        for (size_t splineIndex = 0; splineIndex < splineContainer.size(); ++splineIndex)
        {
            AddSpline(splineContainer[splineIndex]);
        }

        SerializeSplines(&CSplineEntry::m_undo, false);
    }

protected:
    void AddSpline(ISplineInterpolator* pSpline)
    {
        // Find corresponding track and remember it
        for (size_t trackIndex = 0; trackIndex < m_pCtrl->m_tracks.size(); ++trackIndex)
        {
            CTrackViewTrack* pTrack = m_pCtrl->m_tracks[trackIndex];
            if (pTrack->GetSpline() == pSpline)
            {
                CSplineEntry entry;
                entry.m_trackId = pTrack->GetId();
                m_splineEntries.push_back(entry);
            }
        }
    }

    int GetSize() override { return sizeof(*this); }

    void Undo(bool bUndo) override
    {
        CTrackViewSplineCtrl* pCtrl = FindControl(m_pCtrl);
        if (pCtrl)
        {
            pCtrl->SendNotifyEvent(SPLN_BEFORE_CHANGE);
        }

        const CTrackViewSequenceManager* sequenceManager = GetIEditor()->GetSequenceManager();
        CTrackViewSequence* sequence = sequenceManager->GetSequenceByEntityId(m_sequenceEntityId);

        AZ_Assert(sequence, "Expected valid sequence.");
        if (!sequence)
        {
            return;
        }

        CTrackViewSequenceNotificationContext context(sequence);

        if (bUndo)
        {
            // Save key selection states for redo if necessary
            m_redoKeyStates = sequence->SaveKeyStates();
            SerializeSplines(&CSplineEntry::m_redo, false);
        }

        SerializeSplines(&CSplineEntry::m_undo, true);

        // Undo key selection state
        sequence->RestoreKeyStates(m_undoKeyStates);

        if (pCtrl && bUndo)
        {
            pCtrl->m_bKeyTimesDirty = true;
            pCtrl->SendNotifyEvent(SPLN_CHANGE);
            pCtrl->update();
        }

        if (bUndo)
        {
            sequence->OnKeySelectionChanged();
        }
    }

    void Redo() override
    {
        const CTrackViewSequenceManager* pSequenceManager = GetIEditor()->GetSequenceManager();
        CTrackViewSequence* sequence = pSequenceManager->GetSequenceByEntityId(m_sequenceEntityId);
        AZ_Assert(sequence, "Expected valid sequence.");
        if (!sequence)
        {
            return;
        }

        CTrackViewSequenceNotificationContext context(sequence);

        CTrackViewSplineCtrl* pCtrl = FindControl(m_pCtrl);

        if (pCtrl)
        {
            pCtrl->SendNotifyEvent(SPLN_BEFORE_CHANGE);
        }
        SerializeSplines(&CSplineEntry::m_redo, true);

        // Redo key selection state
        sequence->RestoreKeyStates(m_redoKeyStates);

        if (pCtrl)
        {
            pCtrl->m_bKeyTimesDirty = true;
            pCtrl->SendNotifyEvent(SPLN_CHANGE);
            pCtrl->update();
        }

        sequence->OnKeySelectionChanged();
    }

    bool IsSelectionChanged() const override
    {
        const CTrackViewSequenceManager* sequenceManager = GetIEditor()->GetSequenceManager();
        CTrackViewSequence* sequence = sequenceManager->GetSequenceByEntityId(m_sequenceEntityId);
        AZ_Assert(sequence, "Expected valid sequence.");
        if (!sequence)
        {
            return false;
        }

        const std::vector<bool> currentKeyState = sequence->SaveKeyStates();
        return m_undoKeyStates != currentKeyState;
    }

public:
    using CTrackViewSplineCtrls = std::list<CTrackViewSplineCtrl*>;

    static CTrackViewSplineCtrl* FindControl(CTrackViewSplineCtrl* pCtrl)
    {
        if (!pCtrl)
        {
            return nullptr;
        }

        auto iter = std::find(s_activeCtrls.begin(), s_activeCtrls.end(), pCtrl);
        if (iter == s_activeCtrls.end())
        {
            return nullptr;
        }

        return *iter;
    }

    static void RegisterControl(CTrackViewSplineCtrl* pCtrl)
    {
        if (!FindControl(pCtrl))
        {
            s_activeCtrls.push_back(pCtrl);
        }
    }

    static void UnregisterControl(CTrackViewSplineCtrl* pCtrl)
    {
        if (FindControl(pCtrl))
        {
            s_activeCtrls.remove(pCtrl);
        }
    }

    static CTrackViewSplineCtrls s_activeCtrls;

private:
    class CSplineEntry
    {
    public:
        _smart_ptr<ISplineBackup> m_undo;
        _smart_ptr<ISplineBackup> m_redo;
        unsigned int m_trackId;
    };

    void SerializeSplines(_smart_ptr<ISplineBackup> CSplineEntry::* backup, bool bLoading)
    {
        const CTrackViewSequenceManager* sequenceManager = GetIEditor()->GetSequenceManager();
        CTrackViewSequence* sequence = sequenceManager->GetSequenceByEntityId(m_sequenceEntityId);
        AZ_Assert(sequence, "Expected valid sequence");
        if (!sequence)
        {
            return;
        }

        for (auto it = m_splineEntries.begin(); it != m_splineEntries.end(); ++it)
        {
            CSplineEntry& entry = *it;
            CTrackViewTrack* track = sequence->FindTrackById(entry.m_trackId);
            if (track)
            {
                ISplineInterpolator* pSpline = track->GetSpline();

                if (pSpline && bLoading)
                {
                    pSpline->Restore(entry.*backup);
                }
                else if (pSpline)
                {
                    (entry.*backup) = pSpline->Backup();
                }
            }
        }
    }

    AZ::EntityId m_sequenceEntityId;
    CTrackViewSplineCtrl* m_pCtrl;
    std::vector<CSplineEntry> m_splineEntries;
    std::vector<float> m_keyTimes;
    std::vector<bool> m_undoKeyStates;
    std::vector<bool> m_redoKeyStates;
};

CUndoTrackViewSplineCtrl::CTrackViewSplineCtrls CUndoTrackViewSplineCtrl::s_activeCtrls;

//////////////////////////////////////////////////////////////////////////
CTrackViewSplineCtrl::CTrackViewSplineCtrl(QWidget* parent)
    : SplineWidget(parent)
    , m_bKeysFreeze(false)
    , m_bTangentsFreeze(false)
    , m_stashedRecordModeWhenDraggingTime(false)
{
    CUndoTrackViewSplineCtrl::RegisterControl(this);
}

CTrackViewSplineCtrl::~CTrackViewSplineCtrl()
{
    CUndoTrackViewSplineCtrl::UnregisterControl(this);
}

bool CTrackViewSplineCtrl::GetTangentHandlePts(QPoint& inTangentPt, QPoint& pt, QPoint& outTangentPt,
    int nSpline, int nKey, int nDimension)
{
    ISplineInterpolator* pSpline = m_splines[nSpline].pSpline;
    CTrackViewTrack* pTrack = m_tracks[nSpline];

    float time = pSpline->GetKeyTime(nKey);

    ISplineInterpolator::ValueType value, tin, tout;
    ISplineInterpolator::ZeroValue(value);
    pSpline->GetKeyValue(nKey, value);
    pSpline->GetKeyTangents(nKey, tin, tout);

    const CTrackViewKeyHandle& keyHandle = pTrack->GetKey(nKey);

    if (pTrack->GetCurveType() == eAnimCurveType_TCBFloat)
    {
        ITcbKey tcbKey;
        keyHandle.GetKey(&tcbKey);

        Vec2 va, vb, vc;
        va.x = time - 1.0f;
        va.y = value[nDimension] - tin[nDimension];
        vb.x = time;
        vb.y = value[nDimension];
        vc.x = time + 1.0f;
        vc.y = value[nDimension] + tout[nDimension];
        inTangentPt = WorldToClient(va);
        pt = WorldToClient(vb);
        outTangentPt = WorldToClient(vc);

        Vec2 tinv, toutv;
        float maxLength = float(outTangentPt.x() - pt.x());
        tinv.x = float(inTangentPt.x() - pt.x());
        tinv.y = float(inTangentPt.y() - pt.y());
        toutv.x = float(outTangentPt.x() - pt.x());
        toutv.y = float(outTangentPt.y() - pt.y());
        tinv.Normalize();
        toutv.Normalize();
        tinv *= maxLength / (2 - tcbKey.easeto);
        toutv *= maxLength / (2 - tcbKey.easefrom);

        inTangentPt = pt + QPoint(int(tinv.x), int(tinv.y));
        outTangentPt = pt + QPoint(int(toutv.x), int(toutv.y));
    }
    else
    {
        assert(pTrack->GetCurveType() == eAnimCurveType_BezierFloat);
        assert(nDimension == 0);

        I2DBezierKey bezierKey;
        keyHandle.GetKey(&bezierKey);

        Vec2 va, vb, vc;
        va.x = time - tin[0];
        va.y = value[0] - tin[1];
        vb.x = time;
        vb.y = value[0];
        vc.x = time + tout[0];
        vc.y = value[0] + tout[1];
        inTangentPt = WorldToClient(va);
        pt = WorldToClient(vb);
        outTangentPt = WorldToClient(vc);
    }

    return true;
}

void CTrackViewSplineCtrl::ComputeIncomingTangentAndEaseTo(float& ds, float& easeTo, QPoint inTangentPt,
    int nSpline, int nKey, int nDimension)
{
    ISplineInterpolator* pSpline = m_splines[nSpline].pSpline;
    CTrackViewTrack* pTrack = m_tracks[nSpline];

    float time = pSpline->GetKeyTime(nKey);

    ISplineInterpolator::ValueType value, tin, tout;
    ISplineInterpolator::ZeroValue(value);
    pSpline->GetKeyValue(nKey, value);
    pSpline->GetKeyTangents(nKey, tin, tout);

    const CTrackViewKeyHandle& keyHandle = pTrack->GetKey(nKey);

    ITcbKey tcbKey;
    keyHandle.GetKey(&tcbKey);

    // Get the control point.
    Vec2 tinv, vb;
    vb.x = time;
    vb.y = value[nDimension];
    QPoint pt = WorldToClient(vb);

    // Get the max length to compute the 'ease' value.
    float maxLength = float(WorldToClient(Vec2(vb.x + 1, vb.y)).x() - pt.x());

    QPoint tmp = inTangentPt - pt;
    tinv.x = float(tmp.x());
    tinv.y = float(tmp.y());

    // Compute the 'easeTo'.
    easeTo = 2.0f - maxLength / tinv.GetLength();

    // Compute the 'ds'.
    Vec2 va = ClientToWorld(inTangentPt);
    if (time < va.x + 0.000001f)
    {
        if (value[nDimension] > va.y)
        {
            ds = 1000000.0f;
        }
        else
        {
            ds = -1000000.0f;
        }
    }
    else
    {
        ds = (value[nDimension] - va.y) / (time - va.x);
    }
}

void CTrackViewSplineCtrl::ComputeOutgoingTangentAndEaseFrom(float& dd, float& easeFrom, QPoint outTangentPt,
    int nSpline, int nKey, int nDimension)
{
    ISplineInterpolator* pSpline = m_splines[nSpline].pSpline;
    CTrackViewTrack* pTrack = m_tracks[nSpline];

    float time = pSpline->GetKeyTime(nKey);

    ISplineInterpolator::ValueType value, tin, tout;
    ISplineInterpolator::ZeroValue(value);
    pSpline->GetKeyValue(nKey, value);
    pSpline->GetKeyTangents(nKey, tin, tout);

    const CTrackViewKeyHandle& keyHandle = pTrack->GetKey(nKey);

    ITcbKey tcbKey;
    keyHandle.GetKey(&tcbKey);

    // Get the control point.
    Vec2 toutv, vb;
    vb.x = time;
    vb.y = value[nDimension];
    QPoint pt = WorldToClient(vb);

    // Get the max length to comute the 'ease' value.
    float maxLength = float(WorldToClient(Vec2(vb.x + 1, vb.y)).x() - pt.x());

    QPoint tmp = outTangentPt - pt;
    toutv.x = float(tmp.x());
    toutv.y = float(tmp.y());

    // Compute the 'easeFrom'.
    easeFrom = 2.0f - maxLength / toutv.GetLength();

    // Compute the 'dd'.
    Vec2 vc = ClientToWorld(outTangentPt);
    if (vc.x < time + 0.000001f)
    {
        if (value[nDimension] < vc.y)
        {
            dd = 1000000.0f;
        }
        else
        {
            dd = -1000000.0f;
        }
    }
    else
    {
        dd = (vc.y - value[nDimension]) / (vc.x - time);
    }
}

void CTrackViewSplineCtrl::AddSpline(ISplineInterpolator* pSpline, CTrackViewTrack* pTrack, const QColor& color)
{
    QColor colorArray[4];
    colorArray[0] = colorArray[1] = colorArray[2] = colorArray[3] = color;
    AddSpline(pSpline, pTrack, colorArray);
}

void CTrackViewSplineCtrl::AddSpline(ISplineInterpolator* pSpline, CTrackViewTrack* pTrack, QColor anColorArray[4])
{
    for (int i = 0; i < (int)m_splines.size(); i++)
    {
        if (m_splines[i].pSpline == pSpline)
        {
            return;
        }
    }
    SSplineInfo si;

    for (int nCurrentDimension = 0; nCurrentDimension < pSpline->GetNumDimensions(); nCurrentDimension++)
    {
        si.anColorArray[nCurrentDimension] = anColorArray[nCurrentDimension];
    }

    si.pSpline = pSpline;
    si.pDetailSpline = nullptr;
    m_splines.push_back(si);
    m_tracks.push_back(pTrack);
    m_bKeyTimesDirty = true;
    update();
}

void CTrackViewSplineCtrl::RemoveAllSplines()
{
    m_tracks.clear();
    SplineWidget::RemoveAllSplines();
}

void CTrackViewSplineCtrl::MoveSelectedTangentHandleTo(const QPoint& point)
{
    assert(m_pHitSpline && m_nHitKeyIndex >= 0 && m_bHitIncomingHandle >= 0);

    // Set the custom flag to the key.
    int nRemoveFlags, nAddFlags;
    if (m_bHitIncomingHandle)
    {
        nRemoveFlags = SPLINE_KEY_TANGENT_IN_MASK;
        nAddFlags = SPLINE_KEY_TANGENT_CUSTOM << SPLINE_KEY_TANGENT_IN_SHIFT;
    }
    else
    {
        nRemoveFlags = SPLINE_KEY_TANGENT_OUT_MASK;
        nAddFlags = SPLINE_KEY_TANGENT_CUSTOM << SPLINE_KEY_TANGENT_OUT_SHIFT;
    }
    int flags = m_pHitSpline->GetKeyFlags(m_nHitKeyIndex);
    flags &= ~nRemoveFlags;
    flags |= nAddFlags;
    m_pHitSpline->SetKeyFlags(m_nHitKeyIndex, flags);

    // Adjust the incoming or outgoing tangent.
    int splineIndex;
    for (splineIndex = 0; splineIndex < m_splines.size(); ++splineIndex)
    {
        if (m_pHitSpline == m_splines[splineIndex].pSpline)
        {
            break;
        }
    }
    assert(splineIndex < m_splines.size());

    CTrackViewTrack* pTrack = m_tracks[splineIndex];
    CTrackViewKeyHandle keyHandle = pTrack->GetKey(m_nHitKeyIndex);

    if (pTrack->GetCurveType() == eAnimCurveType_TCBFloat)
    {
        if (m_bHitIncomingHandle)
        {
            float ds, easeTo;
            ComputeIncomingTangentAndEaseTo(ds, easeTo, point, splineIndex, m_nHitKeyIndex, m_nHitDimension);
            // ease-to
            ITcbKey key;
            keyHandle.GetKey(&key);
            key.easeto += easeTo;
            if (key.easeto > 1)
            {
                key.easeto = 1.0f;
            }
            else if (key.easeto < 0)
            {
                key.easeto = 0;
            }
            keyHandle.SetKey(&key);
            // tin
            ISplineInterpolator::ValueType tin, tout;
            m_pHitSpline->GetKeyTangents(m_nHitKeyIndex, tin, tout);
            tin[m_nHitDimension] = ds;
            m_pHitSpline->SetKeyInTangent(m_nHitKeyIndex, tin);
        }
        else
        {
            float dd, easeFrom;
            ComputeOutgoingTangentAndEaseFrom(dd, easeFrom, point, splineIndex, m_nHitKeyIndex, m_nHitDimension);
            // ease-from
            ITcbKey key;
            keyHandle.GetKey(&key);
            key.easefrom += easeFrom;
            if (key.easefrom > 1)
            {
                key.easefrom = 1.0f;
            }
            else if (key.easefrom < 0)
            {
                key.easefrom = 0;
            }
            keyHandle.SetKey(&key);
            // tout
            ISplineInterpolator::ValueType tin, tout;
            m_pHitSpline->GetKeyTangents(m_nHitKeyIndex, tin, tout);
            tout[m_nHitDimension] = dd;
            m_pHitSpline->SetKeyOutTangent(m_nHitKeyIndex, tout);
        }
    }
    else
    {
        assert(pTrack->GetCurveType() == eAnimCurveType_BezierFloat);
        assert(m_nHitDimension == 0);

        Vec2 tp = ClientToWorld(point);
        if (m_bHitIncomingHandle)
        {
            // tin
            ISplineInterpolator::ValueType value, tin, tout;
            float time = m_pHitSpline->GetKeyTime(m_nHitKeyIndex);
            m_pHitSpline->GetKeyValue(m_nHitKeyIndex, value);
            m_pHitSpline->GetKeyTangents(m_nHitKeyIndex, tin, tout);
            tin[0] = time - tp.x;
            // Constrain the time range so that the time curve is always monotonically increasing.
            if (tin[0] < 0)
            {
                tin[0] = 0;
            }
            else if (m_nHitKeyIndex > 0
                     && tin[0] > (time - m_pHitSpline->GetKeyTime(m_nHitKeyIndex - 1)))
            {
                tin[0] = time - m_pHitSpline->GetKeyTime(m_nHitKeyIndex - 1);
            }
            tin[1] = value[0] - tp.y;
            m_pHitSpline->SetKeyInTangent(m_nHitKeyIndex, tin);
        }
        else
        {
            // tout
            ISplineInterpolator::ValueType value, tin, tout;
            float time = m_pHitSpline->GetKeyTime(m_nHitKeyIndex);
            m_pHitSpline->GetKeyValue(m_nHitKeyIndex, value);
            m_pHitSpline->GetKeyTangents(m_nHitKeyIndex, tin, tout);
            tout[0] = tp.x - time;
            // Constrain the time range so that the time curve is always monotonically increasing.
            if (tout[0] < 0)
            {
                tout[0] = 0;
            }
            else if (m_nHitKeyIndex < m_pHitSpline->GetKeyCount() - 1
                     && tout[0] > (m_pHitSpline->GetKeyTime(m_nHitKeyIndex + 1) - time))
            {
                tout[0] = m_pHitSpline->GetKeyTime(m_nHitKeyIndex + 1) - time;
            }
            tout[1] = tp.y - value[0];
            m_pHitSpline->SetKeyOutTangent(m_nHitKeyIndex, tout);
        }
    }

    SendNotifyEvent(SPLN_CHANGE);
    update();
}

void CTrackViewSplineCtrl::mouseMoveEvent(QMouseEvent* event)
{
    const QPoint point = event->pos();
    CTrackViewSequence* pSequence = GetIEditor()->GetAnimation()->GetSequence();
    if (!pSequence)
    {
        return;
    }

    CTrackViewSequenceNotificationContext context(pSequence);

    m_cMousePos = point;

    if (m_editMode == NothingMode)
    {
        switch (HitTest(point))
        {
        case HIT_SPLINE:
            setCursor(CMFCUtils::LoadCursor(IDC_ARRWHITE));
            break;
        case HIT_KEY:
        case HIT_TANGENT_HANDLE:
            setCursor(CMFCUtils::LoadCursor(IDC_ARRBLCK));
            break;
        default:
            unsetCursor();
            break;
        }
    }

    if (m_editMode == SelectMode)
    {
        unsetCursor();
        QRect rc(m_cMouseDownPos, point);
        rc = rc.normalized().intersected(m_rcSpline);

        m_rcSelect = rc;
        m_rubberBand->setGeometry(m_rcSelect);
        m_rubberBand->setVisible(true);
    }

    if (m_editMode == TimeMarkerMode)
    {
        unsetCursor();
        SetTimeMarker(XOfsToTime(point.x()));
        SendNotifyEvent(SPLN_TIME_CHANGE);
    }

    if (m_boLeftMouseButtonDown)
    {
        if (m_editMode == TrackingMode && point != m_cMouseDownPos)
        {
            m_startedDragging = true;
            GetIEditor()->RestoreUndo();
            m_pCurrentUndo = nullptr;
            StoreUndo();

            bool bAltClick = CheckVirtualKey(Qt::Key_Menu);
            bool bShiftClick = CheckVirtualKey(Qt::Key_Shift);
            bool bSpaceClick = CheckVirtualKey(Qt::Key_Space);

            Vec2 v0 = ClientToWorld(m_cMouseDownPos);
            Vec2 v1 = ClientToWorld(point);
            if (m_hitCode == HIT_TANGENT_HANDLE)
            {
                if (!m_bTangentsFreeze)
                {
                    MoveSelectedTangentHandleTo(point);
                }
            }
            else if (!m_bKeysFreeze)
            {
                if (bAltClick && bShiftClick)
                {
                    ValueScaleKeys(v0.y, v1.y);
                }
                else if (bAltClick)
                {
                    TimeScaleKeys(m_fTimeMarker, v0.x, v1.x);
                }
                else if (bShiftClick)
                {
                    // Constrains the move to the vertical direction.
                    MoveSelectedKeys(Vec2(0, v1.y - v0.y), false);
                }
                else if (bSpaceClick)
                {
                    // Reset to the original position.
                    MoveSelectedKeys(Vec2(0, 0), false);
                }
                else
                {
                    MoveSelectedKeys(v1 - v0, m_copyKeys);
                }
            }
        }
    }

    if (m_editMode == TrackingMode && GetNumSelected() == 1)
    {
        float time = 0;
        ISplineInterpolator::ValueType  afValue;
        QString                         tipText;
        bool                            boFoundTheSelectedKey(false);

        for (size_t splineIndex = 0, endSpline = m_splines.size(); splineIndex < endSpline; ++splineIndex)
        {
            ISplineInterpolator* pSpline = m_splines[splineIndex].pSpline;
            CTrackViewTrack* pTrack = m_tracks[splineIndex];
            for (int i = 0; i < pSpline->GetKeyCount(); i++)
            {
                const CTrackViewKeyHandle& keyHandle = pTrack->GetKey(i);

                for (int nCurrentDimension = 0; nCurrentDimension < pSpline->GetNumDimensions(); nCurrentDimension++)
                {
                    if (pSpline->IsKeySelectedAtDimension(i, nCurrentDimension))
                    {
                        time = pSpline->GetKeyTime(i);
                        pSpline->GetKeyValue(i, afValue);
                        if (pTrack->GetCurveType() == eAnimCurveType_TCBFloat)
                        {
                            ITcbKey key;
                            keyHandle.GetKey(&key);
                            tipText = QStringLiteral("t=%1  v=%2 / T=%3  C=%4  B=%5")
                                .arg(time * m_fTooltipScaleX, 3, 'f').arg(afValue[nCurrentDimension] * m_fTooltipScaleY, 3, 'f', 2)
                                .arg(key.tens, 3, 'f').arg(key.cont, 3, 'f').arg(key.bias, 3, 'f');
                        }
                        else
                        {
                            assert(pTrack->GetCurveType() == eAnimCurveType_BezierFloat);
                            ISplineInterpolator::ValueType tin, tout;
                            pSpline->GetKeyTangents(i, tin, tout);
                            tipText = QStringLiteral("t=%1  v=%2 / tin=(%3,%4)  tout=(%5,%6)")
                                .arg(time * m_fTooltipScaleX, 3, 'f').arg(afValue[0] * m_fTooltipScaleY, 3, 'f', 2)
                                .arg(tin[0], 3, 'f').arg(tin[1], 3, 'f', 2).arg(tout[0], 3, 'f').arg(tout[1], 3, 'f', 2);
                        }
                        boFoundTheSelectedKey = true;
                        break;
                    }
                }
                if (boFoundTheSelectedKey)
                {
                    break;
                }
            }
        }

        if (point != m_lastToolTipPos)
        {
            m_lastToolTipPos = point;
            m_tooltipText = tipText;
            update();
        }
    }

    switch (m_editMode)
    {
    case ScrollMode:
    {
        // Set the new scrolled coordinates
        float ofsx = m_grid.origin.x - (point.x() - m_cMouseDownPos.x()) / m_grid.zoom.x;
        float ofsy = m_grid.origin.y + (point.y() - m_cMouseDownPos.y()) / m_grid.zoom.y;
        SetScrollOffset(Vec2(ofsx, ofsy));
        m_cMouseDownPos = point;
    }
    break;

    case ZoomMode:
    {
        float ofsx = (point.x() - m_cMouseDownPos.x()) * 0.01f;
        float ofsy = (point.y() - m_cMouseDownPos.y()) * 0.01f;

        Vec2 z = m_grid.zoom;
        if (ofsx != 0)
        {
            z.x = max(z.x * (1.0f + ofsx), 0.001f);
        }
        if (ofsy != 0)
        {
            z.y = max(z.y * (1.0f + ofsy), 0.001f);
        }
        SetZoom(z, m_cMouseDownPos);
        m_cMouseDownPos = point;
    }
    break;
    }
}

void CTrackViewSplineCtrl::AdjustTCB(float d_tension, float d_continuity, float d_bias)
{
    CUndo undo("Modify Spline Keys TCB");
    ConditionalStoreUndo();

    SendNotifyEvent(SPLN_BEFORE_CHANGE);

    for (size_t splineIndex = 0, splineCount = m_splines.size(); splineIndex < splineCount; ++splineIndex)
    {
        ISplineInterpolator* pSpline = m_splines[splineIndex].pSpline;
        CTrackViewTrack* pTrack = m_tracks[splineIndex];

        if (pTrack->GetCurveType() != eAnimCurveType_TCBFloat)
        {
            continue;
        }

        for (int i = 0; i < (int)pSpline->GetKeyCount(); i++)
        {
            CTrackViewKeyHandle keyHandle = pTrack->GetKey(i);

            // If the key is selected in any dimension...
            for (
                int nCurrentDimension = 0;
                nCurrentDimension < pSpline->GetNumDimensions();
                nCurrentDimension++
                )
            {
                if (IsKeySelected(pSpline, i, nCurrentDimension))
                {
                    ITcbKey key;
                    keyHandle.GetKey(&key);
                    // tension
                    key.tens += d_tension;
                    if (key.tens > 1)
                    {
                        key.tens = 1.0f;
                    }
                    else if (key.tens < -1)
                    {
                        key.tens = -1.0f;
                    }
                    // continuity
                    key.cont += d_continuity;
                    if (key.cont > 1)
                    {
                        key.cont = 1.0f;
                    }
                    else if (key.cont < -1)
                    {
                        key.cont = -1.0f;
                    }
                    // bias
                    key.bias += d_bias;
                    if (key.bias > 1)
                    {
                        key.bias = 1.0f;
                    }
                    else if (key.bias < -1)
                    {
                        key.bias = -1.0f;
                    }
                    keyHandle.SetKey(&key);
                    OnUserCommand(ID_TANGENT_AUTO);
                    break;
                }
            }
        }
    }

    SendNotifyEvent(SPLN_CHANGE);
    update();
}

void CTrackViewSplineCtrl::OnUserCommand(UINT cmd)
{
    if (cmd == ID_TANGENT_UNIFY)
    {
        if (IsUnifiedKeyCurrentlySelected() == false)
        {
            ModifySelectedKeysFlags(SPLINE_KEY_TANGENT_ALL_MASK, SPLINE_KEY_TANGENT_UNIFIED);
        }
        else
        {
            ModifySelectedKeysFlags(SPLINE_KEY_TANGENT_ALL_MASK, SPLINE_KEY_TANGENT_BROKEN);
        }
    }
    else if (cmd == ID_FREEZE_KEYS)
    {
        m_bKeysFreeze = !m_bKeysFreeze;
    }
    else if (cmd == ID_FREEZE_TANGENTS)
    {
        m_bTangentsFreeze = !m_bTangentsFreeze;
    }
    else
    {
        SplineWidget::OnUserCommand(cmd);
    }
}

bool CTrackViewSplineCtrl::IsUnifiedKeyCurrentlySelected() const
{
    for (size_t splineIndex = 0, splineCount = m_splines.size(); splineIndex < splineCount; ++splineIndex)
    {
        ISplineInterpolator* pSpline = m_splines[splineIndex].pSpline;

        if (pSpline == nullptr)
        {
            continue;
        }

        for (int i = 0; i < (int)pSpline->GetKeyCount(); i++)
        {
            // If the key is selected in any dimension...
            for (
                int nCurrentDimension = 0;
                nCurrentDimension < pSpline->GetNumDimensions();
                nCurrentDimension++
                )
            {
                if (IsKeySelected(pSpline, i, nCurrentDimension))
                {
                    int flags = pSpline->GetKeyFlags(i);
                    if ((flags & SPLINE_KEY_TANGENT_ALL_MASK) != SPLINE_KEY_TANGENT_UNIFIED)
                    {
                        return false;
                    }
                }
            }
        }
    }

    return true;
}

void CTrackViewSplineCtrl::ClearSelection()
{
    // In this case, we should deselect all keys, even ones in other tracks.
    // So this overriding is necessary.
    CTrackViewSequence* pSequence = GetIEditor()->GetAnimation()->GetSequence();
    if (pSequence)
    {
        pSequence->DeselectAllKeys();
    }
}

ISplineCtrlUndo* CTrackViewSplineCtrl::CreateSplineCtrlUndoObject(std::vector<ISplineInterpolator*>& splineContainer)
{
    return new CUndoTrackViewSplineCtrl(this, splineContainer);
}

void CTrackViewSplineCtrl::mousePressEvent(QMouseEvent* event)
{
    if (GetIEditor()->GetAnimation()->GetSequence())
    {
        SplineWidget::mousePressEvent(event);
        if (m_editMode == TimeMarkerMode)
        {
            // turn off recording when dragging time
            m_stashedRecordModeWhenDraggingTime = GetIEditor()->GetAnimation()->IsRecordMode();
            GetIEditor()->GetAnimation()->SetRecording(false);
        }
    }
}

void CTrackViewSplineCtrl::mouseReleaseEvent(QMouseEvent* event)
{
    if (GetIEditor()->GetAnimation()->GetSequence())
    {
        bool restoreRecordModeToTrue = (m_editMode == TimeMarkerMode && m_stashedRecordModeWhenDraggingTime);

        SplineWidget::mouseReleaseEvent(event);

        if (restoreRecordModeToTrue)
        {
            // restore recording when dragging time
            GetIEditor()->GetAnimation()->SetRecording(true);
            m_stashedRecordModeWhenDraggingTime = false;    // reset stashed value
        }
    }
}

void CTrackViewSplineCtrl::mouseDoubleClickEvent(QMouseEvent* event)
{
    CTrackViewSequence* sequence = GetIEditor()->GetAnimation()->GetSequence();
    if (sequence)
    {
        SplineWidget::mouseDoubleClickEvent(event);

        if (m_hitCode == HIT_SPLINE)
        {
            // HIT_SPLINE results in an added key that gets selected.
            // Use the selected keys to notify the sequence, key by key, of newly added keys
            CTrackViewKeyBundle addedKeys = sequence->GetSelectedKeys();
            for (int keyIdx = addedKeys.GetKeyCount(); --keyIdx >= 0; )
            {
                CTrackViewKeyHandle addedKey = addedKeys.GetKey(keyIdx);
                sequence->OnKeyAdded(addedKey);
            }
        }
    }
}

void CTrackViewSplineCtrl::keyPressEvent(QKeyEvent* event)
{
    auto sequence = GetIEditor()->GetAnimation()->GetSequence();
    if (sequence)
    {
        // HAVE TO INCLUDE CASES FOR THESE IN THE ShortcutOverride handler in ::event() below
        if (event->key() == Qt::Key_S && m_playCallback)
        {
            m_playCallback();
        }
        else if (event->matches(QKeySequence::Delete))
        {
            CUndo undo("Delete Keys");

            SendNotifyEvent(SPLN_BEFORE_CHANGE);
            sequence->DeleteSelectedKeys();
            SendNotifyEvent(SPLN_CHANGE);
            update();
        }
        else if (event->matches(QKeySequence::Undo))
        {
            GetIEditor()->Undo();
        }
        else if (event->matches(QKeySequence::Redo))
        {
            GetIEditor()->Redo();
        }
        else
        {
            SplineWidget::keyPressEvent(event);
        }
    }
}

bool CTrackViewSplineCtrl::event(QEvent* e)
{
    if (e->type() == QEvent::ShortcutOverride)
    {
        // since we respond to the following things, let Qt know so that shortcuts don't override us
        bool respondsToEvent = false;

        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(e);
        if (keyEvent->key() == Qt::Key_S)
        {
            respondsToEvent = true;
        }
        else
        {
            respondsToEvent = keyEvent->matches(QKeySequence::Delete) ||
                keyEvent->matches(QKeySequence::Undo) ||
                keyEvent->matches(QKeySequence::Redo);
        }

        if (respondsToEvent)
        {
            e->accept();
            return true;
        }
    }

    return SplineWidget::event(e);
}

void CTrackViewSplineCtrl::wheelEvent(QWheelEvent* event)
{
    if (GetIEditor()->GetAnimation()->GetSequence())
    {
        SplineWidget::wheelEvent(event);
    }
}

void CTrackViewSplineCtrl::SelectKey(ISplineInterpolator* pSpline, int nKey, int nDimension, bool bSelect)
{
    SplineWidget::SelectKey(pSpline, nKey, nDimension, bSelect);
    GetIEditor()->GetAnimation()->GetSequence()->OnKeySelectionChanged();
}

void CTrackViewSplineCtrl::SelectRectangle(const QRect& rc, bool bSelect)
{
    SplineWidget::SelectRectangle(rc, bSelect);
    GetIEditor()->GetAnimation()->GetSequence()->OnKeySelectionChanged();
}

void CTrackViewSplineCtrl::SetPlayCallback(const std::function<void()>& callback)
{
    m_playCallback = callback;
}
