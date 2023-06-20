/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "connectionManager.h"
#include "connection.h"
#include <QSettings>
#include <QHostInfo>
#include <QNetworkInterface>

#include "native/utilities/assetUtils.h"

namespace
{
    // singleton Pattern
    ConnectionManager* s_singleton = nullptr;


    QString TranslateStatus(int status)
    {
        switch (status)
        {
        case 0:
            return QObject::tr("Disconnected");
        case 1:
            return QObject::tr("Connected");
        case 2:
            return QObject::tr("Connecting");
        default:
            return "";
        }
    }
}

ConnectionManager::ConnectionManager(QObject* parent)
    : QAbstractItemModel(parent)
    , m_nextConnectionId(1)
{
    Q_ASSERT(!s_singleton);
    s_singleton = this;
    qRegisterMetaType<qintptr>("qintptr");
    qRegisterMetaType<quint16>("quint16");
    qRegisterMetaType<QHostAddress>("QHostAddress");

    ConnectionManagerRequestBus::Handler::BusConnect();

    QTimer::singleShot(0, this, SLOT(UpdateAllowedListFromBootStrap()));
}

void ConnectionManager::UpdateAllowedListFromBootStrap()
{
    m_allowedListAddresses.clear();
    QString allowedlist = AssetUtilities::ReadAllowedlistFromSettingsRegistry();
    AZStd::vector<AZStd::string> allowedlistaddressList;
    AzFramework::StringFunc::Tokenize(allowedlist.toUtf8().constData(), allowedlistaddressList, ", \t\n\r");
    for (const AZStd::string& allowedlistaddress : allowedlistaddressList)
    {
        m_allowedListAddresses << allowedlistaddress.c_str();
    }
    Q_EMIT SyncAllowedListAndRejectedList(m_allowedListAddresses, m_rejectedAddresses);
}


ConnectionManager::~ConnectionManager()
{
    ConnectionManagerRequestBus::Handler::BusDisconnect();
    s_singleton = nullptr;
}

ConnectionManager* ConnectionManager::Get()
{
    return s_singleton;
}

int ConnectionManager::getCount() const
{
    return m_connectionMap.size();
}

Connection* ConnectionManager::getConnection(unsigned int connectionId)
{
    auto iter = m_connectionMap.find(connectionId);
    if (iter != m_connectionMap.end())
    {
        return iter.value();
    }
    return nullptr;
}

ConnectionMap& ConnectionManager::getConnectionMap()
{
    return m_connectionMap;
}

unsigned int ConnectionManager::addConnection(qintptr socketDescriptor)
{
    const bool isUserConnection = false;

    return internalAddConnection(isUserConnection, socketDescriptor);
}

unsigned int ConnectionManager::internalAddConnection(bool isUserConnection, qintptr socketDescriptor)
{
    auto connectionId = m_nextConnectionId++;

    // If the connectionId grows, we are appending, otherwise we're inserting at the front
    if (connectionId < m_nextConnectionId)
    {
        beginInsertRows(QModelIndex(), m_connectionMap.count(), m_connectionMap.count());
    }
    else
    {
        beginInsertRows(QModelIndex(), 0, 0);
    }

    Connection* connection = new Connection(isUserConnection, socketDescriptor, this);
    connect(connection, &Connection::IsAddressInAllowedList, this, &ConnectionManager::IsAddressInAllowedList);
    connect(this, &ConnectionManager::AddressIsInAllowedList, connection, &Connection::AddressIsInAllowedList);
  
    connection->SetConnectionId(connectionId);
    connect(connection, &Connection::StatusChanged, this, &ConnectionManager::OnStatusChanged);
    connect(connection, &Connection::DeliverMessage, this, &ConnectionManager::RouteIncomingMessage);
    connect(connection, SIGNAL(DisconnectConnection(unsigned int)), this, SIGNAL(ConnectionDisconnected(unsigned int)));
    connect(connection, SIGNAL(ConnectionDestroyed(unsigned int)), this, SLOT(RemoveConnectionFromMap(unsigned int)));
    connect(connection, SIGNAL(Error(unsigned int, QString)), this, SIGNAL(ConnectionError(unsigned int, QString)));
    connect(connection, &Connection::ConnectionReady, this, &ConnectionManager::ConnectionReady);

    m_connectionMap.insert(connectionId, connection);
    Q_EMIT connectionAdded(connectionId, connection);

    endInsertRows();

    connection->Activate(socketDescriptor);

    return connectionId;
}

unsigned int ConnectionManager::addUserConnection()
{
    const bool isUserConnection = true;

    unsigned int newConnectionId = internalAddConnection(isUserConnection);

    SaveConnections();

    return newConnectionId;
}

void ConnectionManager::OnStatusChanged(unsigned int connId)
{
    QList<unsigned int> keys = m_connectionMap.keys();
    int row = keys.indexOf(connId);

    QModelIndex theIndex = index(row, 0, QModelIndex());
    QModelIndex theIndexLastColumn = index(row, Column::Max, QModelIndex());
    Q_EMIT dataChanged(theIndex, theIndexLastColumn);
    // Here, we only want to emit the bus event when the very last of a particular platform leaves, or the first one joins, so we keep a count:
    auto foundElement = m_connectionMap.find(connId);
    if (foundElement == m_connectionMap.end())
    {
        return;
    }

    Connection* connection = foundElement.value();
    QStringList assetPlatforms = connection->AssetPlatforms();

    if (connection->Status() == Connection::Connected)
    {
        int priorCount = 0;
        for (const auto& thisPlatform : assetPlatforms)
        {
            auto existingEntry = m_platformsConnected.find(thisPlatform);
            if (existingEntry == m_platformsConnected.end())
            {
                m_platformsConnected.insert(thisPlatform, 1);
            }
            else
            {
                priorCount = existingEntry.value();
                m_platformsConnected[thisPlatform] = priorCount + 1;
            }

            if (priorCount == 0)
            {
                AssetProcessorPlatformBus::Broadcast(
                    &AssetProcessorPlatformBus::Events::AssetProcessorPlatformConnected, thisPlatform.toUtf8().data());
            }
        }
    }
    else
    {
        for (const auto& thisPlatform : assetPlatforms)
        {
            // connection dropped!
            int priorCount = m_platformsConnected[thisPlatform];
            m_platformsConnected[thisPlatform] = priorCount - 1;
            if (priorCount == 1)
            {
                AssetProcessorPlatformBus::Broadcast(
                    &AssetProcessorPlatformBus::Events::AssetProcessorPlatformDisconnected, thisPlatform.toUtf8().data());
            }
        }
    }
}


QModelIndex ConnectionManager::parent(const QModelIndex& /*index*/) const
{
    return QModelIndex();
}


QModelIndex ConnectionManager::index(int row, int column, const QModelIndex& parent) const
{
    if (row >= rowCount(parent) || column >= columnCount(parent))
    {
        return QModelIndex();
    }
    return createIndex(row, column);
}


int ConnectionManager::columnCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : Column::Max;
}


int ConnectionManager::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
    {
        return 0;
    }
    return m_connectionMap.count();
}


Connection* ConnectionManager::FindConnection(const QModelIndex& index) const
{
    if (!index.isValid())
    {
        return nullptr;
    }

    if (index.row() >= m_connectionMap.count())
    {
        return nullptr;
    }

    QList<unsigned int> keys = m_connectionMap.keys();
    int key = keys[index.row()];

    auto connectionIter = m_connectionMap.find(key);

    if (connectionIter == m_connectionMap.end())
    {
        return nullptr;
    }

    return connectionIter.value();
}

QVariant ConnectionManager::data(const QModelIndex& index, int role) const
{
    Connection* connection = FindConnection(index);
    if (!connection)
    {
        return QVariant();
    }

    bool isUserConnection = connection->UserCreatedConnection();

    switch (role)
    {
    case UserConnectionRole:
        return isUserConnection;

    case Qt::ToolTipRole:
        switch (index.column())
        {
        case IdColumn:
            if (!isUserConnection)
            {
                return QObject::tr("This connection was triggered automatically by another process connecting to the Asset Processor and can not be edited");
            }
            break;
        }
        break;
    case Qt::CheckStateRole:
        if (index.column() == AutoConnectColumn && isUserConnection)
        {
            return connection->AutoConnect() ? Qt::Checked : Qt::Unchecked;
        }
        break;
    case Qt::EditRole:
    case Qt::DisplayRole:
        switch (index.column())
        {
        case StatusColumn:
            return TranslateStatus(connection->Status());
        case IdColumn:
            return connection->Identifier();
        case IpColumn:
            return connection->IpAddress();
        case PortColumn:
            return connection->Port();
        case PlatformColumn:
            return connection->AssetPlatforms().join(',');
        case AutoConnectColumn:
            if (!isUserConnection)
            {
                return tr("Auto");
            }

            return QVariant();
        }
        break;
    }

    return QVariant();
}

Qt::ItemFlags ConnectionManager::flags(const QModelIndex& index) const
{
    Connection* connection = FindConnection(index);
    if (!connection)
    {
        return QAbstractItemModel::flags(index);
    }

    bool isUserConnection = connection->UserCreatedConnection();

    if ((index.column() == IdColumn || index.column() == IpColumn || index.column() == PortColumn) && isUserConnection)
    {
        return Qt::ItemFlags(Qt::ItemIsSelectable) | Qt::ItemIsEnabled;
    }

    if (index.column() == AutoConnectColumn)
    {
        Qt::ItemFlags autoConnectFlags = Qt::ItemFlags(Qt::ItemIsSelectable) | Qt::ItemIsEnabled;
        if (isUserConnection)
        {
            autoConnectFlags |= (Qt::ItemIsUserCheckable);
        }
        
        return autoConnectFlags;
    }

    return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}


QVariant ConnectionManager::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
    {
        switch (section)
        {
        case StatusColumn:
            return tr("Status");
        case IdColumn:
            return tr("ID");
        case IpColumn:
            return tr("IP");
        case PortColumn:
            return tr("Port");
        case PlatformColumn:
            return tr("Platform");
        case AutoConnectColumn:
            return tr("Enabled");
        default:
            break;
        }
    }

    return QAbstractItemModel::headerData(section, orientation, role);
}


bool ConnectionManager::setData(const QModelIndex& index, const QVariant& value, int /*role*/)
{
    if (!index.isValid())
    {
        return false;
    }

    QList<unsigned int> keys = m_connectionMap.keys();
    int key = keys[index.row()];
    Connection* connection = m_connectionMap[key];

    switch (index.column())
    {
    case PortColumn:
        connection->SetPort(value.toInt());
        break;
    case IpColumn:
        connection->SetIpAddress(value.toString());
        break;
    case IdColumn:
        connection->SetIdentifier(value.toString());
        break;
    case AutoConnectColumn:
        connection->SetAutoConnect(value.toBool());
        break;
    default:
        break;
    }

    Q_EMIT dataChanged(index, index);

    if (connection->UserCreatedConnection())
    {
        SaveConnections();
    }

    return true;
}


unsigned int ConnectionManager::GetConnectionId(QString ipaddress, int port)
{
    for (auto iter = m_connectionMap.begin(); iter != m_connectionMap.end(); iter++)
    {
        if ((iter.value()->IpAddress().compare(ipaddress, Qt::CaseInsensitive) == 0) && iter.value()->Port() == port)
        {
            return iter.value()->ConnectionId();
        }
    }
    return 0;
}

void ConnectionManager::removeConnection(const QModelIndex& index)
{
    if (!index.isValid())
    {
        return;
    }

    if (index.row() >= m_connectionMap.count())
    {
        return;
    }


    QList<unsigned int> keys = m_connectionMap.keys();
    int key = keys[index.row()];

    removeConnection(key);

    // Normally, removing a connection will cause RemoveConnectionFromMap to be called
    // later, when the connection is fully removed. However, SaveConnections stores all
    // user created connections, so this connection needs to be removed early.
    // RemoveConnectionFromMap doesn't call SaveConnections because asset builders connect
    // and disconnection shouldn't cause the settings to get saved constantly.
    // This does mean RemoveConnectionFromMap is called twice, but that's OK because it won't find
    // the key and it will safely handle that situation.
    RemoveConnectionFromMap(key);

    SaveConnections();
}


void ConnectionManager::SaveConnections(QString settingPrefix)
{
    QSettings settings;
    settings.beginWriteArray(QStringLiteral("%1Connections").arg(settingPrefix));
    int idx = 0;
    for (auto iter = m_connectionMap.begin(); iter != m_connectionMap.end(); iter++)
    {
        if (iter.value()->UserCreatedConnection())
        {
            settings.setArrayIndex(idx);
            iter.value()->SaveConnection(settings);
            idx++;
        }
    }
    settings.endArray();
}

void ConnectionManager::LoadConnections(QString settingPrefix)
{
    QSettings settings;
    int numElement = settings.beginReadArray(QStringLiteral("%1Connections").arg(settingPrefix));
    for (int idx = 0; idx < numElement; ++idx)
    {
        settings.setArrayIndex(idx);
        getConnection(addConnection())->LoadConnection(settings);
    }
    settings.endArray();
}

void ConnectionManager::RegisterService(unsigned int type, regFunc func)
{
    m_messageRoute.insert(type, func);
}


void ConnectionManager::NewConnection(qintptr socketDescriptor)
{
    addConnection(socketDescriptor);
}

void ConnectionManager::AllowedListingEnabled(bool enabled)
{
    m_allowedListingEnabled = enabled;
}

void ConnectionManager::AddAddressToAllowedList(QString address)
{
    UpdateAllowedListFromBootStrap();
    while (m_allowedListAddresses.removeOne(address)) {}
    m_allowedListAddresses << address;
    AssetUtilities::WriteAllowedlistToSettingsRegistry(m_allowedListAddresses);
    Q_EMIT SyncAllowedListAndRejectedList(m_allowedListAddresses, m_rejectedAddresses);
}

void ConnectionManager::RemoveAddressFromAllowedList(QString address)
{
    UpdateAllowedListFromBootStrap();
    while (m_allowedListAddresses.removeOne(address)) {}
    AssetUtilities::WriteAllowedlistToSettingsRegistry(m_allowedListAddresses);
    Q_EMIT SyncAllowedListAndRejectedList(m_allowedListAddresses, m_rejectedAddresses);
}

void ConnectionManager::AddRejectedAddress(QString address, bool surpressWarning)
{
    UpdateAllowedListFromBootStrap();
    bool alreadyRejected = false;
    while (m_rejectedAddresses.removeOne(address)) { alreadyRejected = true; }
    m_rejectedAddresses << address;
    if (!surpressWarning && !alreadyRejected)
    {
        Q_EMIT FirstTimeAddedToRejctedList(address);
    }
    Q_EMIT SyncAllowedListAndRejectedList(m_allowedListAddresses, m_rejectedAddresses);
}

void ConnectionManager::RemoveRejectedAddress(QString address)
{
    UpdateAllowedListFromBootStrap();
    while (m_rejectedAddresses.removeOne(address)) {}
    Q_EMIT SyncAllowedListAndRejectedList(m_allowedListAddresses, m_rejectedAddresses);
}

void ConnectionManager::IsAddressInAllowedList(QHostAddress incominghostaddr, void* token)
{
    if (!m_allowedListingEnabled)
    {
        Q_EMIT AddressIsInAllowedList(token, true);
        return;
    }

    QString incomingIpAddress;

    if (incominghostaddr != QHostAddress::Null)
    {
        // any ipv4 address will be like ::ffff:A.B.C.D, we have to retrieve A.B.C.D for comparision with the allowedlisted addresses.
        // for example Qt will tell us the ipv6 (::ffff:127.0.0.1) for 127.0.0.1, 
        // but the allowedlist below will report ipv4 (127.0.0.1) and ipv6 (::1), and they both won't match to ipv6 (::ffff:127.0.0.1).
        bool wasConverted = false;
        quint32 incomingIPv4 = incominghostaddr.toIPv4Address(&wasConverted);
        if (wasConverted)
        {
            incomingIpAddress = QHostAddress(incomingIPv4).toString();
        }
        else
        {
            incomingIpAddress = incominghostaddr.toString();
        }
    }

    QHostInfo incomingInfo = QHostInfo::fromName(incomingIpAddress);

    QString allowedlist = AssetUtilities::ReadAllowedlistFromSettingsRegistry();

    AZStd::vector<AZStd::string> allowedlistaddressList;
    AzFramework::StringFunc::Tokenize(allowedlist.toUtf8().constData(), allowedlistaddressList, ", \t\n\r");

    // allow localhost, loopback regardless, there's no good reason to accidentally lock yourself out your own computer.
    for (const auto& address : QNetworkInterface::allAddresses()) // allAdresses returns the ip address of all local interfaces.
    {
        allowedlistaddressList.push_back(address.toString().toUtf8().constData());
    }

    // does the incoming connection match any entries?
    for (const AZStd::string& allowedListaddress : allowedlistaddressList)
    {
        //address range matching
        size_t maskLocation = allowedListaddress.find('/');
        if (maskLocation != AZStd::string::npos)
        {
            //x.x.x.x/0 is all addresses
            int mask = atoi(allowedListaddress.substr(maskLocation + 1).c_str());
            if (mask == 0)
            {
                Q_EMIT AddressIsInAllowedList(token, true);
                return;
            }
            else
            {
                //If we successfully convert to an ipv4 address then the incominghostaddr MAY have been in an ipv6 representation of
                //an ipv4 address. If this is the case the protocol of the incominghostaddr will be ipv6, this will cause the isInSubnet call
                //to fail even it correctly matches because of mismatching protocols. To get around this create a new host address from the incomingIpAddress
                //so that if it was an ipv6 representation of ipv4, then creating it directly from the ipv4 will allow the protocol check to pass
                //and wont fail the subnet check just because the ipv4 was represented in ipv6 format. This is due to an early out check QT does for protocol and in my opinion,
                //QT should first do a check and see if we are comparing convertible protocols before just early outing.
                //If it wasn't convertible to ipv4 then we know it is ipv6 in which case the protocols will match which make this unnecessary, but still correct.
                QHostAddress ha(incomingIpAddress);
                if (ha.isInSubnet(QHostAddress::parseSubnet(allowedListaddress.c_str())))
                {
                    Q_EMIT AddressIsInAllowedList(token, true);
                    return;
                }
            }
        }
        else//direct address matching
        {
            QHostInfo allowedInfo;
            QHostAddress allowedHostAddress(allowedListaddress.c_str());
            if ((allowedHostAddress.isNull()))
            {
                allowedInfo = QHostInfo::fromName(QString::fromUtf8(allowedListaddress.c_str()));
            }
            else
            {
                QList<QHostAddress> addresses;
                addresses << allowedHostAddress;
                allowedInfo.setAddresses(addresses);
            }

            for (const auto& allowedAddress : allowedInfo.addresses())
            {
                for (const auto& address : incomingInfo.addresses())
                {
                    if (address == allowedAddress)
                    {
                        Q_EMIT AddressIsInAllowedList(token, true);
                        return;
                    }
                }
            }
        }
    }

    AddRejectedAddress(incomingIpAddress);
    Q_EMIT AddressIsInAllowedList(token, false);
}

void ConnectionManager::AddBytesReceived(unsigned int connId, qint64 add, bool update)
{
    auto iter = m_connectionMap.find(connId);
    if (iter != m_connectionMap.end())
    {
        iter.value()->AddBytesReceived(add, update);
    }
}

void ConnectionManager::AddBytesSent(unsigned int connId, qint64 add, bool update)
{
    auto iter = m_connectionMap.find(connId);
    if (iter != m_connectionMap.end())
    {
        iter.value()->AddBytesSent(add, update);
    }
}

void ConnectionManager::AddBytesRead(unsigned int connId, qint64 add, bool update)
{
    auto iter = m_connectionMap.find(connId);
    if (iter != m_connectionMap.end())
    {
        iter.value()->AddBytesRead(add, update);
    }
}

void ConnectionManager::AddBytesWritten(unsigned int connId, qint64 add, bool update)
{
    auto iter = m_connectionMap.find(connId);
    if (iter != m_connectionMap.end())
    {
        iter.value()->AddBytesWritten(add, update);
    }
}

void ConnectionManager::AddOpenRequest(unsigned int connId, bool update)
{
    auto iter = m_connectionMap.find(connId);
    if (iter != m_connectionMap.end())
    {
        iter.value()->AddOpenRequest(update);
    }
}

void ConnectionManager::AddCloseRequest(unsigned int connId, bool update)
{
    auto iter = m_connectionMap.find(connId);
    if (iter != m_connectionMap.end())
    {
        iter.value()->AddCloseRequest(update);
    }
}

void ConnectionManager::AddOpened(unsigned int connId, bool update)
{
    auto iter = m_connectionMap.find(connId);
    if (iter != m_connectionMap.end())
    {
        iter.value()->AddOpened(update);
    }
}

void ConnectionManager::AddClosed(unsigned int connId, bool update)
{
    auto iter = m_connectionMap.find(connId);
    if (iter != m_connectionMap.end())
    {
        iter.value()->AddClosed(update);
    }
}

void ConnectionManager::AddReadRequest(unsigned int connId, bool update)
{
    auto iter = m_connectionMap.find(connId);
    if (iter != m_connectionMap.end())
    {
        iter.value()->AddReadRequest(update);
    }
}

void ConnectionManager::AddWriteRequest(unsigned int connId, bool update)
{
    auto iter = m_connectionMap.find(connId);
    if (iter != m_connectionMap.end())
    {
        iter.value()->AddWriteRequest(update);
    }
}

void ConnectionManager::AddTellRequest(unsigned int connId, bool update)
{
    auto iter = m_connectionMap.find(connId);
    if (iter != m_connectionMap.end())
    {
        iter.value()->AddTellRequest(update);
    }
}

void ConnectionManager::AddSeekRequest(unsigned int connId, bool update)
{
    auto iter = m_connectionMap.find(connId);
    if (iter != m_connectionMap.end())
    {
        iter.value()->AddSeekRequest(update);
    }
}

void ConnectionManager::AddIsReadOnlyRequest(unsigned int connId, bool update)
{
    auto iter = m_connectionMap.find(connId);
    if (iter != m_connectionMap.end())
    {
        iter.value()->AddIsReadOnlyRequest(update);
    }
}

void ConnectionManager::AddIsDirectoryRequest(unsigned int connId, bool update)
{
    auto iter = m_connectionMap.find(connId);
    if (iter != m_connectionMap.end())
    {
        iter.value()->AddIsDirectoryRequest(update);
    }
}

void ConnectionManager::AddSizeRequest(unsigned int connId, bool update)
{
    auto iter = m_connectionMap.find(connId);
    if (iter != m_connectionMap.end())
    {
        iter.value()->AddSizeRequest(update);
    }
}

void ConnectionManager::AddModificationTimeRequest(unsigned int connId, bool update)
{
    auto iter = m_connectionMap.find(connId);
    if (iter != m_connectionMap.end())
    {
        iter.value()->AddModificationTimeRequest(update);
    }
}

void ConnectionManager::AddExistsRequest(unsigned int connId, bool update)
{
    auto iter = m_connectionMap.find(connId);
    if (iter != m_connectionMap.end())
    {
        iter.value()->AddExistsRequest(update);
    }
}

void ConnectionManager::AddFlushRequest(unsigned int connId, bool update)
{
    auto iter = m_connectionMap.find(connId);
    if (iter != m_connectionMap.end())
    {
        iter.value()->AddFlushRequest(update);
    }
}

void ConnectionManager::AddCreatePathRequest(unsigned int connId, bool update)
{
    auto iter = m_connectionMap.find(connId);
    if (iter != m_connectionMap.end())
    {
        iter.value()->AddCreatePathRequest(update);
    }
}

void ConnectionManager::AddDestroyPathRequest(unsigned int connId, bool update)
{
    auto iter = m_connectionMap.find(connId);
    if (iter != m_connectionMap.end())
    {
        iter.value()->AddDestroyPathRequest(update);
    }
}

void ConnectionManager::AddRemoveRequest(unsigned int connId, bool update)
{
    auto iter = m_connectionMap.find(connId);
    if (iter != m_connectionMap.end())
    {
        iter.value()->AddRemoveRequest(update);
    }
}

void ConnectionManager::AddCopyRequest(unsigned int connId, bool update)
{
    auto iter = m_connectionMap.find(connId);
    if (iter != m_connectionMap.end())
    {
        iter.value()->AddCopyRequest(update);
    }
}

void ConnectionManager::AddRenameRequest(unsigned int connId, bool update)
{
    auto iter = m_connectionMap.find(connId);
    if (iter != m_connectionMap.end())
    {
        iter.value()->AddRenameRequest(update);
    }
}

void ConnectionManager::AddFindFileNamesRequest(unsigned int connId, bool update)
{
    auto iter = m_connectionMap.find(connId);
    if (iter != m_connectionMap.end())
    {
        iter.value()->AddFindFileNamesRequest(update);
    }
}

void ConnectionManager::UpdateBytesReceived(unsigned int connId)
{
    auto iter = m_connectionMap.find(connId);
    if (iter != m_connectionMap.end())
    {
        iter.value()->UpdateBytesReceived();
    }
}

void ConnectionManager::UpdateBytesSent(unsigned int connId)
{
    auto iter = m_connectionMap.find(connId);
    if (iter != m_connectionMap.end())
    {
        iter.value()->UpdateBytesSent();
    }
}

void ConnectionManager::UpdateBytesRead(unsigned int connId)
{
    auto iter = m_connectionMap.find(connId);
    if (iter != m_connectionMap.end())
    {
        iter.value()->UpdateBytesRead();
    }
}

void ConnectionManager::UpdateBytesWritten(unsigned int connId)
{
    auto iter = m_connectionMap.find(connId);
    if (iter != m_connectionMap.end())
    {
        iter.value()->UpdateBytesWritten();
    }
}

void ConnectionManager::UpdateOpenRequest(unsigned int connId)
{
    auto iter = m_connectionMap.find(connId);
    if (iter != m_connectionMap.end())
    {
        iter.value()->UpdateOpenRequest();
    }
}

void ConnectionManager::UpdateCloseRequest(unsigned int connId)
{
    auto iter = m_connectionMap.find(connId);
    if (iter != m_connectionMap.end())
    {
        iter.value()->UpdateCloseRequest();
    }
}

void ConnectionManager::UpdateOpened(unsigned int connId)
{
    auto iter = m_connectionMap.find(connId);
    if (iter != m_connectionMap.end())
    {
        iter.value()->UpdateOpened();
    }
}

void ConnectionManager::UpdateClosed(unsigned int connId)
{
    auto iter = m_connectionMap.find(connId);
    if (iter != m_connectionMap.end())
    {
        iter.value()->UpdateClosed();
    }
}

void ConnectionManager::UpdateReadRequest(unsigned int connId)
{
    auto iter = m_connectionMap.find(connId);
    if (iter != m_connectionMap.end())
    {
        iter.value()->UpdateReadRequest();
    }
}

void ConnectionManager::UpdateWriteRequest(unsigned int connId)
{
    auto iter = m_connectionMap.find(connId);
    if (iter != m_connectionMap.end())
    {
        iter.value()->UpdateWriteRequest();
    }
}

void ConnectionManager::UpdateTellRequest(unsigned int connId)
{
    auto iter = m_connectionMap.find(connId);
    if (iter != m_connectionMap.end())
    {
        iter.value()->UpdateTellRequest();
    }
}

void ConnectionManager::UpdateSeekRequest(unsigned int connId)
{
    auto iter = m_connectionMap.find(connId);
    if (iter != m_connectionMap.end())
    {
        iter.value()->UpdateSeekRequest();
    }
}

void ConnectionManager::UpdateIsReadOnlyRequest(unsigned int connId)
{
    auto iter = m_connectionMap.find(connId);
    if (iter != m_connectionMap.end())
    {
        iter.value()->UpdateIsReadOnlyRequest();
    }
}

void ConnectionManager::UpdateIsDirectoryRequest(unsigned int connId)
{
    auto iter = m_connectionMap.find(connId);
    if (iter != m_connectionMap.end())
    {
        iter.value()->UpdateIsDirectoryRequest();
    }
}

void ConnectionManager::UpdateSizeRequest(unsigned int connId)
{
    auto iter = m_connectionMap.find(connId);
    if (iter != m_connectionMap.end())
    {
        iter.value()->UpdateSizeRequest();
    }
}

void ConnectionManager::UpdateModificationTimeRequest(unsigned int connId)
{
    auto iter = m_connectionMap.find(connId);
    if (iter != m_connectionMap.end())
    {
        iter.value()->UpdateModificationTimeRequest();
    }
}

void ConnectionManager::UpdateExistsRequest(unsigned int connId)
{
    auto iter = m_connectionMap.find(connId);
    if (iter != m_connectionMap.end())
    {
        iter.value()->UpdateExistsRequest();
    }
}

void ConnectionManager::UpdateFlushRequest(unsigned int connId)
{
    auto iter = m_connectionMap.find(connId);
    if (iter != m_connectionMap.end())
    {
        iter.value()->UpdateFlushRequest();
    }
}

void ConnectionManager::UpdateCreatePathRequest(unsigned int connId)
{
    auto iter = m_connectionMap.find(connId);
    if (iter != m_connectionMap.end())
    {
        iter.value()->UpdateCreatePathRequest();
    }
}

void ConnectionManager::UpdateDestroyPathRequest(unsigned int connId)
{
    auto iter = m_connectionMap.find(connId);
    if (iter != m_connectionMap.end())
    {
        iter.value()->UpdateDestroyPathRequest();
    }
}

void ConnectionManager::UpdateRemoveRequest(unsigned int connId)
{
    auto iter = m_connectionMap.find(connId);
    if (iter != m_connectionMap.end())
    {
        iter.value()->UpdateRemoveRequest();
    }
}

void ConnectionManager::UpdateCopyRequest(unsigned int connId)
{
    auto iter = m_connectionMap.find(connId);
    if (iter != m_connectionMap.end())
    {
        iter.value()->UpdateCopyRequest();
    }
}

void ConnectionManager::UpdateRenameRequest(unsigned int connId)
{
    auto iter = m_connectionMap.find(connId);
    if (iter != m_connectionMap.end())
    {
        iter.value()->UpdateRenameRequest();
    }
}

void ConnectionManager::UpdateFindFileNamesRequest(unsigned int connId)
{
    auto iter = m_connectionMap.find(connId);
    if (iter != m_connectionMap.end())
    {
        iter.value()->UpdateFindFileNamesRequest();
    }
}

void ConnectionManager::UpdateConnectionMetrics()
{
    for (auto iter = m_connectionMap.begin(); iter != m_connectionMap.end(); iter++)
    {
        iter.value()->UpdateMetrics();
    }
}

bool ConnectionManager::IsResponse(unsigned int serial)
{
    return serial & AzFramework::AssetSystem::RESPONSE_SERIAL_FLAG;
}

void ConnectionManager::RouteIncomingMessage(unsigned int connId, unsigned int type, unsigned int serial, QByteArray payload)
{
    if (IsResponse(serial))
    {
        serial &= ~AzFramework::AssetSystem::RESPONSE_SERIAL_FLAG; // Clear the bit
        getConnection(connId)->InvokeResponseHandler(serial, type, payload);
    }
    else
    {
        SendMessageToService(connId, type, serial, payload);
    }
}

void ConnectionManager::SendMessageToService(unsigned int connId, unsigned int type, unsigned int serial, QByteArray payload)
{
    QString platform = getConnection(connId)->AssetPlatforms().join(',');
    auto iter = m_messageRoute.find(type);
    while (iter != m_messageRoute.end() && iter.key() == type)
    {
        iter.value()(connId, type, serial, payload, platform);
        iter++;
    }
}

//Entry point to removing a connection
//Callable from the GUI or the app about to close
void ConnectionManager::removeConnection(unsigned int connectionId)
{
    Connection* conn = getConnection(connectionId);
    if (conn)
    {
        Q_EMIT beforeConnectionRemoved(connectionId);
        conn->SetAutoConnect(false);
        conn->Terminate();
    }
}


void ConnectionManager::RemoveConnectionFromMap(unsigned int connectionId)
{
    auto iter = m_connectionMap.find(connectionId);
    if (iter != m_connectionMap.end())
    {
        QList<unsigned int> keys = m_connectionMap.keys();
        int row = keys.indexOf(connectionId);

        beginRemoveRows(QModelIndex(), row, row);

        m_connectionMap.erase(iter);
        Q_EMIT ConnectionRemoved(connectionId);

        endRemoveRows();
    }
}

void ConnectionManager::QuitRequested()
{
    QList<Connection*> temporaryList;
    for (auto iter = m_connectionMap.begin(); iter != m_connectionMap.end(); iter++)
    {
        temporaryList.append(iter.value());
    }

    for (auto iter = temporaryList.begin(); iter != temporaryList.end(); iter++)
    {
        (*iter)->Terminate();
    }
    QTimer::singleShot(0, this, SLOT(MakeSureConnectionMapEmpty()));
}

void ConnectionManager::MakeSureConnectionMapEmpty()
{
    if (!m_connectionMap.isEmpty())
    {
        // keep trying to shut connections down, in case some is in an interesting state, where one was being negotiated while we died.
        // note that the end of QuitRequested will ultimately result in this being tried again next time.
        QuitRequested();
    }
    else
    {
        Q_EMIT ReadyToQuit(this);
    }
}


