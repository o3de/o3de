/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ArticleDescriptor.h"
#include "Resource.h"

#include <QJsonArray>
#include <QJsonDocument>

using namespace News;

ArticleDescriptor::ArticleDescriptor(
    Resource& resource)
    : JsonDescriptor(resource)
    , m_imageId(m_json["image"].toString())
    , m_title(m_json["title"].toString())
    , m_body(m_json["body"].toString())
    , m_order(m_json["order"].toInt())
{
    if (m_json.contains("articleStyle") == true)
    {
        m_articleStyle = m_json["articleStyle"].toString();
    }
}

void ArticleDescriptor::Update() const
{
    QJsonObject json;
    json["image"] = m_imageId;
    json["title"] = m_title;
    json["body"] = m_body;
    json["order"] = m_order;
    json["articleStyle"] = m_articleStyle;
    QJsonDocument doc(json);
    QByteArray data = doc.toJson(QJsonDocument::Compact).toStdString().data();
    m_resource.SetData(data);
}

const QString& ArticleDescriptor::GetArticleStyle() const
{
    return m_articleStyle;
}

void ArticleDescriptor::SetArticleStyle(const QString& style)
{
    m_articleStyle = style;
}

const QString& ArticleDescriptor::GetImageId() const
{
    return m_imageId;
}

void ArticleDescriptor::SetImageId(const QString& imageId)
{
    m_imageId = imageId;
}

const QString& ArticleDescriptor::GetTitle() const
{
    return m_title;
}

void ArticleDescriptor::SetTitle(const QString& title)
{
    m_title = title;
}

const QString& ArticleDescriptor::GetBody() const
{
    return m_body;
}

void ArticleDescriptor::SetBody(const QString& body)
{
    m_body = body;
}
