/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <Atom_Feature_Traits_Platform.h>
#if AZ_TRAIT_LUXCORE_SUPPORTED

#include "LuxCoreObject.h"
#include <AzCore/std/string/conversions.h>

namespace AZ
{
    namespace Render
    {
        LuxCoreObject::LuxCoreObject(AZStd::string modelAssetId, AZStd::string materialInstanceId)
        {
            Init(modelAssetId, materialInstanceId);
        }

        LuxCoreObject::LuxCoreObject(const LuxCoreObject &object)
        {
            Init(object.m_modelAssetId, object.m_materialInstanceId);
        }

        LuxCoreObject::~LuxCoreObject()
        {
        }

        luxrays::Properties LuxCoreObject::GetLuxCoreObjectProperties()
        {
            return m_luxCoreObject;
        }

        void LuxCoreObject::Init(AZStd::string modelAssetId, AZStd::string materialInstanceId)
        {
            m_modelAssetId = modelAssetId;
            m_materialInstanceId = materialInstanceId;

            static std::atomic_int ObjectId { 0 };
            const int localObjectId = ObjectId++;

            m_luxCoreObjectName = "scene.objects." + AZStd::to_string(localObjectId);
            AZStd::string shapePropertyName = m_luxCoreObjectName + ".shape";
            AZStd::string materialPropertyName = m_luxCoreObjectName + ".material";

            m_luxCoreObject = m_luxCoreObject << luxrays::Property(std::string(shapePropertyName.data()))(std::string(modelAssetId.data()));
            m_luxCoreObject = m_luxCoreObject << luxrays::Property(std::string(materialPropertyName.data()))(std::string(materialInstanceId.data()));
        }
    };
};

#endif
