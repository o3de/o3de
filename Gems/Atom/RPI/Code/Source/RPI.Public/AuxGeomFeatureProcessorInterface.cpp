/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Public/AuxGeom/AuxGeomFeatureProcessorInterface.h>
#include <Atom/RPI.Public/RPISystemInterface.h>
#include <Atom/RPI.Public/Scene.h>


namespace AZ
{
    namespace RPI
    {
        AuxGeomDrawPtr AuxGeomFeatureProcessorInterface::GetDrawQueueForScene(const ScenePtr scenePtr)
        {
            return GetDrawQueueForScene(scenePtr.get());
        }

        AuxGeomDrawPtr AuxGeomFeatureProcessorInterface::GetDrawQueueForScene(const Scene* scene)
        {
            AZ_Assert(scene, "invalid scene pointer");
            if (auto auxGeomFP = scene->GetFeatureProcessor<AuxGeomFeatureProcessorInterface>())
            {
                return auxGeomFP->GetDrawQueue();
            }
            else
            {
                return nullptr;
            }
        }
    }
}
