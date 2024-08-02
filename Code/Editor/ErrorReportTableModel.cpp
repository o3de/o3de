/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorDefs.h"

#include "ErrorReportTableModel.h"
#include <QIcon>

// Editor
#include "ErrorReport.h"

bool GetPositionFromString(QString er, float* x, float* y, float* z)
{
    er = er.toLower();
    int ind = er.indexOf("pos:");
    int shift = 4;
    if (ind < 0)
    {
        ind = er.indexOf("position:");
        shift = 9;
    }
    if (ind >= 0)
    {
        er = er.mid(ind + shift);
        er.remove(QRegExp("^ *"));
        if (er[0] == '(')
        {
            er = er.mid(1);
            er.remove(QRegExp("^ *"));
            ind = er.indexOf(")");
            if (ind > 0)
            {
                er = er.mid(0, ind);
                er.remove(QRegExp(" *$"));

                ind = er.indexOf(" ");
                int ind2 = er.indexOf(",");
                if (ind < 0 || (ind2 > 0 && ind > ind2))
                {
                    ind = ind2;
                }
                if (ind > 0)
                {
                    *x = er.mid(0, ind).toFloat();
                    er = er.mid(ind);
                    er.remove(QRegExp("^[ ,]*"));

                    ind = er.indexOf(" ");
                    ind2 = er.indexOf(",");
                    if (ind < 0 || (ind2 > 0 && ind > ind2))
                    {
                        ind = ind2;
                    }
                    if (ind > 0)
                    {
                        *y = er.mid(0, ind).toFloat();
                        er = er.mid(ind);
                        er.remove(QRegExp("^[ ,]*"));
                        if (er.length())
                        {
                            *z = er.toFloat();
                            return true;
                        }
                    }
                }
            }
        }
    }
    return false;
}

CErrorReportTableModel::CErrorReportTableModel(QObject* parent)
    : AbstractSortModel(parent)
{
    QIcon error_icon = QIcon(":/error_report_error.svg");
    error_icon.addFile(":/error_report_error.svg", QSize(), QIcon::Selected);
    m_imageList.push_back(error_icon);

    // VALIDATOR_ERROR_DBGBRK - this is never used but we need to space the icon list correctly.
    QIcon dbg_icon = QIcon(":/error_report_error.svg");
    dbg_icon.addFile(":/error_report_error.svg", QSize(), QIcon::Selected);
    m_imageList.push_back(dbg_icon);

    QIcon warning_icon = QIcon(":/error_report_warning.svg");
    warning_icon.addFile(":/error_report_warning.svg", QSize(), QIcon::Selected);
    m_imageList.push_back(warning_icon);

    QIcon comment_icon = QIcon(":/error_report_comment.svg");
    comment_icon.addFile(":/error_report_comment.svg", QSize(), QIcon::Selected);
    m_imageList.push_back(comment_icon);
}

CErrorReportTableModel::~CErrorReportTableModel()
{
}

void CErrorReportTableModel::setErrorReport(CErrorReport* report)
{
    beginResetModel();
    if (!m_errorRecords.empty())
    {
        m_errorRecords.clear();
    }
    if (report != nullptr)
    {
        const int count = report->GetErrorCount();
        m_errorRecords.reserve(count);
        for (int i = 0; i < count; ++i)
        {
            m_errorRecords.push_back(report->GetError(i));
        }
    }
    endResetModel();
}

int CErrorReportTableModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_errorRecords.size());
}

int CErrorReportTableModel::columnCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : 8;
}

QVariant CErrorReportTableModel::data(const QModelIndex& index, int role) const
{
    assert(index.row() < m_errorRecords.size());
    const CErrorRecord& record = m_errorRecords[index.row()];
    return data(record, index.column(), role);
}

QVariant CErrorReportTableModel::data(const CErrorRecord& record, int column, int role) const
{
    switch (role)
    {
    case Qt::DisplayRole:
    {
        switch (column)
        {
        case ColumnCount:
            return record.count;
        case ColumnText:
            return QString(record.error).simplified();
        case ColumnFile:
            return record.file;
        case ColumnObject:
            if (!record.error.isEmpty())
            {
                float x, y, z;
                if (GetPositionFromString(record.error, &x, &y, &z))
                {
                    return tr("Pos: (%1, %2, %3)").arg(x, 0, 'f').arg(y, 0, 'f').arg(z, 0, 'f');
                }
            }
            break;
        case ColumnModule:
            switch (record.module)
            {
            case VALIDATOR_MODULE_RENDERER:
                return tr("Renderer");
            case VALIDATOR_MODULE_3DENGINE:
                return tr("3DEngine");
            case VALIDATOR_MODULE_ASSETS:
                return tr("Assets");
            case VALIDATOR_MODULE_SYSTEM:
                return tr("System");
            case VALIDATOR_MODULE_AUDIO:
                return tr("Audio");
            case VALIDATOR_MODULE_MOVIE:
                return tr("Movie");
            case VALIDATOR_MODULE_EDITOR:
                return tr("Editor");
            case VALIDATOR_MODULE_NETWORK:
                return tr("Network");
            case VALIDATOR_MODULE_PHYSICS:
                return tr("Physics");
            case VALIDATOR_MODULE_FEATURETESTS:
                return tr("FeatureTests");
            default:
                return tr("Unknown");
            }
        case ColumnDescription:
            return record.description;
        case ColumnAssetScope:
            return record.assetScope;
        }
        break;
    }
    case Qt::DecorationRole:
    {
        if (column == ColumnSeverity)
        {
            return m_imageList[record.severity];
        }
        break;
    }
    case Qt::UserRole:
    {
        return QVariant::fromValue(&record);
    }
    case Qt::TextAlignmentRole:
    {
        return m_alignments.value(column, Qt::AlignLeft) | Qt::AlignVCenter;
    }
    case Qt::ForegroundRole:
    {
        if (column == ColumnObject)
        {
            return QPalette().color(QPalette::Link);
        }
        break;
    }
    case Qt::FontRole:
    {
        if (column == ColumnObject)
        {
            static QFont f;
            f.setUnderline(true);
            return f;
        }
        break;
    }
    case SeverityRole:
        return record.severity;
    }
    return QVariant();
}

QVariant CErrorReportTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal)
    {
        return QVariant();
    }
    else if (role == Qt::TextAlignmentRole)
    {
        return m_alignments.value(section, Qt::AlignLeft) | Qt::AlignVCenter;
    }
    else if (role == Qt::DisplayRole)
    {
        switch (section)
        {
        case ColumnSeverity:
            return tr("");
        case ColumnCount:
            return tr("N");
        case ColumnText:
            return tr("Text");
        case ColumnFile:
            return tr("File");
        case ColumnObject:
            return tr("Object/Material");
        case ColumnModule:
            return tr("Module");
        case ColumnDescription:
            return tr("Description");
        case ColumnAssetScope:
            return tr("Scope");
        default:
            return QString();
        }
    }
    else
    {
        return QVariant();
    }
}

bool CErrorReportTableModel::setHeaderData(int section, Qt::Orientation orientation, const QVariant& value, int role)
{
    if (orientation == Qt::Horizontal && section >= 0 && section < columnCount() && value.canConvert<int>() && role == Qt::TextAlignmentRole)
    {
        m_alignments.insert(section, value.toInt());
        Q_EMIT headerDataChanged(orientation, section, section);
        const int rows = rowCount();
        if (rows > 0)
        {
            Q_EMIT dataChanged(index(0, section), index(rows - 1, section));
        }
        return true;
    }
    return QAbstractTableModel::setHeaderData(section, orientation, value, role);
}

bool CErrorReportTableModel::LessThan(const QModelIndex& lhs, const QModelIndex& rhs) const
{
    int column = lhs.column();
    if (column == ColumnSeverity)
    {
        return lhs.data(SeverityRole).toInt() < rhs.data(SeverityRole).toInt();
    }
    const QVariant l = lhs.data();
    const QVariant r = rhs.data();
    bool ok;
    const int lInt = l.toInt(&ok);
    const int rInt = r.toInt(&ok);
    if (ok)
    {
        return lInt < rInt;
    }
    return l.toString() < r.toString();
}

#include <moc_ErrorReportTableModel.cpp>
