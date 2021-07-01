/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef ERRORREPORTTABLEMODEL_H
#define ERRORREPORTTABLEMODEL_H

#if !defined(Q_MOC_RUN)
#include "Util/AbstractSortModel.h"

#include <QMetaType>
#include <QPixmap>
#endif


class CErrorReport;
class CErrorRecord;

bool GetPositionFromString(QString er, float* x, float* y, float* z);

class CErrorReportTableModel
    : public AbstractSortModel
{
    Q_OBJECT
public:
    enum Column
    {
        ColumnSeverity,
        ColumnCount,
        ColumnText,
        ColumnFile,
        ColumnObject,
        ColumnModule,
        ColumnDescription,
        ColumnAssetScope
    };

    enum Roles
    {
        SeverityRole = Qt::UserRole + 1
    };

    CErrorReportTableModel(QObject* parent = 0);
    ~CErrorReportTableModel();

    void setErrorReport(CErrorReport* report);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    bool setHeaderData(int section, Qt::Orientation orientation, const QVariant& value, int role = Qt::EditRole) override;

    bool LessThan(const QModelIndex& lhs, const QModelIndex& rhs) const override;

protected:
    QVariant data(const CErrorRecord& record, int column, int role = Qt::DisplayRole) const;

private:
    QHash<int, int> m_alignments;
    std::vector<CErrorRecord> m_errorRecords;
    QVector<QIcon> m_imageList;
};

Q_DECLARE_METATYPE(const CErrorRecord*)

#endif
