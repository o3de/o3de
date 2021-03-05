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
