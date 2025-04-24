/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <IMovieSystem.h>

#include <Atom/RPI.Reflect/Image/Image.h>
#include <AtomCore/Instance/InstanceData.h>
#include <AzCore/std/containers/vector.h>

#include "AnimTrack.h"

namespace Maestro
{

    class CScreenFaderTrack : public TAnimTrack<IScreenFaderKey>
    {
    public:
        AZ_CLASS_ALLOCATOR(CScreenFaderTrack, AZ::SystemAllocator);
        AZ_RTTI(CScreenFaderTrack, "{3279BB19-D32D-482E-BD6E-C2DCD8858328}", IAnimTrack);

        CScreenFaderTrack();
        ~CScreenFaderTrack();

        //! IAnimTrack Method Overriding.
        void GetKeyInfo(int keyIndex, const char*& description, float& duration) const override;
        void SerializeKey(IScreenFaderKey& key, XmlNodeRef& keyNode, bool bLoading) override;
        void SetFlags(int flags) override;
        void SetKey(int keyIndex, IKey* key) override;
        //~ IAnimTrack Method Overriding.

        void PreloadTextures();

        AZ::Data::Instance<AZ::RPI::Image> GetActiveTexture() const;
        bool SetActiveTexture(int keyIndex);

        void SetScreenFaderTrackDefaults();

        bool IsTextureVisible() const { return m_bTextureVisible; }
        void SetTextureVisible(bool bVisible) { m_bTextureVisible = bVisible; }

        AZ::Vector4 GetDrawColor() const { return m_drawColor; }
        void SetDrawColor(AZ::Vector4 vDrawColor) { m_drawColor = vDrawColor; }

        static void Reflect(AZ::ReflectContext* context);

    private:
        AZStd::vector<AZ::Data::Instance<AZ::RPI::Image>> m_preloadedTextures;
        bool m_bTextureVisible;
        AZ::Vector4 m_drawColor;
        int m_activeTextureNumber;
    };

} // namespace Maestro
