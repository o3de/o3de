/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Deep Selection Header


#ifndef CRYINCLUDE_EDITOR_EDITMODE_DEEPSELECTION_H
#define CRYINCLUDE_EDITOR_EDITMODE_DEEPSELECTION_H
#pragma once

class CBaseObject;

//! Deep Selection
//! Additional output information of HitContext on using "deep selection mode".
//! At the deep selection mode, it supports second selection pass for easy
//! selection on crowded area with two different method.
//! One is to show pop menu of candidate objects list. Another is the cyclic
//! selection on pick clicking.
class CDeepSelection
    : public _i_reference_target_t
{
public:
    //! Deep Selection Mode Definition
    enum EDeepSelectionMode
    {
        DSM_NONE    = 0,    // Not using deep selection.
        DSM_POP     = 1,    // Deep selection mode with pop context menu.
        DSM_CYCLE   = 2     // Deep selection mode with cyclic selection on each clinking same point.
    };

    //! Subclass for container of the selected object with hit distance.
    struct RayHitObject
    {
        RayHitObject(float dist, CBaseObject* pObj)
            : distance(dist)
            , object(pObj)
        {
        }

        float distance;
        CBaseObject* object;
    };

    //! Constructor
    CDeepSelection();
    virtual ~CDeepSelection();

    void Reset(bool bResetLastPick = false);
    void AddObject(float distance, CBaseObject* pObj);
    //! Check if clicking point is same position with last position,
    //! to decide whether to continue cycling mode.
    bool OnCycling (const QPoint& pt);
    //! All objects in list are excluded for hitting test except one, current selection.
    void ExcludeHitTest(int except);
    void SetMode(EDeepSelectionMode mode);
    inline EDeepSelectionMode GetMode() const { return m_Mode; }
    inline EDeepSelectionMode GetPreviousMode() const { return m_previousMode; }
    //! Collect object in the deep selection range. The distance from the minimum
    //! distance is less than deep selection range.
    int CollectCandidate(float fMinDistance, float fRange);
    //! Return the candidate object in index position, then it is to be current
    //! selection position.
    CBaseObject* GetCandidateObject(int index);
    //! Return the current selection position that is update in "GetCandidateObject"
    //! function call.
    inline int GetCurrentSelectPos() const { return m_CurrentSelectedPos; }
    //! Return the number of objects in the deep selection range.
    inline int GetCandidateObjectCount() const { return m_CandidateObjectCount; }

private:
    //! Current mode
    EDeepSelectionMode m_Mode;
    EDeepSelectionMode m_previousMode;
    //! Last picking point to check whether cyclic selection continue.
    QPoint m_LastPickPoint;
    //! List of the selected objects with ray hitting
    std::vector<RayHitObject> m_RayHitObjects;
    int m_CandidateObjectCount;
    int m_CurrentSelectedPos;
};
#endif // CRYINCLUDE_EDITOR_EDITMODE_DEEPSELECTION_H
