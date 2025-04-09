/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef CONNECTIONMANAGER_H
#define CONNECTIONMANAGER_H

#if !defined(Q_MOC_RUN)
#include <AzCore/std/function/function_fwd.h> // <functional> complains about exception handling and is not okay to mix with azcore/etc stuff.
#include <QReadWriteLock>
#include <QMap>
#include <QMultiMap>
#include <QObject>
#include <QString>
#include <QHostAddress>
#include <QStringListModel>
#include "native/utilities/AssetUtilEBusHelper.h"
#include <QAbstractItemModel>
#endif

class Connection;
//! defines the callback type for registering handlers to handle messages coming from outside into Asset Processor.
//! params are 
//! connectionId, who its coming from
//! messageType, type of message, for use when you bind the same handler interface to many different message types
//! int serial number of message, to check for duplicates and also if you want to respond, you must respond with the same number
//! QByteArray payload, the payload of the message
//! QString platform - the platform of the sender ("pc" for example)
typedef AZStd::function<void(unsigned int, unsigned int, unsigned int, QByteArray, QString)> regFunc;
typedef QMap<unsigned int, Connection*> ConnectionMap;
typedef QMultiMap<unsigned int, regFunc> RouteMultiMap;

class ConnectionManagerRequests
    : public AZ::EBusTraits
{
public:
    virtual void RegisterService(unsigned int messageType, regFunc func) = 0;
};

using ConnectionManagerRequestBus = AZ::EBus<ConnectionManagerRequests>;

namespace AssetProcessor
{
    class PlatformConfiguration;
}

/** This is a container class for connection
 */
class ConnectionManager
    : public QAbstractItemModel,
    public ConnectionManagerRequestBus::Handler
{
    Q_OBJECT

public:

    enum Column
    {
        StatusColumn,
        IdColumn,
        IpColumn,
        PortColumn,
        PlatformColumn,
        AutoConnectColumn,
        Max
    };

    enum Roles
    {
        UserConnectionRole = Qt::UserRole + 1,
    };

    explicit ConnectionManager(QObject* parent = 0);
    virtual ~ConnectionManager();
    // Singleton pattern:
    static ConnectionManager* Get();
    Q_INVOKABLE int getCount() const;
    Q_INVOKABLE Connection* getConnection(unsigned int connectionId);
    Q_INVOKABLE ConnectionMap& getConnectionMap();
    Q_INVOKABLE unsigned int addConnection(qintptr socketDescriptor = -1);
    Q_INVOKABLE unsigned int addUserConnection();
    Q_INVOKABLE void removeConnection(unsigned int connectionId);
    unsigned int GetConnectionId(QString ipaddress, int port);
    void SaveConnections(QString settingPrefix = ""); // settingPrefix allowed for testing purposes.
    void LoadConnections(QString settingPrefix = ""); // settingPrefix allowed for testing purposes.

    void RegisterService(unsigned int type, regFunc func) override;

    //QAbstractItemListModel
    QVariant data(const QModelIndex& index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    QModelIndex index(int row, int column, const QModelIndex&) const override;
    QModelIndex parent(const QModelIndex&) const override;
    int columnCount(const QModelIndex& parent) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role) override;

    void removeConnection(const QModelIndex& index);

Q_SIGNALS:
    void connectionAdded(unsigned int connectionId, Connection* connection);
    void beforeConnectionRemoved(unsigned int connectionId);

    void ConnectionDisconnected(unsigned int connectionId);
    void ConnectionRemoved(unsigned int connectionId);

    void ConnectionError(unsigned int connId, QString error);

    void ConnectionReady(unsigned int connectionId, QStringList platforms);

    void ReadyToQuit(QObject* source);

    void SyncAllowedListAndRejectedList(QStringList allowedList, QStringList rejectedList);

    // this is a response to the allowedlist request with that same token.
    void AddressIsInAllowedList(void* token, bool result);

    void FirstTimeAddedToRejctedList(QString ipAddress);

public Q_SLOTS:
    void SendMessageToService(unsigned int connId, unsigned int type, unsigned int serial, QByteArray payload);
    void QuitRequested();
    void RemoveConnectionFromMap(unsigned int connectionId);
    void MakeSureConnectionMapEmpty();
    void NewConnection(qintptr socketDescriptor);

    void AllowedListingEnabled(bool enabled);
    void IsAddressInAllowedList(QHostAddress hostAddress, void* token);
    void AddAddressToAllowedList(QString address);
    void RemoveAddressFromAllowedList(QString address);
    void AddRejectedAddress(QString address, bool surpressWarning = false);
    void RemoveRejectedAddress(QString address);

    //metrics
    void AddBytesReceived(unsigned int connId, qint64 add, bool update);
    void AddBytesSent(unsigned int connId, qint64 add, bool update);
    void AddBytesRead(unsigned int connId, qint64 add, bool update);
    void AddBytesWritten(unsigned int connId, qint64 add, bool update);
    void AddOpenRequest(unsigned int connId, bool update);
    void AddCloseRequest(unsigned int connId, bool update);
    void AddOpened(unsigned int connId, bool update);
    void AddClosed(unsigned int connId, bool update);
    void AddReadRequest(unsigned int connId, bool update);
    void AddWriteRequest(unsigned int connId, bool update);
    void AddTellRequest(unsigned int connId, bool update);
    void AddSeekRequest(unsigned int connId, bool update);
    void AddIsReadOnlyRequest(unsigned int connId, bool update);
    void AddIsDirectoryRequest(unsigned int connId, bool update);
    void AddSizeRequest(unsigned int connId, bool update);
    void AddModificationTimeRequest(unsigned int connId, bool update);
    void AddExistsRequest(unsigned int connId, bool update);
    void AddFlushRequest(unsigned int connId, bool update);
    void AddCreatePathRequest(unsigned int connId, bool update);
    void AddDestroyPathRequest(unsigned int connId, bool update);
    void AddRemoveRequest(unsigned int connId, bool update);
    void AddCopyRequest(unsigned int connId, bool update);
    void AddRenameRequest(unsigned int connId, bool update);
    void AddFindFileNamesRequest(unsigned int connId, bool update);

    void UpdateBytesReceived(unsigned int connId);
    void UpdateBytesSent(unsigned int connId);
    void UpdateBytesRead(unsigned int connId);
    void UpdateBytesWritten(unsigned int connId);
    void UpdateOpenRequest(unsigned int connId);
    void UpdateCloseRequest(unsigned int connId);
    void UpdateOpened(unsigned int connId);
    void UpdateClosed(unsigned int connId);
    void UpdateReadRequest(unsigned int connId);
    void UpdateWriteRequest(unsigned int connId);
    void UpdateTellRequest(unsigned int connId);
    void UpdateSeekRequest(unsigned int connId);
    void UpdateIsReadOnlyRequest(unsigned int connId);
    void UpdateIsDirectoryRequest(unsigned int connId);
    void UpdateSizeRequest(unsigned int connId);
    void UpdateModificationTimeRequest(unsigned int connId);
    void UpdateExistsRequest(unsigned int connId);
    void UpdateFlushRequest(unsigned int connId);
    void UpdateCreatePathRequest(unsigned int connId);
    void UpdateDestroyPathRequest(unsigned int connId);
    void UpdateRemoveRequest(unsigned int connId);
    void UpdateCopyRequest(unsigned int connId);
    void UpdateRenameRequest(unsigned int connId);
    void UpdateFindFileNamesRequest(unsigned int connId);

    void UpdateConnectionMetrics();

    void OnStatusChanged(unsigned int connId);

    void UpdateAllowedListFromBootStrap();

private:

    unsigned int internalAddConnection(bool isUserConnection, qintptr socketDescriptor = -1);
    bool IsResponse(unsigned int serial);
    void RouteIncomingMessage(unsigned int connId, unsigned int type, unsigned int serial, QByteArray payload);
    Connection* FindConnection(const QModelIndex& index) const;

    unsigned int m_nextConnectionId;
    ConnectionMap m_connectionMap;
    RouteMultiMap m_messageRoute;
    QHostAddress m_lastHostAddress = QHostAddress::Null;
    AZ::u64 m_lastConnectionTimeInUTCMilliSecs = 0;


    // keeps track of how many platforms are connected of a given type
    // the key is the name of the platform, and the value is the number of those kind of platforms.
    QHash<QString, int> m_platformsConnected;

    //allowed listing
    bool m_allowedListingEnabled = true;

    //these lists are just caches, only used for updating
    QStringList m_allowedListAddresses;
    QStringList m_rejectedAddresses;
};


#endif // CONNECTIONMANAGER_H
