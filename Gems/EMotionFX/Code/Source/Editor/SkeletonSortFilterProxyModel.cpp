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

#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <Source/Editor/SkeletonSortFilterProxyModel.h>
#include <Source/Editor/SelectionProxyModel.h>
#include <Source/Editor/SkeletonModel.h>
#include <QItemSelectionModel>


namespace EMotionFX
{
    const char* SkeletonSortFilterProxyModel::s_actorCategory = "Actor";
    const char* SkeletonSortFilterProxyModel::s_simulationCategory = "Simulation";

    const char* SkeletonSortFilterProxyModel::s_bonesFilterName = "Joints that influence skin";
    const char* SkeletonSortFilterProxyModel::s_meshesFilterName = "Meshes";
    const char* SkeletonSortFilterProxyModel::s_ragdollNodesFilterName = "Ragdoll joints and colliders";
    const char* SkeletonSortFilterProxyModel::s_hitDetectionNodesFilterName = "Hit detection colliders";
    const char* SkeletonSortFilterProxyModel::s_clothFilterName = "Cloth colliders";

    SkeletonSortFilterProxyModel::SkeletonSortFilterProxyModel(SkeletonModel* sourceSkeletonModel, QItemSelectionModel* sourceSelectionModel, QObject* parent)
        : QSortFilterProxyModel(parent)
        , m_recursiveMode(true)
        , m_actor(nullptr)
    {
        m_filterFlags[SHOW_BONES] = false;
        m_filterFlags[SHOW_MESHES] = false;
        m_filterFlags[SHOW_RAGDOLLJOINTS] = false;
        m_filterFlags[SHOW_HITDETECTIONJOINTS] = false;
        m_filterFlags[SHOW_CLOTHCOLLIDERJOINTS] = false;

        setSourceModel(sourceSkeletonModel);
        m_selectionProxyModel = new SelectionProxyModel(sourceSelectionModel, this, parent);
    }

    void SkeletonSortFilterProxyModel::SetFilterRecursiveMode(bool enabled)
    {
        if (m_recursiveMode != enabled)
        {
            m_recursiveMode = enabled;
            invalidate();
        }
    }

    void SkeletonSortFilterProxyModel::SetFilterFlag(Filter filter, bool enabled)
    {
        if (m_filterFlags[filter] != enabled)
        {
            m_filterFlags[filter] = enabled;
            invalidate();
        }
    }

    bool SkeletonSortFilterProxyModel::GetFilterFlag(Filter filter) const
    {
        return m_filterFlags[filter];
    }

    bool SkeletonSortFilterProxyModel::AllFiltersDisabled() const
    {
        for (int i = 0; i < FILTER_COUNT; ++i)
        {
            if (m_filterFlags[i] == true)
            {
                return false;
            }
        }

        return true;
    }

    void SkeletonSortFilterProxyModel::ConnectFilterWidget(AzQtComponents::FilteredSearchWidget* filterWidget)
    {
        filterWidget->AddTypeFilter(s_actorCategory, s_bonesFilterName);
        filterWidget->AddTypeFilter(s_actorCategory, s_meshesFilterName);

        filterWidget->AddTypeFilter(s_simulationCategory, s_ragdollNodesFilterName);
        filterWidget->AddTypeFilter(s_simulationCategory, s_hitDetectionNodesFilterName);
        filterWidget->AddTypeFilter(s_simulationCategory, s_clothFilterName);

        connect(filterWidget, &AzQtComponents::FilteredSearchWidget::TextFilterChanged, this, &SkeletonSortFilterProxyModel::OnTextFilterChanged);
        connect(filterWidget, &AzQtComponents::FilteredSearchWidget::TypeFilterChanged, this, &SkeletonSortFilterProxyModel::OnTypeFilterChanged);
    }

    void SkeletonSortFilterProxyModel::OnTextFilterChanged(const QString& text)
    {
        setFilterWildcard(text);
    }

    void SkeletonSortFilterProxyModel::OnTypeFilterChanged(const AzQtComponents::SearchTypeFilterList& activeTypeFilters)
    {
        for (int i = 0; i < FILTER_COUNT; ++i)
        {
            m_filterFlags[i] = false;
        }

        for (const AzQtComponents::SearchTypeFilter& searchTypeFilter : activeTypeFilters)
        {
            if (searchTypeFilter.displayName == s_bonesFilterName)
            {
                m_filterFlags[SHOW_BONES] = true;
            }
            else if (searchTypeFilter.displayName == s_meshesFilterName)
            {
                m_filterFlags[SHOW_MESHES] = true;
            }
            else if (searchTypeFilter.displayName == s_ragdollNodesFilterName)
            {
                m_filterFlags[SHOW_RAGDOLLJOINTS] = true;
            }
            else if (searchTypeFilter.displayName == s_hitDetectionNodesFilterName)
            {
                m_filterFlags[SHOW_HITDETECTIONJOINTS] = true;
            }
            else if (searchTypeFilter.displayName == s_clothFilterName)
            {
                m_filterFlags[SHOW_CLOTHCOLLIDERJOINTS] = true;
            }
        }

        invalidate();
    }

    bool SkeletonSortFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
    {
        // Do not use sourceParent->child because an invalid parent does not produce valid children (which our index function does)
        QModelIndex sourceIndex = sourceModel()->index(sourceRow, 0, sourceParent);
        if (!sourceIndex.isValid())
        {
            return false;
        }

        bool shouldShow = false;

        const Node* node = sourceIndex.data(SkeletonModel::ROLE_POINTER).value<Node*>();
        const Actor* actor = sourceIndex.data(SkeletonModel::ROLE_ACTOR_POINTER).value<Actor*>();
        const ActorInstance* actorInstance = sourceIndex.data(SkeletonModel::ROLE_ACTOR_INSTANCE_POINTER).value<ActorInstance*>();

        if (AllFiltersDisabled())
        {
            shouldShow = true;
        }
        else
        {
            if (m_filterFlags[SHOW_BONES] && sourceIndex.data(SkeletonModel::ROLE_BONE).value<bool>())
            {
                shouldShow = true;
            }

            if (m_filterFlags[SHOW_MESHES] && sourceIndex.data(SkeletonModel::ROLE_HASMESH).value<bool>())
            {
                shouldShow = true;
            }

            if (m_filterFlags[SHOW_RAGDOLLJOINTS] && sourceIndex.data(SkeletonModel::ROLE_RAGDOLL).value<bool>())
            {
                shouldShow = true;
            }

            if (m_filterFlags[SHOW_HITDETECTIONJOINTS] && sourceIndex.data(SkeletonModel::ROLE_HITDETECTION).value<bool>())
            {
                shouldShow = true;
            }

            if (m_filterFlags[SHOW_CLOTHCOLLIDERJOINTS] && sourceIndex.data(SkeletonModel::ROLE_CLOTH).value<bool>())
            {
                shouldShow = true;
            }
        }

        if (shouldShow && !QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent))
        {
            shouldShow = false;
        }

        // RecuriveMode overrides the "shouldShow" with the children's state. Meaning, if one child is shown then
        // the parent is shown as well.
        if (!shouldShow && m_recursiveMode)
        {
            // Qt5.10 includes an option for the QSortFilterProxyModel to be recursively filtering
            // Once we move to that Qt version we can remove this method/class
            if (sourceIndex.isValid())
            {
                const int rows = sourceModel()->rowCount(sourceIndex);
                for (int i = 0; i < rows; ++i)
                {
                    if (filterAcceptsRow(i, sourceIndex))
                    {
                        shouldShow = true;
                        break;
                    }
                }
            }
        }

        return shouldShow;
    }
} // namespace EMotionFX
