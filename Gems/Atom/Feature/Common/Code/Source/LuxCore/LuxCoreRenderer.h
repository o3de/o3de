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

#include <Atom/Feature/LuxCore/LuxCoreBus.h>
#include "LuxCoreMaterial.h"
#include "LuxCoreMesh.h"
#include "LuxCoreObject.h"
#include "LuxCoreTexture.h"

namespace AZ
{
    namespace Render
    {
        // Hold all converted data, write scene and render file to disk when command received
        // Can be extend to do real-time rendering in the future
        class LuxCoreRenderer
            : public LuxCoreRequestsBus::Handler
        {
        public:
            LuxCoreRenderer();
            ~LuxCoreRenderer();

            ////////////////////////////////////////////////////////////////////////
            // LuxCoreRequestsBus
            void SetCameraEntityID(AZ::EntityId id);
            void AddMesh( Data::Asset<RPI::ModelAsset> modelAsset);
            void AddMaterial(Data::Instance<RPI::Material> material);
            void AddTexture(Data::Instance<RPI::Image> image, LuxCoreTextureType type);
            void AddObject(AZ::Data::Asset<AZ::RPI::ModelAsset> modelAsset, AZ::Data::InstanceId materialInstanceId);
            bool CheckTextureStatus();
            void RenderInLuxCore();
            void ClearLuxCore();
            void ClearObject();
            /////////////////////////////////////////////////////////////////////////

        private:
            AZ::EntityId m_cameraEntityId;
            AZ::Transform m_cameraTransform;

            AZStd::unordered_map<AZStd::string, LuxCoreMesh> m_meshs;
            AZStd::unordered_map<AZStd::string, LuxCoreMaterial> m_materials;
            AZStd::unordered_map<AZStd::string, LuxCoreTexture> m_textures;
            AZStd::vector<LuxCoreObject> m_objects;
        };
    }   
}
#endif
