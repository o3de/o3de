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

#include "DeleteDescriptor.h"
#include "NewsBuilder/S3Connector.h"
#include "NewsShared/ResourceManagement/Resource.h"

#include <functional>

namespace News
{

    DeleteDescriptor::DeleteDescriptor(Resource& resource)
        : Descriptor(resource) {}

    void DeleteDescriptor::Delete(S3Connector& s3Connector,
        std::function<void()> deleteSuccessCallback,
        std::function<void(QString)> deleteFailCallback) const
    {
        Aws::String error;
        if (!s3Connector.DeleteObject(
            m_resource.GetId().toStdString().c_str(),
            error))
        {
            deleteFailCallback(QString("Error deleting resource %1: %2")
                .arg(m_resource.GetId())
                .arg(error.c_str()));
        }
        else
        {
            deleteSuccessCallback();
        }
    }

}
