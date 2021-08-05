/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <QAbstractTableModel>
#include <AzCore/Math/Uuid.h>

namespace ScriptCanvasEditor
{
    namespace Model
    {
        //! Stores the data for the list of ScriptCanvas libraries
        class LibraryData
            : public QAbstractTableModel
        {
        public:

            enum Role
            {
                DataSetRole = Qt::UserRole
            };

            enum ColumnIndex
            {
                Name,
                Count
            };

            LibraryData(QObject* parent = nullptr);

            int rowCount(const QModelIndex &parent = QModelIndex()) const override;
            int columnCount(const QModelIndex &parent = QModelIndex()) const override;

            QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

            Qt::ItemFlags flags(const QModelIndex &index) const override;

            void Add(const char* name, const AZ::Uuid& uuid);

            struct Data
            {
                QString m_name;
                AZ::Uuid m_uuid;
            };
            typedef QVector<Data> DataSet;

            DataSet m_data;
        };
    }
}
