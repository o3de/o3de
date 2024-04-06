/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "UiEditorAnimationBus.h"
#include "EditorDefs.h"
#include "Editor/Resource.h"
#include "UiAnimViewSequenceManager.h"
#include "UiAnimViewSplineCtrl.h"
#include "UiAnimViewSequence.h"
#include "UiAnimViewDialog.h"
#include "UiAnimViewUndo.h"
#include "UiAnimViewTrack.h"

#include <QMouseEvent>
#include <QRubberBand>

class CUndoUiAnimViewSplineCtrl
    : public ISplineCtrlUndo
    , public CUndoAnimKeySelection
{
public:
    CUndoUiAnimViewSplineCtrl(CUiAnimViewSplineCtrl* pCtrl, std::vector<ISplineInterpolator*>& splineContainer)
        : CUndoAnimKeySelection(CUiAnimViewSequenceManager::GetSequenceManager()->GetAnimationContext()->GetSequence())
    {
        m_sequenceName = QString::fromUtf8(CUiAnimViewSequenceManager::GetSequenceManager()->GetAnimationContext()->GetSequence()->GetName().c_str());

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
            CUiAnimViewTrack* pTrack = m_pCtrl->m_tracks[trackIndex];
            if (pTrack->GetSpline() == pSpline)
            {
                CSplineEntry entry;
                entry.m_pTrack = pTrack;
                m_splineEntries.push_back(entry);
            }
        }
    }

    virtual int GetSize() { return sizeof(*this); }
    virtual const char* GetDescription() { return "UndoUiAnimViewSplineCtrl"; };

    virtual void Undo(bool bUndo)
    {
        CUiAnimViewSplineCtrl* pCtrl = FindControl(m_pCtrl);
        if (pCtrl)
        {
            pCtrl->SendNotifyEvent(SPLN_BEFORE_CHANGE);
        }

        const CUiAnimViewSequenceManager* pSequenceManager = CUiAnimViewSequenceManager::GetSequenceManager();
        CUiAnimViewSequence* pSequence = pSequenceManager->GetSequenceByName(m_sequenceName);

        assert(pSequence);
        if (!pSequence)
        {
            return;
        }

        CUiAnimViewSequenceNotificationContext context(pSequence);

        if (bUndo)
        {
            // Save key selection states for redo if necessary
            m_redoKeyStates = SaveKeyStates(pSequence);
            SerializeSplines(&CSplineEntry::m_redo, false);
        }

        SerializeSplines(&CSplineEntry::m_undo, true);

        // Undo key selection state
        RestoreKeyStates(pSequence, m_undoKeyStates);

        if (pCtrl && bUndo)
        {
            pCtrl->m_bKeyTimesDirty = true;
            pCtrl->SendNotifyEvent(SPLN_CHANGE);
            pCtrl->update();
        }

        if (bUndo)
        {
            pSequence->OnKeySelectionChanged();
        }
    }

    virtual void Redo()
    {
        const CUiAnimViewSequenceManager* pSequenceManager = CUiAnimViewSequenceManager::GetSequenceManager();
        CUiAnimViewSequence* pSequence = pSequenceManager->GetSequenceByName(m_sequenceName);
        assert(pSequence);
        if (!pSequence)
        {
            return;
        }

        CUiAnimViewSequenceNotificationContext context(pSequence);

        CUiAnimViewSplineCtrl* pCtrl = FindControl(m_pCtrl);

        if (pCtrl)
        {
            pCtrl->SendNotifyEvent(SPLN_BEFORE_CHANGE);
        }
        SerializeSplines(&CSplineEntry::m_redo, true);

        // Redo key selection state
        RestoreKeyStates(pSequence, m_redoKeyStates);

        if (pCtrl)
        {
            pCtrl->m_bKeyTimesDirty = true;
            pCtrl->SendNotifyEvent(SPLN_CHANGE);
            pCtrl->update();
        }

        pSequence->OnKeySelectionChanged();
    }

    virtual bool IsSelectionChanged() const
    {
        return CUndoAnimKeySelection::IsSelectionChanged();
    }

public:
    typedef std::list<CUiAnimViewSplineCtrl*> CUiAnimViewSplineCtrls;

    static CUiAnimViewSplineCtrl* FindControl(CUiAnimViewSplineCtrl* pCtrl)
    {
        if (!pCtrl)
        {
            return 0;
        }

        auto iter = std::find(s_activeCtrls.begin(), s_activeCtrls.end(), pCtrl);
        if (iter == s_activeCtrls.end())
        {
            return 0;
        }

        return *iter;
    }

    static void RegisterControl(CUiAnimViewSplineCtrl* pCtrl)
    {
        if (!FindControl(pCtrl))
        {
            s_activeCtrls.push_back(pCtrl);
        }
    }

    static void UnregisterControl(CUiAnimViewSplineCtrl* pCtrl)
    {
        if (FindControl(pCtrl))
        {
            s_activeCtrls.remove(pCtrl);
        }
    }

    static CUiAnimViewSplineCtrls s_activeCtrls;

private:
    class CSplineEntry
    {
    public:
        _smart_ptr<ISplineBackup> m_undo;
        _smart_ptr<ISplineBackup> m_redo;
        CUiAnimViewTrack* m_pTrack;
    };

    void SerializeSplines(_smart_ptr<ISplineBackup> CSplineEntry::* backup, bool bLoading)
    {
        for (auto it = m_splineEntries.begin(); it != m_splineEntries.end(); ++it)
        {
            CSplineEntry& entry = *it;
            ISplineInterpolator* pSpline = entry.m_pTrack->GetSpline();

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

    QString m_sequenceName;
    CUiAnimViewSplineCtrl* m_pCtrl;
    std::vector<CSplineEntry> m_splineEntries;
    std::vector<float> m_keyTimes;
};

CUndoUiAnimViewSplineCtrl::CUiAnimViewSplineCtrls CUndoUiAnimViewSplineCtrl::s_activeCtrls;

//////////////////////////////////////////////////////////////////////////
CUiAnimViewSplineCtrl::CUiAnimViewSplineCtrl(QWidget* parent)
    : SplineWidget(parent)
    , m_bKeysFreeze(false)
    , m_bTangentsFreeze(false)
{
    CUndoUiAnimViewSplineCtrl::RegisterControl(this);
}

CUiAnimViewSplineCtrl::~CUiAnimViewSplineCtrl()
{
    CUndoUiAnimViewSplineCtrl::UnregisterControl(this);
}

bool CUiAnimViewSplineCtrl::GetTangentHandlePts(QPoint& inTangentPt, QPoint& pt, QPoint& outTangentPt,
    int nSpline, int nKey, int nDimension)
{
    ISplineInterpolator* pSpline = m_splines[nSpline].pSpline;
    CUiAnimViewTrack* pTrack = m_tracks[nSpline];

    float time = pSpline->GetKeyTime(nKey);

    ISplineInterpolator::ValueType value, tin, tout;
    ISplineInterpolator::ZeroValue(value);
    pSpline->GetKeyValue(nKey, value);
    pSpline->GetKeyTangents(nKey, tin, tout);

    CUiAnimViewKeyHandle keyHandle = pTrack->GetKey(nKey);

    if (pTrack->GetCurveType() == eUiAnimCurveType_TCBFloat)
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
        assert(pTrack->GetCurveType() == eUiAnimCurveType_BezierFloat);
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

void CUiAnimViewSplineCtrl::ComputeIncomingTangentAndEaseTo(float& ds, float& easeTo, QPoint inTangentPt,
    int nSpline, int nKey, int nDimension)
{
    ISplineInterpolator* pSpline = m_splines[nSpline].pSpline;
    CUiAnimViewTrack* pTrack = m_tracks[nSpline];

    float time = pSpline->GetKeyTime(nKey);

    ISplineInterpolator::ValueType value, tin, tout;
    ISplineInterpolator::ZeroValue(value);
    pSpline->GetKeyValue(nKey, value);
    pSpline->GetKeyTangents(nKey, tin, tout);

    CUiAnimViewKeyHandle keyHandle = pTrack->GetKey(nKey);

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

void CUiAnimViewSplineCtrl::ComputeOutgoingTangentAndEaseFrom(float& dd, float& easeFrom, QPoint outTangentPt,
    int nSpline, int nKey, int nDimension)
{
    ISplineInterpolator* pSpline = m_splines[nSpline].pSpline;
    CUiAnimViewTrack* pTrack = m_tracks[nSpline];

    float time = pSpline->GetKeyTime(nKey);

    ISplineInterpolator::ValueType value, tin, tout;
    ISplineInterpolator::ZeroValue(value);
    pSpline->GetKeyValue(nKey, value);
    pSpline->GetKeyTangents(nKey, tin, tout);

    CUiAnimViewKeyHandle keyHandle = pTrack->GetKey(nKey);

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

void CUiAnimViewSplineCtrl::AddSpline(ISplineInterpolator* pSpline, CUiAnimViewTrack* pTrack, const QColor& color)
{
    QColor colorArray[4];
    colorArray[0] = colorArray[1] = colorArray[2] = colorArray[3] = color;
    AddSpline(pSpline, pTrack, colorArray);
}

void CUiAnimViewSplineCtrl::AddSpline(ISplineInterpolator* pSpline, CUiAnimViewTrack* pTrack, QColor anColorArray[4])
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
    si.pDetailSpline = NULL;
    m_splines.push_back(si);
    m_tracks.push_back(pTrack);
    m_bKeyTimesDirty = true;
    update();
}

void CUiAnimViewSplineCtrl::RemoveAllSplines()
{
    m_tracks.clear();
    SplineWidget::RemoveAllSplines();
}

void CUiAnimViewSplineCtrl::MoveSelectedTangentHandleTo(const QPoint& point)
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

    CUiAnimViewTrack* pTrack = m_tracks[splineIndex];
    CUiAnimViewKeyHandle keyHandle = pTrack->GetKey(m_nHitKeyIndex);

    if (pTrack->GetCurveType() == eUiAnimCurveType_TCBFloat)
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
        assert(pTrack->GetCurveType() == eUiAnimCurveType_BezierFloat);
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

void CUiAnimViewSplineCtrl::mouseMoveEvent(QMouseEvent* event)
{
    const QPoint point = event->pos();
    CUiAnimViewSequence* pSequence = nullptr;
    UiEditorAnimationBus::BroadcastResult(pSequence, &UiEditorAnimationBus::Events::GetCurrentSequence);
    if (!pSequence)
    {
        return;
    }

    CUiAnimViewSequenceNotificationContext context(pSequence);

    m_cMousePos = point;

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
            UiAnimUndoManager::Get()->Restore();
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
            CUiAnimViewTrack* pTrack = m_tracks[splineIndex];
            for (int i = 0; i < pSpline->GetKeyCount(); i++)
            {
                CUiAnimViewKeyHandle keyHandle = pTrack->GetKey(i);

                for (int nCurrentDimension = 0; nCurrentDimension < pSpline->GetNumDimensions(); nCurrentDimension++)
                {
                    if (pSpline->IsKeySelectedAtDimension(i, nCurrentDimension))
                    {
                        time = pSpline->GetKeyTime(i);
                        pSpline->GetKeyValue(i, afValue);
                        if (pTrack->GetCurveType() == eUiAnimCurveType_TCBFloat)
                        {
                            ITcbKey key;
                            keyHandle.GetKey(&key);
                            tipText = QStringLiteral("t=%1  v=%2 / T=%3  C=%4  B=%5")
                                .arg(time * m_fTooltipScaleX, 0, 'f', 3).arg(afValue[nCurrentDimension] * m_fTooltipScaleY, 2, 'f', 3)
                                .arg(key.tens, 0, 'f', 3).arg(key.cont, 0, 'f', 3).arg(key.bias, 0, 'f', 3);
                        }
                        else
                        {
                            assert(pTrack->GetCurveType() == eUiAnimCurveType_BezierFloat);
                            ISplineInterpolator::ValueType tin, tout;
                            pSpline->GetKeyTangents(i, tin, tout);
                            tipText = QStringLiteral("t=%1  v=%2 / tin=(%3,%4)  tout=(%5,%6)")
                                .arg(time * m_fTooltipScaleX, 0, 'f', 3).arg(afValue[0] * m_fTooltipScaleY, 2, 'f', 3)
                                .arg(tin[0], 0, 'f', 3).arg(tin[1], 2, 'f', 3).arg(tout[0], 0, 'f', 3).arg(tout[1], 2, 'f', 3);
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
        const float ofsx = m_grid.origin.GetX() - (point.x() - m_cMouseDownPos.x()) / m_grid.zoom.GetX();
        const float ofsy = m_grid.origin.GetY() + (point.y() - m_cMouseDownPos.y()) / m_grid.zoom.GetY();
        SetScrollOffset(Vec2(ofsx, ofsy));
        m_cMouseDownPos = point;
    }
    break;

    case ZoomMode:
    {
        const float ofsx = (point.x() - m_cMouseDownPos.x()) * 0.01f;
        const float ofsy = (point.y() - m_cMouseDownPos.y()) * 0.01f;

        AZ::Vector2 z = m_grid.zoom;
        if (ofsx != 0)
        {
            z.SetX(max(z.GetX() * (1.0f + ofsx), 0.001f));
        }
        if (ofsy != 0)
        {
            z.SetY(max(z.GetY() * (1.0f + ofsy), 0.001f));
        }
        SetZoom(Vec2(z.GetX(), z.GetY()), m_cMouseDownPos);
        m_cMouseDownPos = point;
    }
    break;
    }
}

void CUiAnimViewSplineCtrl::AdjustTCB(float d_tension, float d_continuity, float d_bias)
{
    UiAnimUndo undo("Modify Spline Keys TCB");
    ConditionalStoreUndo();

    SendNotifyEvent(SPLN_BEFORE_CHANGE);

    for (size_t splineIndex = 0, splineCount = m_splines.size(); splineIndex < splineCount; ++splineIndex)
    {
        ISplineInterpolator* pSpline = m_splines[splineIndex].pSpline;
        CUiAnimViewTrack* pTrack = m_tracks[splineIndex];

        if (pTrack->GetCurveType() != eUiAnimCurveType_TCBFloat)
        {
            continue;
        }

        for (int i = 0; i < (int)pSpline->GetKeyCount(); i++)
        {
            CUiAnimViewKeyHandle keyHandle = pTrack->GetKey(i);

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

void CUiAnimViewSplineCtrl::OnUserCommand(UINT cmd)
{
    CUiAnimViewSequence* pSequence = nullptr;
    UiEditorAnimationBus::BroadcastResult(pSequence, &UiEditorAnimationBus::Events::GetCurrentSequence);
    if (!pSequence)
    {
        return; // no active sequence
    }

    if (cmd == ID_TANGENT_UNIFY)
    {
        // do nothing if there are no splines
        if (m_splines.size() == 0)
        {
            return;
        }

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

bool CUiAnimViewSplineCtrl::IsUnifiedKeyCurrentlySelected() const
{
    for (size_t splineIndex = 0, splineCount = m_splines.size(); splineIndex < splineCount; ++splineIndex)
    {
        ISplineInterpolator* pSpline = m_splines[splineIndex].pSpline;

        if (!pSpline)
        {
            continue;
        }

        for (int i = 0; i < pSpline->GetKeyCount(); i++)
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

void CUiAnimViewSplineCtrl::ClearSelection()
{
    // In this case, we should deselect all keys, even ones in other tracks.
    // So this overriding is necessary.
    CUiAnimViewSequence* pSequence = nullptr;
    UiEditorAnimationBus::BroadcastResult(pSequence, &UiEditorAnimationBus::Events::GetCurrentSequence);
    if (pSequence)
    {
        pSequence->DeselectAllKeys();
    }
}

ISplineCtrlUndo* CUiAnimViewSplineCtrl::CreateSplineCtrlUndoObject(std::vector<ISplineInterpolator*>& splineContainer)
{
    return new CUndoUiAnimViewSplineCtrl(this, splineContainer);
}

void CUiAnimViewSplineCtrl::mousePressEvent(QMouseEvent* event)
{
    CUiAnimViewSequence* pSequence = nullptr;
    UiEditorAnimationBus::BroadcastResult(pSequence, &UiEditorAnimationBus::Events::GetCurrentSequence);
    if (pSequence)
    {
        SplineWidget::mousePressEvent(event);
    }
}

void CUiAnimViewSplineCtrl::mouseReleaseEvent(QMouseEvent* event)
{
    CUiAnimViewSequence* pSequence = nullptr;
    UiEditorAnimationBus::BroadcastResult(pSequence, &UiEditorAnimationBus::Events::GetCurrentSequence);
    if (pSequence)
    {
        SplineWidget::mouseReleaseEvent(event);
    }
}

void CUiAnimViewSplineCtrl::mouseDoubleClickEvent(QMouseEvent* event)
{
    CUiAnimViewSequence* pSequence = nullptr;
    UiEditorAnimationBus::BroadcastResult(pSequence, &UiEditorAnimationBus::Events::GetCurrentSequence);
    if (pSequence)
    {
        SplineWidget::mouseDoubleClickEvent(event);
    }
}

void CUiAnimViewSplineCtrl::keyPressEvent(QKeyEvent* event)
{
    CUiAnimViewSequence* pSequence = nullptr;
    UiEditorAnimationBus::BroadcastResult(pSequence, &UiEditorAnimationBus::Events::GetCurrentSequence);
    if (pSequence)
    {
        SplineWidget::keyPressEvent(event);

        if (event->key() == Qt::Key_S && m_playCallback)
        {
            m_playCallback();
        }
    }
}

void CUiAnimViewSplineCtrl::wheelEvent(QWheelEvent* event)
{
    CUiAnimViewSequence* pSequence = nullptr;
    UiEditorAnimationBus::BroadcastResult(pSequence, &UiEditorAnimationBus::Events::GetCurrentSequence);
    if (pSequence)
    {
        SplineWidget::wheelEvent(event);
    }
}

void CUiAnimViewSplineCtrl::SelectKey(ISplineInterpolator* pSpline, int nKey, int nDimension, bool bSelect)
{
    SplineWidget::SelectKey(pSpline, nKey, nDimension, bSelect);
    CUiAnimViewSequence* pSequence = nullptr;
    UiEditorAnimationBus::BroadcastResult(pSequence, &UiEditorAnimationBus::Events::GetCurrentSequence);
    pSequence->OnKeySelectionChanged();
}

void CUiAnimViewSplineCtrl::SelectRectangle(const QRect& rc, bool bSelect)
{
    SplineWidget::SelectRectangle(rc, bSelect);
    CUiAnimViewSequence* pSequence = nullptr;
    UiEditorAnimationBus::BroadcastResult(pSequence, &UiEditorAnimationBus::Events::GetCurrentSequence);
    pSequence->OnKeySelectionChanged();
}

void CUiAnimViewSplineCtrl::SetPlayCallback(const std::function<void()>& callback)
{
    m_playCallback = callback;
}
