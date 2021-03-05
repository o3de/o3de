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

    void ReadyToQuit(QObject* source);

    void SyncWhiteListAndRejectedList(QStringList whiteList, QStringList rejectedList);

    // this is a response to the whitelist request with that same token.
    void AddressIsWhiteListed(void* token, bool result);

    void FirstTimeAddedToRejctedList(QString ipAddress);

public Q_SLOTS:
    void SendMessageToService(unsigned int connId, unsigned int type, unsigned int serial, QByteArray payload);
    void QuitRequested();
    void RemoveConnectionFromMap(unsigned int connectionId);
    void MakeSureConnectionMapEmpty();
    void NewConnection(qintptr socketDescriptor);
    
    void WhiteListingEnabled(bool enabled);
    void IsAddressWhiteListed(QHostAddress hostAddress, void* token);
    void AddWhiteListedAddress(QString address);
    void RemoveWhiteListedAddress(QString address);
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
    
    void UpdateWhiteListFromBootStrap();

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

    //white listing
    bool m_whiteListingEnabled = true;

    //these lists are just caches, only used for updating    
    QStringList m_whiteListedAddresses;
    QStringList m_rejectedAddresses;
};


#endif // CONNECTIONMANAGER_H
