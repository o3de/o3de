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

#include <Atom/RPI.Reflect/Shader/ShaderResourceGroupAsset.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

namespace AZ
{
    class ReflectContext;

    namespace RPI
    {
        struct SceneDescriptor final
        {
            AZ_TYPE_INFO(SceneDescriptor, "{B561F34F-A60A-4A02-82FD-E8A4A032BFDB}");
            static void Reflect(AZ::ReflectContext* context);

            //! List of feature processors which the scene will initially enable.
            AZStd::vector<AZStd::string> m_featureProcessorNames;
        };
    } // namespace RPI
} // namespace AZ