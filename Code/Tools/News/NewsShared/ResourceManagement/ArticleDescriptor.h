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

#pragma once

#include "JsonDescriptor.h"

#include <QJsonObject>

namespace News
{
    class Resource;

    //! ArticleDescriptor represents Resource as an article
    class ArticleDescriptor
        : public JsonDescriptor
    {
    public:
        explicit ArticleDescriptor(Resource& resource);

        //! If article was modified, call this to update resource data
        void Update() const;

        const QString& GetArticleStyle() const;

        void SetArticleStyle(const QString& style);

        const QString& GetImageId() const;

        void SetImageId(const QString& imageId);

        const QString& GetTitle() const;

        void SetTitle(const QString& title);

        const QString& GetBody() const;

        void SetBody(const QString& body);

    private:
        QString m_articleStyle = "default";
        QString m_imageId;
        QString m_title;
        QString m_body;
        int m_order;
    };
}
