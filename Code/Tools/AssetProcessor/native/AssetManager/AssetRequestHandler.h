/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include "native/assetprocessor.h"
#include "native/utilities/assetUtils.h"

#include <QString>
#include <QByteArray>
#include <QHash>
#include <QObject>
#include <AzCore/Interface/Interface.h>

#include <AzFramework/Asset/AssetSystemTypes.h>
#include <connection/connectionManager.h>
#endif

namespace AzFramework
{
    namespace AssetSystem
    {
        class BaseAssetProcessorMessage;
    } // namespace AssetSystem
} // namespace AzFramework

namespace AssetProcessor
{
    class AssetRequestHandler;

    template<typename TRequest>
    struct MessageData
    {
        static_assert(AZStd::is_base_of<AzFramework::AssetSystem::BaseAssetProcessorMessage, TRequest>::value, "TRequest must derive from BaseAssetProcessorMessage");

        AZStd::shared_ptr<TRequest> m_message;
        NetworkRequestID m_key;
        QString m_platform;
        bool m_fencingFailed{ false };

        MessageData() = default;
        MessageData(AZStd::shared_ptr<TRequest> message, NetworkRequestID key, QString platform, bool fencingFailed = false)
            : m_message(message), m_key(key), m_platform(platform), m_fencingFailed(fencingFailed)
        {}

        template<typename TOther>
        MessageData(const MessageData<TOther>& rhs)
        {
            m_message = AZStd::rtti_pointer_cast<TRequest>(rhs.m_message);
            m_key = rhs.m_key;
            m_platform = rhs.m_platform;
            m_fencingFailed = rhs.m_fencingFailed;
        }
    };

    struct IRequestRouter
    {
        friend class AssetRequestHandler;

        AZ_RTTI(IRequestRouter, "{FC7F875C-2CD1-4CD2-AC63-71097DF612AC}");

        IRequestRouter(AZStd::function<void(unsigned int, unsigned int, QByteArray, QString)> requestHandler)
            : m_requestHandler(AZStd::move(requestHandler))
        {
            AZ::Interface<IRequestRouter>::Register(this);
        }

        virtual ~IRequestRouter()
        {
            AZ::Interface<IRequestRouter>::Unregister(this);
        }

        //! Registers a QT object callback as a handler for a TRequest type of message.
        //! The callback function will be run on obj's thread
        //! If the return value of the handler is void, no response will be sent.
        //! Not thread-safe, do not call after AP initialization stage
        template<typename TRequest, typename TResponse, typename TClass>
        void RegisterQueuedCallbackHandler(TClass* obj, TResponse(TClass::* handler)(AssetProcessor::MessageData<TRequest>))
        {
            AZ_Assert(obj, "Programmer Error - Handler object is null");

            // Return type is set to void here since the response needs to be delayed along with the handler call
            // HandleResponse gets called twice in this whole chain but the first time won't attempt to send a response because of this void
            RegisterMessageHandler<TRequest, void>([=](MessageData<TRequest> messageData)
                {
                    QMetaObject::invokeMethod(obj, [=]()
                        {
                            // This will run on the obj's thread and handle sending the response now that we're ready to process
                            HandleResponse<TRequest, TResponse>([obj, handler](MessageData<TRequest> messageData) -> TResponse
                                {
                                    return (obj->*handler)(messageData);
                                }, messageData);
                        }, Qt::ConnectionType::QueuedConnection);
                });
        }

        //! Registers a callback as a handler for a TRequest type of message.
        //! If the return value of the handler is void, no response will be sent.
        //! Not thread-safe, do not call after AP initialization stage
        template <class TRequest, class TResponse>
        void RegisterMessageHandler(TResponse(*handler)(MessageData<TRequest> messageData))
        {
            RegisterMessageHandler<TRequest, TResponse>(AZStd::function<TResponse(MessageData<TRequest>)>(AZStd::move(handler)));
        }

        //! Registers a callback as a handler for a TRequest type of message.
        //! If the return value of the handler is void, no response will be sent.
        //! Not thread-safe, do not call after AP initialization stage
        template <class TRequest, class TResponse>
        void RegisterMessageHandler(AZStd::function<TResponse(MessageData<TRequest>)> handler)
        {
            static constexpr unsigned int MessageType = TRequest::MessageType;

            m_messageHandlers[MessageType] = [handler = AZStd::move(handler)](MessageData<AzFramework::AssetSystem::BaseAssetProcessorMessage> messageData)
            {
                MessageData<TRequest> downcastData = messageData;

                if (downcastData.m_message)
                {
                    IRequestRouter::HandleResponse<TRequest, TResponse>(AZStd::move(handler), downcastData);
                }
                else
                {
                    AZ_TracePrintf(AssetProcessor::DebugChannel, "Expected message type (%d) but incoming message type is %d.\n", MessageType, messageData.m_message->GetMessageType());
                }
            };

            using namespace AZStd::placeholders;

            ConnectionManagerRequestBus::Broadcast(&ConnectionManagerRequestBus::Events::RegisterService, MessageType, AZStd::bind(m_requestHandler, _1, _3, _4, _5));
        }

        template<class TRequest>
        void UnregisterMessageHandler()
        {
            static constexpr unsigned int MessageType = TRequest::MessageType;

            auto messageItr = m_messageHandlers.find(MessageType);

            if(messageItr != m_messageHandlers.end())
            {
                m_messageHandlers.erase(messageItr);
            }
        }

        AZ_DISABLE_COPY_MOVE(IRequestRouter);

    protected:

        //! Helper to handle sending a response for a message if one is needed.
        template<class TRequest, class TResponse, typename AZStd::enable_if_t<!AZStd::is_void_v<TResponse>>* = nullptr>
        static void HandleResponse(AZStd::function<TResponse(MessageData<TRequest>)> handler, MessageData<TRequest> messageData)
        {
            auto&& response = handler(messageData);

            ConnectionBus::Event(messageData.m_key.first, &ConnectionBus::Events::SendResponse, messageData.m_key.second, response);
        }

        template<class TRequest, class TResponse, typename AZStd::enable_if_t<AZStd::is_void_v<TResponse>>* = nullptr>
        static void HandleResponse(AZStd::function<TResponse(MessageData<TRequest>)> handler, MessageData<TRequest> messageData)
        {
            // This template handles void returns which mean no response should be sent
            handler(messageData);
        }

        using MessageHandler = AZStd::function<void(MessageData<AzFramework::AssetSystem::BaseAssetProcessorMessage>)>;

        //! Map of messageType to message handler callback
        AZStd::unordered_map<unsigned int /*messageType*/, MessageHandler> m_messageHandlers;

        //! Parent object callback which will be registered with the ConnectionManager for each message
        AZStd::function<void(unsigned int, unsigned int, QByteArray, QString)> m_requestHandler;
    };

    //! AssetRequestHandler
    //!  this exists to handle requests from outside sources to compile assets.
    //!  or to get the status of groups of assets.
    class AssetRequestHandler
        : public QObject
    {
        using AssetStatus = AzFramework::AssetSystem::AssetStatus;
        using BaseAssetProcessorMessage = AzFramework::AssetSystem::BaseAssetProcessorMessage;
        Q_OBJECT
    public:

        AssetRequestHandler();
    protected:

        //! This function creates a fence file.
        //! It will return the fencefile path if it succeeds, otherwise it returns an empty string
        virtual QString CreateFenceFile(unsigned int fenceId);
        //! This function delete a fence file.
        //! it will return true if it succeeds, otherwise it returns false.
        virtual bool DeleteFenceFile(QString fenceFileName);

Q_SIGNALS:
        //! Request that a compile group is created for all assets that match that platform and search term.
        //! emitting this signal will ultimately result in OnCompileGroupCreated and OnCompileGroupFinished being executed
        //! at some later time with the same groupID.
        void RequestCompileGroup(NetworkRequestID groupID, QString platform, QString searchTerm, AZ::Data::AssetId assetId, bool isStatusRequest, int searchType);

        //! This request goes out to ask the system in general whether an asset can be found (as a product).
        void RequestAssetExists(NetworkRequestID groupID, QString platform, QString searchTerm, AZ::Data::AssetId assetId, int searchType);

        void RequestEscalateAssetByUuid(QString platform, AZ::Uuid escalatedAssetUUID);
        void RequestEscalateAssetBySearchTerm(QString platform, QString escalatedSearchTerm);

    public Q_SLOTS:
        //! ProcessGetAssetStatus - someone on the network wants to know about the status of an asset.
        //! isStatusRequest will be TRUE if its a status request.  If its false it means its a compile request
        void ProcessAssetRequest(MessageData<AzFramework::AssetSystem::RequestAssetStatus> messageData);

        //! OnCompileGroupCreated is invoked in response to asking for a compile group to be created.
        //! Its status will either be Unknown if no assets are queued or in flight that match that pattern
        //! or it will be Queued or Compiling if some were matched.
        //! If you get a Queued or Compiling, you will eventually get a OnCompileGroupFinished with the same group ID.
        void OnCompileGroupCreated(NetworkRequestID groupID, AssetStatus status);

        //! OnCompileGroupFinished is expected to be called when a compile group completes or fails.
        //! the status is expected to be either Compiled or Failed.
        void OnCompileGroupFinished(NetworkRequestID groupID, AssetStatus status);

        //! Called from the outside in response to a RequestAssetExists.
        void OnRequestAssetExistsResponse(NetworkRequestID groupID, bool exists);

        void OnFenceFileDetected(unsigned int fenceId);

        //!  This will get called for every asset related messages or messages that require fencing
        virtual void OnNewIncomingRequest(unsigned int connId, unsigned int serial, QByteArray payload, QString platform);

    public:

        //! Just return how many in flight requests there are.
        int GetNumOutstandingAssetRequests() const;

    protected:
        template<typename TRequest, typename TResponse>
        AZStd::function<TResponse(MessageData<TRequest>)> ToFunction(TResponse(AssetRequestHandler::* func)(MessageData<TRequest>))
        {
            using namespace AZStd::placeholders;

            return AZStd::function<TResponse(MessageData<TRequest>)>(AZStd::bind(func, this, _1));
        }

        // Invokes the appropriate handler and returns true if the message should be deleted by the caller and false if the request handler is responsible for deleting the message
        virtual bool InvokeHandler(MessageData<AzFramework::AssetSystem::BaseAssetProcessorMessage> message);

        //! This is an internal struct that is used for storing all the necessary information for requests that require fencing
        struct RequestInfo
        {
            RequestInfo() = default;

            RequestInfo(NetworkRequestID requestId, AZStd::shared_ptr<BaseAssetProcessorMessage> message, QString platform)
                : m_requestId(requestId)
                , m_message(AZStd::move(message))
                , m_platform(platform)
            {
            }

            NetworkRequestID m_requestId{};
            AZStd::shared_ptr<BaseAssetProcessorMessage> m_message{};
            QString m_platform{};
        };
        AZStd::unordered_map<unsigned int, RequestInfo> m_pendingFenceRequestMap;

    protected:

        void DeleteFenceFile_Retry(unsigned fenceId, QString fenceFileName, NetworkRequestID key, AZStd::shared_ptr<BaseAssetProcessorMessage> message, QString platform, int retriesRemaining);

        void SendAssetStatus(NetworkRequestID groupID, unsigned int type, AssetStatus status);

        void HandleRequestEscalateAsset(MessageData<AzFramework::AssetSystem::RequestEscalateAsset> messageData);

        // we keep state about a request in this class:
        class AssetRequestLine
        {
        public:
            AssetRequestLine(QString platform, QString searchTerm, const AZ::Data::AssetId& assetId, bool isStatusRequest, int searchType);
            bool IsStatusRequest() const;
            QString GetPlatform() const;
            QString GetSearchTerm() const;
            const AZ::Data::AssetId& GetAssetId() const;
            QString GetDisplayString() const;
            int GetSearchType() const;
        private:
            bool m_isStatusRequest;
            QString m_platform;
            QString m_searchTerm;
            AZ::Data::AssetId m_assetId;
            int m_searchType{ 0 };
        };

        // this map keeps track of whether a request was for a compile (FALSE), or a status (TRUE)
        QHash<NetworkRequestID, AssetRequestLine> m_pendingAssetRequests;

        unsigned int m_fenceId = 0;

        IRequestRouter m_requestRouter{ [this](unsigned int connId, unsigned int serial, QByteArray payload, QString platform) {OnNewIncomingRequest(connId, serial, payload, platform); } };
    };
} // namespace AssetProcessor
