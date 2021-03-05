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
#include <QtCore/QItemSelectionModel>
#include <AzCore/std/containers/vector.h>
#endif

QT_FORWARD_DECLARE_CLASS(QAbstractProxyModel)

namespace EMStudio
{
    // This class is a QItemSelectionModel that syncs through model proxies and maintains
    // selection. In Qt we can have a model being filtered/sorted by proxy models. If the
    // selection model is connected to the original model, the view needs a new selection
    // model that understands the filtering. This class does that conversion.
    // NOTE: this class does not support changing proxy models (anywhere in the chain). 
    // The class will have to be recreated with the new proxy model.
    //
    class SelectionProxyModel
        : public QItemSelectionModel
    {
        Q_OBJECT

    public:
        SelectionProxyModel(QItemSelectionModel* sourceSelectionModel, QAbstractProxyModel* proxyModel, QObject* parent = nullptr);
        ~SelectionProxyModel() override;

        void setCurrentIndex(const QModelIndex &index, QItemSelectionModel::SelectionFlags command) override;
        void select(const QModelIndex &index, QItemSelectionModel::SelectionFlags command) override;
        void select(const QItemSelection &selection, QItemSelectionModel::SelectionFlags command) override;
        void clear() override;
        void reset() override;
        void clearCurrentIndex() override;

    private slots:
        void OnSourceSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
        void OnSourceSelectionCurrentChanged(const QModelIndex& current, const QModelIndex& previous);
        void OnProxyModelRowsInserted(const QModelIndex& parent, int first, int last);
        
        /*void currentRowChanged(const QModelIndex &current, const QModelIndex &previous);
        void currentColumnChanged(const QModelIndex &current, const QModelIndex &previous);
        void modelChanged(QAbstractItemModel *model);*/

    private:
        QModelIndex mapFromSource(const QModelIndex& sourceIndex);
        QItemSelection mapFromSource(const QItemSelection& sourceSelection);

        QModelIndex mapToSource(const QModelIndex& targetIndex);
        QItemSelection mapToSource(const QItemSelection& targetSelection);

        // Contains the chain of proxy models that leads us to the real model. The proxy models
        // are sorted from left: outter-most proxy to right: inner-most proxy
        AZStd::vector<QAbstractProxyModel*> m_proxyModels;
        QItemSelectionModel* m_sourceSelectionModel;
    };

}   // namespace EMStudio
