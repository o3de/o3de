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

#include <Atom_Feature_Traits_Platform.h>
#if AZ_TRAIT_LUXCORE_SUPPORTED

#include <AzCore/std/string/string.h>
#include <luxcore/luxcore.h>

namespace AZ
{
    namespace Render
    {
        // Object holds mesh and material in LuxCore
        class LuxCoreObject final
        {
        public:
            LuxCoreObject(AZStd::string modelAssetId, AZStd::string materialInstanceId);
            LuxCoreObject(const LuxCoreObject &object);
            ~LuxCoreObject();

            luxrays::Properties GetLuxCoreObjectProperties();

        private:

            void Init(AZStd::string modelAssetId, AZStd::string materialInstanceId);
            AZStd::string m_luxCoreObjectName;
            luxrays::Properties m_luxCoreObject;

            AZStd::string m_modelAssetId;
            AZStd::string m_materialInstanceId;
        };
    };
};
#endif
