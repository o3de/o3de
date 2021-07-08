/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "NewsShared/ResourceManagement/Descriptor.h"

#include <QString>
#include <functional>

namespace News
{
    class S3Connector;

    class DeleteDescriptor
        : public Descriptor
    {
    public:
        explicit DeleteDescriptor(Resource& resource);

        void Delete(S3Connector& s3Connector,
            std::function<void()> deleteSuccessCallback,
            std::function<void(QString)> deleteFailCallback) const;
    };
}
