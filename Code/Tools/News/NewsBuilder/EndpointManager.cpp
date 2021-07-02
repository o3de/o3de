/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EndpointManager.h"
#include <NewsShared/ResourceManagement/ResourceManifest.h>

#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QCoreApplication>

namespace News
{

    Endpoint::Endpoint(QString name, QString awsProfile, QString url, QString bucket)
        : m_name(name)
        , m_awsProfile(awsProfile)
        , m_url(url)
        , m_bucket(bucket)
    {}

    Endpoint::Endpoint(const QJsonObject& json)
        : Endpoint(json["name"].toString(),
            json["awsProfile"].toString(),
            json["url"].toString(),
            json["bucket"].toString())
    {
    }

    Endpoint::~Endpoint() {}

    void Endpoint::Write(QJsonObject& json) const
    {
        json["name"] = m_name;
        json["awsProfile"] = m_awsProfile;
        json["url"] = m_url;
        json["bucket"] = m_bucket;
    }

    QString Endpoint::GetName() const
    {
        return m_name;
    }

    void Endpoint::SetName(const QString& name)
    {
        m_name = QString(name);
    }

    QString Endpoint::GetAwsProfile() const
    {
        return m_awsProfile;
    }

    void Endpoint::SetAwsProfile(const QString& awsProfile)
    {
        m_awsProfile = QString(awsProfile);
    }

    QString Endpoint::GetUrl() const
    {
        return m_url;
    }

    void Endpoint::SetUrl(const QString& url)
    {
        m_url = QString(url);
    }

    QString Endpoint::GetBucket() const
    {
        return m_bucket;
    }

    void Endpoint::SetBucket(const QString& bucket)
    {
        m_bucket = bucket;
    }

    EndpointManager::EndpointManager()
    {
        Load();
    }

    EndpointManager::~EndpointManager()
    {
        ClearEndpoints();
    }

    void EndpointManager::Load()
    {
        ClearEndpoints();
        QFile file("newsBuilderConfig.txt");
        if (file.open(QIODevice::ReadOnly))
        {
            QTextStream in(&file);
            QString str = in.readAll().trimmed();
            QByteArray array(str.toStdString().c_str());
            QJsonDocument doc(QJsonDocument::fromJson(array));
            QJsonObject json = doc.object();
            QJsonArray endpointArray = json["endpoints"].toArray();
            for (auto endpointDoc : endpointArray)
            {
                m_endpoints.append(new Endpoint(endpointDoc.toObject()));
            }
            int endpointIndex = json["currentEndpointIndex"].toInt();
            if (endpointIndex >= 0 && m_endpoints.count() > endpointIndex)
            {
                m_selectedEndpoint = m_endpoints[endpointIndex];
            }
            else
            {
                m_selectedEndpoint = nullptr;
            }
            file.close();

            SaveUrl();
        }
    }

    void EndpointManager::Save()
    {
        QFile file("newsBuilderConfig.txt");
        if (file.open(QIODevice::WriteOnly))
        {
            QJsonObject json;
            QJsonArray endpointArray;
            for (auto endpoint : m_endpoints)
            {
                QJsonObject endpointObject;
                endpoint->Write(endpointObject);
                endpointArray.append(endpointObject);
            }
            json["endpoints"] = endpointArray;
            json["currentEndpointIndex"] = m_endpoints.indexOf(m_selectedEndpoint);
            QTextStream out(&file);
            QJsonDocument doc(json);
            out << doc.toJson(QJsonDocument::Compact);
            file.close();

            SaveUrl();
        }
    }

    void EndpointManager::SelectEndpoint(Endpoint* endpoint)
    {
        m_selectedEndpoint = endpoint;
    }

    void EndpointManager::AddEndpoint(Endpoint* endpoint)
    {
        m_endpoints.append(endpoint);
    }

    void EndpointManager::RemoveEndpoint(Endpoint* endpoint)
    {
        if (endpoint == m_selectedEndpoint)
        {
            m_selectedEndpoint = nullptr;
        }
        m_endpoints.removeAll(endpoint);
        delete endpoint;
    }

    Endpoint* EndpointManager::GetSelectedEndpoint() const
    {
        return m_selectedEndpoint;
    }

    QList<Endpoint*>::const_iterator EndpointManager::begin() const
    {
        return m_endpoints.begin();
    }

    QList<Endpoint*>::const_iterator EndpointManager::end() const
    {
        return m_endpoints.end();
    }

    void EndpointManager::ClearEndpoints()
    {
        for (auto endpoint : m_endpoints)
        {
            delete endpoint;
        }
        m_endpoints.clear();
    }

    void EndpointManager::SaveUrl() const
    {
        if (!m_selectedEndpoint)
        {
            return;
        }
        // Editor looks for config file in /dev folder, so this file is placed in parent directory
        QFile file(QCoreApplication::applicationDirPath() + "/newsConfig.txt");
        if (file.open(QIODevice::WriteOnly))
        {
            QTextStream out(&file);
            out << m_selectedEndpoint->GetUrl();
            file.close();
        }
    }

}
