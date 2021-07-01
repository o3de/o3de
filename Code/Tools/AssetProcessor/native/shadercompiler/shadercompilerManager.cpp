/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "shadercompilerManager.h"
#include "shadercompilerjob.h"

#include <QThreadPool>

#include "native/utilities/assetUtils.h"

ShaderCompilerManager::ShaderCompilerManager(QObject* parent)
    : QObject(parent)
    , m_isUnitTesting(false)
    , m_numberOfJobsStarted(0)
    , m_numberOfJobsEnded(0)
    , m_numberOfErrors(0)
{
}

ShaderCompilerManager::~ShaderCompilerManager()
{
}

void ShaderCompilerManager::process(unsigned int connID, unsigned int type, unsigned int serial, QByteArray payload)
{
    (void)type;
    (void)serial;
    Q_ASSERT(AssetUtilities::ComputeCRC32Lowercase("ShaderCompilerProxyRequest") == type);
    decodeShaderCompilerRequest(connID, payload);
}

void ShaderCompilerManager::decodeShaderCompilerRequest(unsigned int connID, QByteArray payload)
{
    if (payload.length() < sizeof(unsigned int) + sizeof(unsigned int) + 2 + sizeof(unsigned short))
    {
        QString error = "Payload size is too small";
        AZ_Warning(AssetProcessor::ConsoleChannel, false, error.toUtf8().data());
        emit sendErrorMessage(error);
        return;
    }

    unsigned char* data_end = reinterpret_cast<unsigned char*>(payload.data() + payload.size());
    unsigned int* requestId = reinterpret_cast<unsigned int*>(data_end - sizeof(unsigned int));
    unsigned int* serverListSizePtr = reinterpret_cast<unsigned int*>(data_end - sizeof(unsigned int) - sizeof(unsigned int));
    unsigned short* serverPortPtr = reinterpret_cast<unsigned short*>(data_end - sizeof(unsigned int) - sizeof(unsigned int) - sizeof(unsigned short));

    ShaderCompilerRequestMessage msg;
    QString error;

    msg.requestId = *requestId;
    msg.serverListSize = *serverListSizePtr;
    msg.serverPort =  *serverPortPtr;
    if ((msg.serverListSize <= 0) || (msg.serverListSize > 100000))
    {
        error = "Shader Compiler Server List is wrong";
        AZ_Warning(AssetProcessor::ConsoleChannel, false, error.toUtf8().data());
        emit sendErrorMessage(error);
        return;
    }
    if (msg.serverPort == 0)
    {
        error = "Shader Compiler port is wrong";
        AZ_Warning(AssetProcessor::ConsoleChannel, false, error.toUtf8().data());
        emit sendErrorMessage(error);
        return;
    }

    char* position_of_first_null = reinterpret_cast<char*>(serverPortPtr) - 1;// -1 for null
    if ((*position_of_first_null) != '\0')
    {
        error = "Shader Compiler payload is corrupt,position is not null";
        AZ_Warning(AssetProcessor::ConsoleChannel, false, error.toUtf8().data());
        emit sendErrorMessage(error);
        return;
    }
    char* beginning_of_serverList = position_of_first_null - msg.serverListSize;
    char* position_of_second_null = beginning_of_serverList - 1;//-1 for null

    if ((*position_of_second_null) != '\0')
    {
        error = "Shader Compiler payload is corrupt,position is not null";
        AZ_Warning(AssetProcessor::ConsoleChannel, false, error.toUtf8().data());
        emit sendErrorMessage(error);
        return;
    }

    unsigned int originalPayloadSize = static_cast<unsigned int>(payload.size()) - sizeof(unsigned int) - sizeof(unsigned int) - sizeof(unsigned short) - static_cast<unsigned int>(msg.serverListSize) - 2;
    msg.serverList = beginning_of_serverList;
    msg.originalPayload.insert(0, payload.data(), static_cast<unsigned int>(originalPayloadSize));
    ShaderCompilerJob* shaderCompilerJob = new ShaderCompilerJob();
    shaderCompilerJob->initialize(this, msg);
    shaderCompilerJob->setIsUnitTesting(m_isUnitTesting);
    m_shaderCompilerJobMap[msg.requestId] = connID;
    shaderCompilerJob->setAutoDelete(true);
    QThreadPool* threadPool = QThreadPool::globalInstance();
    threadPool->start(shaderCompilerJob);
}

void ShaderCompilerManager::OnShaderCompilerJobComplete(QByteArray payload, unsigned int requestId)
{
    auto iterator = m_shaderCompilerJobMap.find(requestId);
    if (iterator != m_shaderCompilerJobMap.end())
    {
        sendResponse(iterator.value(), AssetUtilities::ComputeCRC32Lowercase("ShaderCompilerProxyResponse"), 0, payload);
    }
    else
    {
        QString error = "Shader Compiler cannot find the connection id";
        AZ_Warning(AssetProcessor::ConsoleChannel, false, error.toUtf8().data());
        emit sendErrorMessage(error);
    }
}

void ShaderCompilerManager::sendResponse(unsigned int connId, unsigned int /*type*/, unsigned int /*serial*/, QByteArray payload)
{
    EBUS_EVENT_ID(connId, AssetProcessor::ConnectionBus, SendRaw, AssetUtilities::ComputeCRC32Lowercase("ShaderCompilerProxyResponse"), 0, payload);
}

void ShaderCompilerManager::shaderCompilerError(QString errorMessage, QString server, QString timestamp, QString payload)
{
    m_numberOfErrors++;
    emit numberOfErrorsChanged();
    emit sendErrorMessageFromShaderJob(errorMessage, server, timestamp, payload);
}

void ShaderCompilerManager::jobStarted()
{
    m_numberOfJobsStarted++;
    emit numberOfJobsStartedChanged();
}

void ShaderCompilerManager::jobEnded()
{
    m_numberOfJobsEnded++;
    numberOfJobsEndedChanged();
}


void ShaderCompilerManager::setIsUnitTesting(bool isUnitTesting)
{
    m_isUnitTesting = isUnitTesting;
}

int ShaderCompilerManager::numberOfJobsStarted()
{
    return m_numberOfJobsStarted;
}

int ShaderCompilerManager::numberOfJobsEnded()
{
    return m_numberOfJobsEnded;
}

int ShaderCompilerManager::numberOfErrors()
{
    return m_numberOfErrors;
}

