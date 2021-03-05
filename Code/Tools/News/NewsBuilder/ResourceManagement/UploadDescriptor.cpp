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

        bool good = ss->good();
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
