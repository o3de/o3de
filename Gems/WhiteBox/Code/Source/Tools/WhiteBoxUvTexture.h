/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/std/containers/unordered_map.h>

#include <QImage>
#include <QString>

namespace AZ::RPI
{
    class MaterialAsset;
}

namespace WhiteBox
{
    //! The base colour texture of a material, read from its source image so the UV editor can draw it.
    class UvTextureCache
    {
    public:
        //! What Find learned about a material.
        struct Result
        {
            QImage m_image;       //!< Null when the material has no base colour texture Qt can read.
            QString m_name;       //!< The texture's file name, for display.
            bool m_pending = false; //!< The material is still loading; ask again later.
        };

        //! @param materialId invalid means the built-in White Box material.
        Result Find(AZ::Data::AssetId materialId);

    private:
        AZStd::unordered_map<AZ::Data::AssetId, Result> m_done;
        AZStd::unordered_map<AZ::Data::AssetId, AZ::Data::Asset<AZ::RPI::MaterialAsset>> m_loading;
    };
} // namespace WhiteBox
