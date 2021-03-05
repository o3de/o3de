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

#include "AzToolsFramework_precompiled.h"
#include "LayerResult.h"

#include <QString>

namespace AzToolsFramework
{
    namespace Layers
    {
        void LayerResult::MessageResult()
        {
            switch (m_result)
            {
            case AzToolsFramework::Layers::LayerResultStatus::Success:
                // Nothing to message on a success.
                break;
            case AzToolsFramework::Layers::LayerResultStatus::Error:
                AZ_Error("Layer", false, m_message.toUtf8().data());
                break;
            case AzToolsFramework::Layers::LayerResultStatus::Warning:
                AZ_Warning("Layer", false, m_message.toUtf8().data());
                break;
            default:
                AZ_Warning("Layer", false, "Unknown layer error.");
                AZ_Error("Layer", false, m_message.toUtf8().data());
                break;
            }
        }
    }
}
