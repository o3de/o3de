/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "AnimNode.h"

namespace Maestro
{

    class CScreenFaderTrack;

    class CAnimScreenFaderNode : public CAnimNode
    {
    public:
        AZ_CLASS_ALLOCATOR(CAnimScreenFaderNode, AZ::SystemAllocator);
        AZ_RTTI(CAnimScreenFaderNode, "{C24D5F2D-B17A-4350-8381-539202A99FDD}", CAnimNode);

        CAnimScreenFaderNode();
        explicit CAnimScreenFaderNode(const int id);

        ~CAnimScreenFaderNode();

        static void Initialize();

        //-----------------------------------------------------------------------------
        //! Overrides from CAnimNode
        void Animate(SAnimContext& ac) override;

        void CreateDefaultTracks() override;

        void OnReset() override;

        void Activate(bool bActivate) override;

        void Serialize(XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks) override;

        //-----------------------------------------------------------------------------
        //! Overrides from IAnimNode
        unsigned int GetParamCount() const override;
        CAnimParamType GetParamType(unsigned int nIndex) const override;
        void SetFlags(int flags) override;

        void Render() override;

        bool IsAnyTextureVisible() const;

        static void Reflect(AZ::ReflectContext* context);

    protected:
        bool GetParamInfoFromType(const CAnimParamType& paramId, SParamInfo& info) const override;

        bool NeedToRender() const override
        {
            return true;
        }

    private:
        CAnimScreenFaderNode(const CAnimScreenFaderNode&);

    private:
        void PrecacheTexData();

        Vec4 m_startColor;
        bool m_bActive;
        float m_screenWidth, m_screenHeight;
        int m_lastActivatedKey;

        bool m_texPrecached;
    };

} // namespace Maestro
