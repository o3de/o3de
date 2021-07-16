/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef SHADERCOMPILERMANAGER_H
#define SHADERCOMPILERMANAGER_H

#if !defined(Q_MOC_RUN)
#include <QObject>
#include <QHash>
#include <QString>
#include <QByteArray>
#endif

typedef QHash<unsigned int, unsigned int> ShaderCompilerJobMap;

/**
 * The Shader Compiler Manager class receive  a shader compile request
 * and starts a shader compiler job for it
 */
class ShaderCompilerManager
    : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int numberOfJobsStarted READ numberOfJobsStarted NOTIFY numberOfJobsStartedChanged)
    Q_PROPERTY(int numberOfJobsEnded READ numberOfJobsEnded NOTIFY numberOfJobsEndedChanged)
    Q_PROPERTY(int numberOfErrors READ numberOfErrors NOTIFY numberOfErrorsChanged)
public:

    explicit ShaderCompilerManager(QObject* parent = 0);
    virtual ~ShaderCompilerManager();

    void process(unsigned int connID, unsigned int type, unsigned int serial, QByteArray payload);
    void decodeShaderCompilerRequest(unsigned int connID, QByteArray payload);
    void setIsUnitTesting(bool isUnitTesting);
    int numberOfJobsStarted();
    int numberOfJobsEnded();
    int numberOfErrors();
    virtual void sendResponse(unsigned int connId, unsigned int type, unsigned int serial, QByteArray payload);

signals:
    void sendErrorMessage(QString errorMessage);
    void sendErrorMessageFromShaderJob(QString errorMessage, QString server, QString timestamp, QString payload);
    void numberOfJobsStartedChanged();
    void numberOfJobsEndedChanged();
    void numberOfErrorsChanged();


public slots:
    void OnShaderCompilerJobComplete(QByteArray payload, unsigned int requestId);
    void shaderCompilerError(QString errorMessage, QString server, QString timestamp, QString payload);
    void jobStarted();
    void jobEnded();


private:
    ShaderCompilerJobMap m_shaderCompilerJobMap;
    bool m_isUnitTesting;
    int m_numberOfJobsStarted;
    int m_numberOfJobsEnded;
    int m_numberOfErrors;
};

#endif // SHADERCOMPILERMANAGER_H
