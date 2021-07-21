/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphSortFilterProxyModel.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphModel.h>

namespace EMStudio
{

    AnimGraphSortFilterProxyModel::AnimGraphSortFilterProxyModel(QObject* parent)
        : QSortFilterProxyModel(parent)
        , m_recursiveMode(true)
        , m_disableSelectionForFiltered(false)
        , m_showStatesOnly(false)
        , m_showNodesOnly(true)
    {}

    void AnimGraphSortFilterProxyModel::setFilterRecursiveMode(bool enabled)
    {
        if (m_recursiveMode != enabled)
        {
            m_recursiveMode = enabled;
            invalidate();
        }
    }

    void AnimGraphSortFilterProxyModel::setDisableSelectionForFilteredButShowedElements(bool enabled)
    {
        if (m_disableSelectionForFiltered != enabled)
        {
            m_disableSelectionForFiltered = enabled;
            invalidate();
        }
    }

    void AnimGraphSortFilterProxyModel::setFilterStatesOnly(bool enabled)
    {
        if (m_showStatesOnly != enabled)
        {
            m_showStatesOnly = enabled;
            invalidate();
        }
    }

    void AnimGraphSortFilterProxyModel::setFilterNodeTypes(const AZStd::unordered_set<AZ::TypeId>& filterNodeTypes)
    {
        if (m_filterNodeTypes != filterNodeTypes)
        {
            m_filterNodeTypes = filterNodeTypes;
            invalidate();
        }
    }

    void AnimGraphSortFilterProxyModel::setNonFilterableIndex(const QModelIndex& sourceIndex)
    {
        if (m_nonFilterableSourceIndex != sourceIndex)
        {
            m_nonFilterableSourceIndex = sourceIndex;
            invalidateFilter();
        }
    }

    bool AnimGraphSortFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
    {
        // Do not use sourceParent->child because an invalid parent does not produce valid children (which
        // our index function does)
        QModelIndex sourceIndex = sourceModel()->index(sourceRow, 0, sourceParent);
        if (!sourceIndex.isValid())
        {
            return false;
        }

        return !IsFiltered(sourceIndex, m_recursiveMode);
    }

    bool AnimGraphSortFilterProxyModel::IsFiltered(const QModelIndex& index, bool recursiveMode) const
    {
        if (m_nonFilterableSourceIndex.isValid() && m_nonFilterableSourceIndex == index)
        {
            return false;
        }

        bool filtered = false;
        if (m_showNodesOnly)
        {
            if (index.data(AnimGraphModel::ROLE_MODEL_ITEM_TYPE).value<AnimGraphModel::ModelItemType>() != AnimGraphModel::ModelItemType::NODE)
            {
                filtered = true;
            }
        }

        if (!filtered && m_showStatesOnly)
        {
            const bool canActAsState = index.data(AnimGraphModel::ROLE_NODE_CAN_ACT_AS_STATE).toBool();
            if (!canActAsState)
            {
                filtered = true;
            }
        }

        if (!filtered && !QSortFilterProxyModel::filterAcceptsRow(index.row(), index.parent()))
        {
            filtered = true;
        }

        if (!filtered && !m_filterNodeTypes.empty())
        {
            const AZ::TypeId nodeTypeId = index.data(AnimGraphModel::ROLE_RTTI_TYPE_ID).value<AZ::TypeId>();

            if (m_filterNodeTypes.find(nodeTypeId) == m_filterNodeTypes.end())
            {
                filtered = true;
            }
        }

        // RecuriveMode overrides the "filtered" with the children's state. Meaning, if one child is shown then
        // the parent is shown as well.
        if (filtered && recursiveMode)
        {
            // Qt5.10 includes an option for the QSortFilterProxyModel to be recursively filtering
            // Once we move to that Qt version we can remove this method/class
            if (index.isValid())
            {
                const int rows = index.model()->rowCount(index);
                for (int i = 0; i < rows; ++i)
                {
                    if (!IsFiltered(index.model()->index(i, 0, index), recursiveMode))
                    {
                        filtered = false;
                        break;
                    }
                }
            }
        }

        return filtered;
    }

    Qt::ItemFlags AnimGraphSortFilterProxyModel::flags(const QModelIndex &index) const
    {
        QModelIndex sourceIndex = mapToSource(index);
        if (sourceIndex.isValid())
        {
            if (m_disableSelectionForFiltered && IsFiltered(sourceIndex, false))
            {
                return QSortFilterProxyModel::flags(index) & ~Qt::ItemIsSelectable;
            }
        }

        return QSortFilterProxyModel::flags(index);
    }

} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/moc_AnimGraphSortFilterProxyModel.cpp>
