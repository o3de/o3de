/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <QList>
#include <qobjectdefs.h>

class QJsonObject;

namespace News
{
    //! Endpoint represents a location of news data
    class Endpoint
    {
    public:
        Endpoint(QString name, QString awsProfile, QString url, QString bucket);
        explicit Endpoint(const QJsonObject& json);
        ~Endpoint();

        void Write(QJsonObject& json) const;

        QString GetName() const;
        void SetName(const QString& name);

        QString GetAwsProfile() const;
        void SetAwsProfile(const QString& awsProfile);

        QString GetUrl() const;
        void SetUrl(const QString& url);

        QString GetBucket() const;
        void SetBucket(const QString& bucket);

    private:
        //! name of endpoint to distinguish with others
        QString m_name;
        //! name of aws credentials file
        QString m_awsProfile;
        //! url location of news data (e.g. cloudfront url)
        QString m_url;
        //! name of s3 bucket where news data resides
        QString m_bucket;
    };

    //! EndpointManager handles endpoint collection
    class EndpointManager
    {
    public:
        EndpointManager();
        ~EndpointManager();

        //! Load endpoints file
        void Load();
        //! Save endpoints file
        void Save();
        void SelectEndpoint(Endpoint* endpoint);
        void AddEndpoint(Endpoint* endpoint);
        void RemoveEndpoint(Endpoint* endpoint);
        Endpoint* GetSelectedEndpoint() const;

        QList<Endpoint*>::const_iterator begin() const;
        QList<Endpoint*>::const_iterator end() const;

    private:
        QList<Endpoint*> m_endpoints;
        Endpoint* m_selectedEndpoint = nullptr;

        void ClearEndpoints();
        //! Saves news config file which is used by LY Editor to determine news location
        void SaveUrl() const;
    };
}
