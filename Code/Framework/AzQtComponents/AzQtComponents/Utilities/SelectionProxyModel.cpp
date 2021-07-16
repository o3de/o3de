/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Utilities/SelectionProxyModel.h>
#include <QAbstractProxyModel>

namespace AzQtComponents
{
    SelectionProxyModel::SelectionProxyModel(QItemSelectionModel* sourceSelectionModel, QAbstractProxyModel* proxyModel, QObject* parent)
        : QItemSelectionModel(proxyModel, parent)
        , m_sourceSelectionModel(sourceSelectionModel)
    {
        connect(sourceSelectionModel, &QItemSelectionModel::selectionChanged, this, &SelectionProxyModel::OnSourceSelectionChanged);
        connect(sourceSelectionModel, &QItemSelectionModel::currentChanged, this, &SelectionProxyModel::OnSourceSelectionCurrentChanged);
        connect(proxyModel, &QAbstractItemModel::rowsInserted, this, &SelectionProxyModel::OnProxyModelRowsInserted);
        connect(this, &QItemSelectionModel::selectionChanged, this, &SelectionProxyModel::OnProxySelectionChanged);

        // Find the chain of proxy models
        QAbstractProxyModel* sourceProxyModel = proxyModel;
        while (sourceProxyModel)
        {
            m_proxyModels.push_back(sourceProxyModel);
            sourceProxyModel = qobject_cast<QAbstractProxyModel*>(sourceProxyModel->sourceModel());
        }

        const QItemSelection currentSelection = mapFromSource(m_sourceSelectionModel->selection());
        QItemSelectionModel::select(currentSelection, QItemSelectionModel::ClearAndSelect);

        const QModelIndex currentModelIndex = mapFromSource(m_sourceSelectionModel->currentIndex());
        QItemSelectionModel::setCurrentIndex(currentModelIndex, QItemSelectionModel::ClearAndSelect);
    }

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

    void SelectionProxyModel::OnSourceSelectionCurrentChanged(const QModelIndex& current, [[maybe_unused]] const QModelIndex& previous)
    {
        QModelIndex targetCurrent = mapFromSource(current);
        QItemSelectionModel::setCurrentIndex(targetCurrent, QItemSelectionModel::NoUpdate);
    }

    void SelectionProxyModel::OnSourceSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
    {
        QItemSelection targetSelected = mapFromSource(selected);
        QItemSelection targetDeselected = mapFromSource(deselected);

        QItemSelectionModel::select(targetSelected, QItemSelectionModel::Select);
        QItemSelectionModel::select(targetDeselected, QItemSelectionModel::Deselect);
    }

    void SelectionProxyModel::OnProxySelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
    {
        const QItemSelection sourceSelected = mapToSource(selected);
        const QItemSelection sourceDeselected = mapToSource(deselected);

        // Disconnect from the selectionChanged signal in the source model to prevent recursion. We could also block the signals
        // of the source selection model, but someone else may be connected to its signals and expect to get an update.
        disconnect(m_sourceSelectionModel, &QItemSelectionModel::selectionChanged, this, &SelectionProxyModel::OnSourceSelectionChanged);
        if (selected.empty() && deselected.empty())
        {
            // Force the signal to fire
            emit m_sourceSelectionModel->selectionChanged({}, {});
        }
        else
        {
            m_sourceSelectionModel->select(sourceSelected, QItemSelectionModel::Select);
            m_sourceSelectionModel->select(sourceDeselected, QItemSelectionModel::Deselect);
        }
        connect(m_sourceSelectionModel, &QItemSelectionModel::selectionChanged, this, &SelectionProxyModel::OnSourceSelectionChanged);
    }

    void SelectionProxyModel::OnProxyModelRowsInserted([[maybe_unused]] const QModelIndex& parent, [[maybe_unused]] int first, [[maybe_unused]] int last)
    {
        QModelIndex sourceIndex = m_sourceSelectionModel->currentIndex();
        QModelIndex targetIndex = mapFromSource(sourceIndex);
        if (targetIndex != currentIndex())
        {
            QItemSelectionModel::setCurrentIndex(targetIndex, QItemSelectionModel::SelectCurrent | QItemSelectionModel::Rows);
        }

        QItemSelection sourceSelection = m_sourceSelectionModel->selection();
        QItemSelection targetSelection = mapFromSource(sourceSelection);
        if (targetSelection != selection())
        {
            QItemSelectionModel::select(targetSelection, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
        }
    }

    QModelIndex SelectionProxyModel::mapFromSource(const QModelIndex& sourceIndex)
    {
        QModelIndex mappedIndex = sourceIndex;
        for (QVector<QAbstractProxyModel*>::const_reverse_iterator itProxy = m_proxyModels.rbegin(); itProxy != m_proxyModels.rend(); ++itProxy)
        {
            mappedIndex = (*itProxy)->mapFromSource(mappedIndex);
        }
        return mappedIndex;
    }

    QItemSelection SelectionProxyModel::mapFromSource(const QItemSelection& sourceSelection)
    {
        QItemSelection mappedSelection = sourceSelection;
        for (QVector<QAbstractProxyModel*>::const_reverse_iterator itProxy = m_proxyModels.rbegin(); itProxy != m_proxyModels.rend(); ++itProxy)
        {
            mappedSelection = (*itProxy)->mapSelectionFromSource(mappedSelection);
        }
        return mappedSelection;
    }

    QModelIndex SelectionProxyModel::mapToSource(const QModelIndex& targetIndex)
    {
        QModelIndex mappedIndex = targetIndex;
        for (QVector<QAbstractProxyModel*>::const_iterator itProxy = m_proxyModels.begin(); itProxy != m_proxyModels.end(); ++itProxy)
        {
            mappedIndex = (*itProxy)->mapToSource(mappedIndex);
        }
        return mappedIndex;
    }

    QItemSelection SelectionProxyModel::mapToSource(const QItemSelection& targetSelection)
    {
        QItemSelection mappedSelection = targetSelection;
        for (QVector<QAbstractProxyModel*>::const_iterator itProxy = m_proxyModels.begin(); itProxy != m_proxyModels.end(); ++itProxy)
        {
            mappedSelection = (*itProxy)->mapSelectionToSource(mappedSelection);
        }
        return mappedSelection;
    }
} // namespace AzQtComponents
