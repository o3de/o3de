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
#ifndef ASSETBUILDER_CONNECTIONMESSAGE_H
#define ASSETBUILDER_CONNECTIONMESSAGE_H

#include <QByteArray>

namespace AssetProcessor
{
    struct MessageHeader
    {
        unsigned int type;
        unsigned int size;
        unsigned int serial;
    };

    // This is the framing for all packets sent to/from the AssetProcessor
    struct Message
    {
        MessageHeader header;
        QByteArray payload;

        Message() = default;
        Message(const Message&) = default;
    };
}

#endif // ASSETBUILDER_CONNECTIONMESSAGE_H
