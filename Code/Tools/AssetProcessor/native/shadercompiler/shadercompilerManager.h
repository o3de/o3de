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
