/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <QString>

class QJsonObject;

namespace News
{
    class Descriptor;

    //! Resource is a central element of in-editor messages
    //! It represents articles, images, and anything else that is part of news feed
    class Resource
    {
    public:
        //! resources are stored as json objects in \ref News::ResourceManifest
        //! this creates resource with empty data array, that can be downloaded later
        //! by calling News::ResourceManifest::Sync
        explicit Resource(const QJsonObject& json);

        explicit Resource(const QString& id,
            const QString& type);

        Resource(const QString& id,
            const QByteArray& data,
            const QString& url,
            const QString& type,
            int refCount,
            int version);

        ~Resource();

        //! Saves resource's description to a json file
        void Write(QJsonObject& json) const;

        QString GetId() const;

        void SetId(const QString& id);

        QByteArray GetData() const;

        void SetData(QByteArray data);

        QString GetType() const;

        int GetRefCount() const;

        void SetRefCount(int refCount);

        int GetVersion() const;

        void SetVersion(int version);

    private:
        QString m_id;
        QByteArray m_data;
        QString m_type;
        int m_refCount;
        int m_version;
    };
}
