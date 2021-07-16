/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
