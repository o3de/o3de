/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
