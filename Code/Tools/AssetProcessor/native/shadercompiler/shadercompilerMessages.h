/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef SHADERCOMPILERMESSAGES_H
#define SHADERCOMPILERMESSAGES_H

#include <QByteArray>
#include <QString>

struct ShaderCompilerRequestMessage
{
    QByteArray originalPayload;
    QString serverList;
    unsigned short serverPort;
    unsigned int serverListSize;
    unsigned int requestId;
};

#endif //SHADERCOMPILERMESSAGES_H

