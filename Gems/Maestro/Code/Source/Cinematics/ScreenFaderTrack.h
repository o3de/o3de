/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <IMovieSystem.h>
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

        //-----------------------------------------------------------------------------
        //! IAnimTrack Method Overriding.
        //-----------------------------------------------------------------------------
        void GetKeyInfo(int key, const char*& description, float& duration) override;
        void SerializeKey(IScreenFaderKey& key, XmlNodeRef& keyNode, bool bLoading) override;
        void SetFlags(int flags) override;

        void PreloadTextures();
        ITexture* GetActiveTexture() const;
        void SetScreenFaderTrackDefaults();

        bool IsTextureVisible() const
        {
            return m_bTextureVisible;
        }

        void SetTextureVisible(bool bVisible)
        {
            m_bTextureVisible = bVisible;
        }

        Vec4 GetDrawColor() const
        {
            return m_drawColor;
        }

        void SetDrawColor(Vec4 vDrawColor)
        {
            m_drawColor = vDrawColor;
        }

        int GetLastTextureID() const
        {
            return m_lastTextureID;
        }

        void SetLastTextureID(int nTextureID)
        {
            m_lastTextureID = nTextureID;
        }

        bool SetActiveTexture(int index);

        static void Reflect(AZ::ReflectContext* context);

    private:
        void ReleasePreloadedTextures();

        AZStd::vector<ITexture*> m_preloadedTextures;
        bool m_bTextureVisible;
        Vec4 m_drawColor;
        int m_lastTextureID;
    };

} // namespace Maestro
