/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <native/utilities/AssetUtilEBusHelper.h>
#include <native/utilities/ByteArrayStream.h>
#include <AzFramework/Asset/AssetProcessorMessages.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/IO/GenericStreams.h>
#include <QByteArray>
#include <QString>
#include <AzCore/std/functional.h>

namespace AssetProcessor
{
    //! This class is mocks connection bus functionality in unit test mode
    class MockConnectionHandler
        : public AssetProcessor::ConnectionBus::Handler
    {
    public:
        ~MockConnectionHandler()
        {
            BusDisconnect();
        }

        size_t Send(unsigned int serial, const AzFramework::AssetSystem::BaseAssetProcessorMessage& message) override
        {
            QByteArray buffer;
            ByteArrayStream stream;
            bool wroteToStream = AZ::Utils::SaveObjectToStream(stream, AZ::DataStream::ST_BINARY, &message, message.RTTI_GetType());
            AZ_Assert(wroteToStream, "Connection::Send: Could not serialize to stream (type=%u)", message.GetMessageType());
            if (wroteToStream)
            {
                SendRaw(message.GetMessageType(), serial, stream.GetArray());
                return buffer.size();
            }
            return 0;
        }
        size_t SendRaw(unsigned int type, unsigned int serial, const QByteArray& payload) override
        {
            m_sent = true;
            if (m_callback)
            {
                //if callback is not null than calls it
                m_callback(type, serial, payload);
            }
            else
            {
                m_type = type;
                m_serial = serial;
                m_payload = payload;
            }

            return payload.size();
        }

        size_t SendPerPlatform(unsigned int serial, const AzFramework::AssetSystem::BaseAssetProcessorMessage& message, const QString& platform) override
        {
            if (QString::compare(platform, "pc", Qt::CaseInsensitive) == 0 || QString::compare(platform, "android", Qt::CaseInsensitive) == 0)
            {
                return Send(serial, message);
            }
            return 0;
        }

        size_t SendRawPerPlatform(unsigned int type, unsigned int serial, const QByteArray& data, const QString& platform) override
        {
            if (QString::compare(platform, "pc", Qt::CaseInsensitive) == 0 || QString::compare(platform, "android", Qt::CaseInsensitive) == 0)
            {
                return SendRaw(type, serial, data);
            }

            return 0;
        }

        unsigned int SendRequest(const AzFramework::AssetSystem::BaseAssetProcessorMessage& message, const AssetProcessor::ConnectionBusTraits::ResponseCallback& callback) override
        {
            Send(0, message);

            callback(message.GetMessageType(), QByteArray());

            return 0;
        }

        size_t SendResponse(unsigned int serial, const AzFramework::AssetSystem::BaseAssetProcessorMessage& message) override
        {
            return Send(serial, message);
        }

        void RemoveResponseHandler(unsigned int /*serial*/) override
        {
            // Nothing to do
        }

        typedef AZStd::function< void (unsigned int, unsigned int, const QByteArray&) > SendMessageCallBack;
        bool m_sent = false;
        unsigned int m_type = 0;
        unsigned int m_serial = 0;
        QByteArray m_payload;
        SendMessageCallBack m_callback;
    };
}
