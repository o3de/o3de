/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/SelectionProxyModel.h>
#include <QtCore/QSortFilterProxyModel>


namespace EMStudio
{

    SelectionProxyModel::SelectionProxyModel(QItemSelectionModel* sourceSelectionModel, QAbstractProxyModel* proxyModel, QObject* parent)
        : QItemSelectionModel(proxyModel, parent)
        , m_sourceSelectionModel(sourceSelectionModel)
    {
        connect(sourceSelectionModel, &QItemSelectionModel::selectionChanged, this, &SelectionProxyModel::OnSourceSelectionChanged);
        connect(sourceSelectionModel, &QItemSelectionModel::currentChanged, this, &SelectionProxyModel::OnSourceSelectionCurrentChanged);
        
        connect(proxyModel, &QAbstractItemModel::rowsInserted, this, &SelectionProxyModel::OnProxyModelRowsInserted);

        // Find the chain of proxy models
        QAbstractProxyModel* sourceProxyModel = proxyModel;
        while (sourceProxyModel)
        {
            m_proxyModels.emplace_back(sourceProxyModel);
            sourceProxyModel = qobject_cast<QAbstractProxyModel*>(sourceProxyModel->sourceModel());
        }

        const QItemSelection currentSelection = mapFromSource(m_sourceSelectionModel->selection());
        QItemSelectionModel::select(currentSelection, QItemSelectionModel::Current | QItemSelectionModel::ClearAndSelect);

        const QModelIndex currentModelIndex = mapFromSource(m_sourceSelectionModel->currentIndex());
        QItemSelectionModel::setCurrentIndex(currentModelIndex, QItemSelectionModel::Current | QItemSelectionModel::ClearAndSelect);
    }

    SelectionProxyModel::~SelectionProxyModel()
    {}

    void SelectionProxyModel::setCurrentIndex(const QModelIndex &index, QItemSelectionModel::SelectionFlags command)
    {
        const QModelIndex sourcetIndex = mapToSource(index);
        m_sourceSelectionModel->setCurrentIndex(sourcetIndex, command);
    }

    void SelectionProxyModel::select(const QModelIndex &index, QItemSelectionModel::SelectionFlags command)
    {
        const QModelIndex sourceIndex = mapToSource(index);
        m_sourceSelectionModel->select(sourceIndex, command);
    }

    void SelectionProxyModel::select(const QItemSelection &selection, QItemSelectionModel::SelectionFlags command)
    {
        const QItemSelection sourceSelection = mapToSource(selection);
        m_sourceSelectionModel->select(sourceSelection, command);
    }

    void SelectionProxyModel::clear()
    {
        m_sourceSelectionModel->clear();
    }

    void SelectionProxyModel::reset()
    {
        m_sourceSelectionModel->reset();
    }

    void SelectionProxyModel::clearCurrentIndex()
    {
        m_sourceSelectionModel->clearCurrentIndex();
    }

    void SelectionProxyModel::OnSourceSelectionCurrentChanged(const QModelIndex& current, const QModelIndex& previous)
    {
        AZ_UNUSED(previous);

        QModelIndex targetCurrent = mapFromSource(current);
        QItemSelectionModel::setCurrentIndex(targetCurrent, QItemSelectionModel::Current | QItemSelectionModel::NoUpdate);
    }

    void SelectionProxyModel::OnSourceSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
    {
        QItemSelection targetSelected = mapFromSource(selected);
        QItemSelection targetDeselected = mapFromSource(deselected);

        QItemSelectionModel::select(targetSelected, QItemSelectionModel::Current | QItemSelectionModel::Select | QItemSelectionModel::Rows);
        QItemSelectionModel::select(targetDeselected, QItemSelectionModel::Current | QItemSelectionModel::Deselect | QItemSelectionModel::Rows);
    }

    void SelectionProxyModel::OnProxyModelRowsInserted([[maybe_unused]] const QModelIndex& parent, [[maybe_unused]] int first, [[maybe_unused]] int last)
    {
        QModelIndex sourceIndex = m_sourceSelectionModel->currentIndex();
        QModelIndex targetIndex = mapFromSource(sourceIndex);
        if (targetIndex != currentIndex())
        {
            QItemSelectionModel::setCurrentIndex(targetIndex, QItemSelectionModel::Current | QItemSelectionModel::Select | QItemSelectionModel::Rows);
        }

        QItemSelection sourceSelection = m_sourceSelectionModel->selection();
        QItemSelection targetSelection = mapFromSource(sourceSelection);
        if (targetSelection != selection())
        {
            QItemSelectionModel::select(targetSelection, QItemSelectionModel::Current | QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
        }
    }

    QModelIndex SelectionProxyModel::mapFromSource(const QModelIndex& sourceIndex)
    {
        QModelIndex mappedIndex = sourceIndex;
        for (AZStd::vector<QAbstractProxyModel*>::const_reverse_iterator itProxy = m_proxyModels.rbegin(); itProxy != m_proxyModels.rend(); ++itProxy) {
            mappedIndex = (*itProxy)->mapFromSource(mappedIndex);
        }
        return mappedIndex;
    }

    QItemSelection SelectionProxyModel::mapFromSource(const QItemSelection& sourceSelection)
    {
        QItemSelection mappedSelection = sourceSelection;
        for (AZStd::vector<QAbstractProxyModel*>::const_reverse_iterator itProxy = m_proxyModels.rbegin(); itProxy != m_proxyModels.rend(); ++itProxy) {
            mappedSelection = (*itProxy)->mapSelectionFromSource(mappedSelection);
        }
        return mappedSelection;
    }

    QModelIndex SelectionProxyModel::mapToSource(const QModelIndex& targetIndex)
    {
        QModelIndex mappedIndex = targetIndex;
        for (AZStd::vector<QAbstractProxyModel*>::const_iterator itProxy = m_proxyModels.begin(); itProxy != m_proxyModels.end(); ++itProxy) {
            mappedIndex = (*itProxy)->mapToSource(mappedIndex);
        }
        return mappedIndex;
    }

    QItemSelection SelectionProxyModel::mapToSource(const QItemSelection& targetSelection)
    {
        QItemSelection mappedSelection = targetSelection;
        for (AZStd::vector<QAbstractProxyModel*>::const_iterator itProxy = m_proxyModels.begin(); itProxy != m_proxyModels.end(); ++itProxy) {
            mappedSelection = (*itProxy)->mapSelectionToSource(mappedSelection);
        }
        return mappedSelection;
    }

} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/moc_SelectionProxyModel.cpp>
