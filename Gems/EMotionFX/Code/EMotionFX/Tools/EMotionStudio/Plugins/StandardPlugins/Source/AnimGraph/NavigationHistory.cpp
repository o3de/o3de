/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphModel.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/NavigationHistory.h>


namespace EMStudio
{
    NavigationHistory::NavigationHistory(AnimGraphModel& animGraphModel)
        : m_animGraphModel(animGraphModel)
        , m_navigationIndex(-1)
        , m_localStepping(false)
    {
        connect(&m_animGraphModel, &AnimGraphModel::FocusChanged, this, &NavigationHistory::OnFocusChanged);
        connect(&m_animGraphModel, &AnimGraphModel::rowsRemoved, this, &NavigationHistory::OnRowsRemoved);
    }

    NavigationHistory::~NavigationHistory()
    {
    }

    bool NavigationHistory::CanStepBackward() const
    {
        return (m_navigationIndex > 0);
    }
    
    bool NavigationHistory::CanStepForward() const
    {
        return ((m_navigationIndex + 1) < m_navigationHistory.size());
    }

    void NavigationHistory::StepBackward()
    {
        if (CanStepBackward())
        {
            m_localStepping = true;
            --m_navigationIndex;
            m_animGraphModel.Focus(m_navigationHistory[m_navigationIndex]);
            m_localStepping = false;

            if (m_navigationIndex < 1)
            {
                emit ChangedSteppingLimits();
            }
        }
    }

    void NavigationHistory::StepForward()
    {
        if (CanStepForward())
        {
            m_localStepping = true;
            ++m_navigationIndex;
            m_animGraphModel.Focus(m_navigationHistory[m_navigationIndex]);
            m_localStepping = false;

            if ((m_navigationIndex + 1) == static_cast<int>(m_navigationHistory.size()))
            {
                emit ChangedSteppingLimits();
            }
        }
    }

    void NavigationHistory::OnFocusChanged(const QModelIndex& newFocusIndex, [[maybe_unused]] const QModelIndex& newFocusParent, [[maybe_unused]] const QModelIndex& oldFocusIndex, [[maybe_unused]] const QModelIndex& oldFocusParent)
    {
        if (m_localStepping)
        {
            // Stepping through history, no need to update
            return;
        }

        // check if the current entry is the same as the last one, if so, dont add another one
        int navigationHistoryCount = static_cast<int>(m_navigationHistory.size());
        if (m_navigationIndex >= 0 && m_navigationIndex < navigationHistoryCount && m_navigationHistory[m_navigationIndex] == newFocusIndex)
        {
            return;
        }

        const bool couldStepBackward = CanStepBackward();
        const bool couldStepForward = CanStepForward();

        // If we are adding a new entry and we are not at the last entry, drop all entries after the current one
        if (m_navigationIndex >= 0 && navigationHistoryCount > 0 && (m_navigationIndex + 1) < navigationHistoryCount)
        {
            m_navigationHistory.erase(m_navigationHistory.begin() + m_navigationIndex + 1, m_navigationHistory.end());
            navigationHistoryCount = static_cast<int>(m_navigationHistory.size());
        }

        // if we reached the maximum number of history entries remove the oldest ones
        if (navigationHistoryCount >= s_maxHistoryEntries)
        {
            // Remove from the beginning the amount of entries
            const int amountToRemove = s_maxHistoryEntries - navigationHistoryCount + 1;
            m_navigationHistory.erase(m_navigationHistory.begin(), m_navigationHistory.begin() + amountToRemove);
            navigationHistoryCount = static_cast<int>(m_navigationHistory.size());
            m_navigationIndex -= amountToRemove;
            AZ_Assert(m_navigationIndex >= -1 && m_navigationIndex < m_navigationHistory.size(), "Some calculation went wrong an the navigation index was left out of range");
        }

        // add an entry
        m_navigationHistory.emplace_back(newFocusIndex);
        navigationHistoryCount = static_cast<int>(m_navigationHistory.size());

        // set the index to the last added entry
        m_navigationIndex = navigationHistoryCount - 1;

        if (CanStepBackward() != couldStepBackward || CanStepForward() != couldStepForward)
        {
            emit ChangedSteppingLimits();
        }
    }

    void NavigationHistory::OnRowsRemoved(const QModelIndex &parent, int first, int last)
    {
        AZ_UNUSED(parent);
        AZ_UNUSED(first);
        AZ_UNUSED(last);

        const bool couldStepBackward = CanStepBackward();
        const bool couldStepForward = CanStepForward();

        // Remove invalid items
        AZStd::vector<QPersistentModelIndex>::const_iterator itHistory = m_navigationHistory.begin();
        while (itHistory != m_navigationHistory.end())
        {
            if (itHistory->isValid())
            {
                ++itHistory;
            }
            else
            {
                const int itPosition = static_cast<int>(itHistory - m_navigationHistory.begin());
                if (itPosition < m_navigationIndex)
                {
                    --m_navigationIndex;
                }
                itHistory = m_navigationHistory.erase(itHistory);
            }
        }

        if (CanStepBackward() != couldStepBackward || CanStepForward() != couldStepForward)
        {
            emit ChangedSteppingLimits();
        }
    }

} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/moc_NavigationHistory.cpp>
