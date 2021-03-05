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

#include "Resource.h"

#include <QJsonObject>

using namespace News;

Resource::Resource(const QJsonObject& json)
    : Resource(
        json["id"].toString(),
        QByteArray(),
        json["url"].toString(),
        json["type"].toString(),
        json["refCount"].toInt(),
        json["version"].toInt()) {}

Resource::Resource(const QString& id, const QString& type)
    : Resource(
        id, 
        QByteArray(),
        "",
        type,
        1,
        0) {}

Resource::Resource(const QString& id,
    const QByteArray& data,
    [[maybe_unused]] const QString& url,
    const QString& type,
    int refCount,
    int version)
    : m_id(id)
    , m_data(data)
    , m_type(type)
    , m_refCount(refCount)
    , m_version(version) {}

Resource::~Resource() {}

void Resource::Write(QJsonObject& json) const
{
    json["id"] = m_id;
    json["type"] = m_type;
    json["refCount"] = m_refCount;
    json["version"] = m_version;
}

QString Resource::GetId() const
{
    return m_id;
}

void Resource::SetId(const QString& id)
{
    m_id = id;
}

QByteArray Resource::GetData() const
{
    return m_data;
}

void Resource::SetData(QByteArray data)
{
    m_data = data;
}

QString Resource::GetType() const
{
    return m_type;
}

int Resource::GetRefCount() const
{
    return m_refCount;
}

void Resource::SetRefCount(int refCount)
{
    m_refCount = refCount;
}

int Resource::GetVersion() const
{
    return m_version;
}

void Resource::SetVersion(int version)
{
    m_version = version;
}
