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
