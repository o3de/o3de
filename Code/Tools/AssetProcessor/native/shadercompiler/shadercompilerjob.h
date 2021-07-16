/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
