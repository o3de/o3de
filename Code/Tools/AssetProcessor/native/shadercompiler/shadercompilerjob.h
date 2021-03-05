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
#ifndef SHADERCOMPILERJOB_H
#define SHADERCOMPILERJOB_H

#include <QRunnable>
#include "shadercompilerMessages.h"

class QByteArray;
class QObject;

/**
 * This class is responsible for connecting to the shader compiler server
 * and getting back the response to the shader compiler manager
 */
class ShaderCompilerJob
    : public QRunnable
{
public:

    explicit ShaderCompilerJob();
    virtual ~ShaderCompilerJob();
    ShaderCompilerRequestMessage ShaderCompilerMessage() const;
    void initialize(QObject* pManager, const ShaderCompilerRequestMessage& ShaderCompilerMessage);
    QString getServerAddress();
    bool isServerListEmpty();
    virtual void run() override;

    void setIsUnitTesting(bool isUnitTesting);

    bool attemptDelivery(QString serverAddress, QByteArray& payload);

private:
    ShaderCompilerRequestMessage m_ShaderCompilerMessage;
    QObject*  m_manager;
    bool m_isUnitTesting;
};

#endif // SHADERCOMPILERJOB_H
