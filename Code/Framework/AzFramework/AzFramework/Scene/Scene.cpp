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

#include <AzFramework/Scene/Scene.h>

namespace AzFramework
{
    Scene::Scene(AZStd::string name)
        : m_name(AZStd::move(name))
    {
    }

    Scene::Scene(AZStd::string name, AZStd::shared_ptr<Scene> parent)
        : m_name(AZStd::move(name))
        , m_parent(AZStd::move(parent))
    {
    }

    const AZStd::string& Scene::GetName() const
    {
        return m_name;
    }
}
