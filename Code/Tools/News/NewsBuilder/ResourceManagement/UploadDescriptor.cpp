/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "NewsBuilder/S3Connector.h"
#include "UploadDescriptor.h"
#include "NewsShared/ResourceManagement/Resource.h"

#include <functional>
#include <aws/core/utils/memory/stl/AwsStringStream.h>
#include <aws/core/utils/memory/stl/AWSStreamFwd.h>

#include <QObject>

namespace News
{

    UploadDescriptor::UploadDescriptor(Resource& resource)
        : Descriptor(resource) {}

    void UploadDescriptor::Upload(S3Connector& s3Connector,
        std::function<void(QString)> uploadSuccessCallback,
        std::function<void(QString)> uploadFailCallback) const
    {
        Aws::String error;

        auto ss =
            Aws::MakeShared<Aws::StringStream>(S3Connector::ALLOCATION_TAG,
                std::ios::in | std::ios::out | std::ios::binary);
        auto* pbuf = ss->rdbuf();
        pbuf->sputn(m_resource.GetData().data(), m_resource.GetData().size());
        Aws::String awsUrl;

        if (!s3Connector.PutObject(
            m_resource.GetId().toStdString().c_str(),
            ss,
            m_resource.GetData().size(),
            awsUrl,
            error))
        {
            uploadFailCallback(QObject::tr("Error uploading resource %1: %2")
                .arg(m_resource.GetId())
                .arg(error.c_str()));
        }
        else
        {
            uploadSuccessCallback(QString(awsUrl.c_str()));
        }
    }

}
