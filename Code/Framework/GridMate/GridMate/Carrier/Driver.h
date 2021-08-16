/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef GM_DRIVER_H
#define GM_DRIVER_H

#include <GridMate/Types.h>

#include <AzCore/std/delegate/delegate.h>
#include <AzCore/std/smart_ptr/intrusive_ptr.h>
#include <AzCore/std/string/conversions.h>

namespace GridMate
{
    struct ThreadConnection;
    class CarrierThread;
    class DriverAddress;

    /**
     * Driver interface is the interface for the lowest level of the transport layer.
     * \note All the code is executed in a thread context! Any interaction with
     * the outside code should be made thread safe.
     */
    class Driver
    {
        friend class DriverAddress;
    public:
        typedef unsigned int ResultCode;

        //list of common error codes
        enum ErrorCodes
        {
            EC_OK = 0,                  ///<

            // Socket errors
            EC_SOCKET_CREATE,
            EC_SOCKET_LISTEN,
            EC_SOCKET_CLOSE,
            EC_SOCKET_MAKE_NONBLOCK,
            EC_SOCKET_BIND,
            EC_SOCKET_SOCK_OPT,
            EC_SOCKET_CONNECT,
            EC_SOCKET_ACCEPT,

            EC_SECURE_CONFIG,   // Invalid configuration.
            EC_SECURE_CREATE,   // Failed to create and configure the SSL context.
            EC_SECURE_CERT,     // Failed to load the provided certificate.
            EC_SECURE_PKEY,     // Failed to load the provided private key.
            EC_SECURE_CA_CERT,  // Failed to load the provided CA cert or cert chain.

            EC_SEND,
            EC_SEND_ADDRESS_NOT_BOUND,  ///< We failed to send because the remote address was NOT bound.
            EC_RECEIVE,

            EC_PLATFORM = 1000, ///< use codes above 1000 for platform specific error codes
            EC_BUFFER_TOOLARGE = 1001
        };

        /**
        * Family types for BSD socket.
        */
        enum BSDSocketFamilyType
        {
            BSD_AF_INET = 0,
            BSD_AF_INET6,
            BSD_AF_UNSPEC,
        };

        Driver() :
            m_canSend(true)
        {}
        virtual ~Driver() {}

        virtual void Update() {}
        virtual void ProcessIncoming() {}
        virtual void ProcessOutgoing() {}

        /// \todo Add QoS support

        /**
        * Platform specific functionality.
        */
        /// Return maximum number of active connections at the same time.
        virtual unsigned int GetMaxNumConnections() const = 0;
        /// Return maximum data size we can send/receive at once in bytes, supported by the platform.
        virtual unsigned int GetMaxSendSize() const = 0;
        /// Return packet overhead size in bytes.
        virtual unsigned int GetPacketOverheadSize() const { return 8 /* standard UDP*/ + 20 /* min for IPv4 */; }
        /*
        UDP/VDP has a 44 byte header when using port 1000 - the header is up to 4 bytes larger when using other ports
        using voice chat over VDP adds an additional 2 bytes to the packet header
        worst case VDP header with voice is 52 bytes - additional overhead incurred for using other ports cannot exceed 4 bytes so this is partially unaccounted for
        */

        /// Transforms an error code to string.
        //virtual void ResultCodeToString(string& str, ResultCode resultCode) const = 0;

        /**
         * User should implement create and bind a UDP socket. This socket will be used for all communications.
         * \param ft family type (this value depends on the platform), 0 will use the default family type (for BSD socket this is ipv4)
         * \param address when 0 it we will assume "any address".
         * \param port When left 0, we use implicit bind (assigned by the system). Otherwise provide a valid port number.
         * \param receiveBufferSize socket receive buffer size in bytes, use 0 for default values.
         * \param sendBufferSize socket send buffer size, use 0 for default values.
         */
        virtual ResultCode  Initialize(int familyType = 0, const char* address = nullptr, unsigned int port = 0, bool isBroadcast = false, unsigned int receiveBufferSize = 0, unsigned int sendBufferSize = 0) = 0;

        /// Returns communication port (must be called after Initialize, otherwise it will return 0)
        virtual unsigned int    GetPort() const = 0;

        /// Send data to a user defined address
        virtual ResultCode      Send(const AZStd::intrusive_ptr<DriverAddress>& to, const char* data, unsigned int dataSize) = 0;
        /**
         * Receives a datagram and stores the source address. maxDataSize must be >= than GetMaxSendSize(). Returns the num of of received bytes.
         * \note If a datagram from a new connection is received, NewConnectionCB will be called. If it rejects the connection the returned from pointer
         * will be NULL while the actual data will be returned.
         */
        virtual unsigned int    Receive(char* data, unsigned int maxDataSize, AZStd::intrusive_ptr<DriverAddress>& from, ResultCode* resultCode = 0) = 0;
        /**
         *  Wait for data to be to the ready for receive. Time out is the maximum time to wait
         * before this function returns. If left to default value it will be in blocking mode (wait until data is ready to be received).
         * \returns true if there is data to be received (always true if timeOut == 0), otherwise false.
         */
        virtual bool            WaitForData(AZStd::chrono::microseconds timeOut = AZStd::chrono::microseconds(0)) = 0;

        /**
         * When you enter wait for data mode, for many reasons you might want to stop wait for data.
         * If you implement this function you need to make sure it's a thread safe function.
         */
        virtual void            StopWaitForData() = 0;

        /// Return true if WaitForData was interrupted before the timeOut expired, otherwise false.
        virtual bool            WasStopeedWaitingForData() = 0;

        /// @{ Address conversion functionality. They MUST implemented thread safe. Generally this is not a problem since they just part local data.
        ///  Create address from ip and port. If ip == NULL we will assign a broadcast address.
        virtual AZStd::string   IPPortToAddress(const char* ip, unsigned int port) const = 0;
        virtual bool            AddressToIPPort(const AZStd::string& address, AZStd::string& ip, unsigned int& port) const = 0;
        /// @}

        /**
         * Creates internal driver address to be used for send/receive calls.
         * \note if the ip and the port are the same, the same pointer will be returned. You can use the returned pointer
         * to compare for unique addresses.
         * \note Driver address allocates internal resources, use it only when you intend to communicate. Otherwise operate with
         * the string address.
         */
        virtual AZStd::intrusive_ptr<DriverAddress> CreateDriverAddress(const AZStd::string& address) = 0;

        /**
         * Returns true if the driver can accept new data (ex, has buffer space).
         */
        virtual bool CanSend() const { return m_canSend; }

    protected:
        virtual void             DestroyDriverAddress(DriverAddress* address) = 0;

        bool                    m_canSend;      ///< Can the driver accept more data
    };

    /**
     * Driver address interface, for low level driver communication.
     */
    class DriverAddress
    {
        friend struct ThreadConnection;
        friend class CarrierThread;
    public:
        DriverAddress(Driver* driver)
            : m_threadConnection(NULL)
            , m_driver(driver)
            , m_refCount(0)
        {
            AZ_Assert(m_driver != NULL, "You must provide a valid driver");
        }

        DriverAddress(const DriverAddress& rhs)
            : m_threadConnection(rhs.m_threadConnection)
            , m_driver(rhs.m_driver)
            , m_refCount(rhs.m_refCount)
        {
        }

        virtual ~DriverAddress() {}

        virtual AZStd::string ToString() const = 0;

        virtual AZStd::string ToAddress() const = 0;

        virtual AZStd::string GetIP() const = 0;

        virtual unsigned int  GetPort() const = 0;

        Driver* GetDriver() const   { return m_driver; }

        bool IsBoundToCarrierConnection() const { return m_threadConnection != nullptr; }

        virtual const void* GetTargetAddress(unsigned int& addressSize) const { addressSize = 0; return nullptr; }

        //////////////////////////////////////////////////////////////////////////
        // for AZStd::intrusive_ptr
        void    add_ref() { ++m_refCount;   }
        void    release()
        {
            AZ_Assert(m_refCount > 0, "Reference count logic error, trying to remove reference when refcount is 0");
            if (--m_refCount == 0)
            {
                m_driver->DestroyDriverAddress(this);
            }
        }
        //////////////////////////////////////////////////////////////////////////
    protected:
        ThreadConnection*       m_threadConnection;     ///< Used by the CarrierThread/ThreadConnection.
        Driver*                 m_driver;
        /// reference counting
        mutable unsigned int    m_refCount;
    };
}

#endif // GM_DRIVER_H
