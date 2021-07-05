/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
