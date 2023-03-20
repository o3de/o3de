/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SceneAPI/SDKWrapper/NodeWrapper.h>

namespace AZ
{
    namespace SDKNode
    {
        const char* NodeWrapper::GetName() const
        {
            return "";
        }
        AZ::u64 NodeWrapper::GetUniqueId() const
        {
            return 0;
        }

        int NodeWrapper::GetMaterialCount() const
        {
            return -1;
        }

        int NodeWrapper::GetChildCount()const
        {
            return -1;
        }
        const std::shared_ptr<NodeWrapper> NodeWrapper::GetChild([[maybe_unused]] int childIndex) const
        {
            return {};
        }

    }// namespace Node
}//namespace AZ
