/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef SHADERCOMPILERMODEL_H
#define SHADERCOMPILERMODEL_H

#if !defined(Q_MOC_RUN)
#include <QAbstractItemModel>
#include <QList>
#include <QVariant>
#include <QHash>
#include <QByteArray>
#include <QString>
#endif

class QModelIndex;
class QObject;

struct ShaderCompilerErrorInfo
{
    QString m_shaderError;
    QString m_shaderTimestamp;
    QString m_shaderOriginalPayload;
    QString m_shaderServerName;

    ShaderCompilerErrorInfo(QString shaderError, QString shaderTimestamp, QString shaderOriginalPayload, QString shaderServerName)
        : m_shaderError(shaderError)
        , m_shaderTimestamp(shaderTimestamp)
        , m_shaderOriginalPayload(shaderOriginalPayload)
        , m_shaderServerName(shaderServerName)
    {
    }
};

/** The Shader Compiler model is responsible for capturing error requests
 */
class ShaderCompilerModel
    : public QAbstractItemModel
{
    Q_OBJECT
public:

    enum DataRoles
    {
        TimeStampRole = Qt::UserRole + 1,
        ServerRole,
        ErrorRole,
        OriginalRequestRole,
    };

    enum Column
    {
        ColumnTimeStamp,
        ColumnServer,
        ColumnError,
        Max
    };

    /// standard Qt constructor
    explicit ShaderCompilerModel(QObject* parent = 0);
    virtual ~ShaderCompilerModel();

    // singleton pattern
    static ShaderCompilerModel* Get();


    /// QAbstractListModel interface
    QModelIndex parent(const QModelIndex&) const override;
    QModelIndex index(int row, int column, const QModelIndex& parent) const override;
    int columnCount(const QModelIndex&) const override;
    virtual int rowCount(const QModelIndex& parent) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    virtual QVariant data(const QModelIndex& index, int role) const override;
    virtual QHash<int, QByteArray> roleNames() const override;
    virtual Qt::ItemFlags flags(const QModelIndex& index) const override;

public slots:
    void addShaderErrorInfoEntry(QString errorMessage, QString timestamp, QString payload, QString server);

private:

    QList<ShaderCompilerErrorInfo> m_shaderErrorInfoList;
};


#endif // SHADERCOMPILERMODEL_H



