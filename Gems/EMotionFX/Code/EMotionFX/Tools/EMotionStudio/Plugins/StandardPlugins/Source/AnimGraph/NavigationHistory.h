/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#if !defined(Q_MOC_RUN)
#include <MCore/Source/StandardHeaders.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/StandardPluginsConfig.h>
#include <QObject>
#endif

namespace EMStudio
{
    class AnimGraphModel;
    
    // Handles history of the graph's navigation (back and forward)
    // The history affects the focus, every time focus changes, the new focus is added
    // to the history. Going back changes back the focus to the previous focus in the
    // history.
    //
    class NavigationHistory
        : public QObject
    {
        Q_OBJECT

    public:
        NavigationHistory(AnimGraphModel& animGrahModel);
        ~NavigationHistory();

        bool CanStepBackward() const;
        bool CanStepForward() const;

        void StepBackward();
        void StepForward();

    signals:
        void ChangedSteppingLimits();

    private slots:
        void OnFocusChanged(const QModelIndex& newFocusIndex, const QModelIndex& newFocusParent, const QModelIndex& oldFocusIndex, const QModelIndex& oldFocusParent);
        void OnRowsRemoved(const QModelIndex &parent, int first, int last);

    private:
        AnimGraphModel& m_animGraphModel;
        AZStd::vector<QPersistentModelIndex> m_navigationHistory;
        int m_navigationIndex;

        // Indicates if the focus was changed by stepping through history. 
        // This is used to avoid processing the OnFocusChanged callback
        bool m_localStepping;

        static const size_t s_maxHistoryEntries = 256;
    };

} // namespace EMStudio
