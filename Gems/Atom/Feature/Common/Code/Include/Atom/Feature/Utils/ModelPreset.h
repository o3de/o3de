/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/Memory.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/string/string.h>
#include <Atom/RPI.Reflect/Model/ModelAsset.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAsset.h>

namespace AZ
{
    namespace Render
    {
        //! ModelPreset describes a model that can be displayed in the viewport
        struct ModelPreset final
        {
            AZ_TYPE_INFO(AZ::Render::ModelPreset, "{A7304AE2-EC26-44A4-8C00-89D9731CCB13}");
            AZ_CLASS_ALLOCATOR(ModelPreset, AZ::SystemAllocator, 0);
            static void Reflect(AZ::ReflectContext* context);

            static constexpr AZStd::string_view Extension = "modelpreset.azasset";
            AZ::Data::Asset<AZ::RPI::ModelAsset> m_modelAsset;
        };

        using ModelPresetPtr = AZStd::shared_ptr<ModelPreset>;
        using ModelPresetPtrVector = AZStd::vector<ModelPresetPtr>;
    }
}
