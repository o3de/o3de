/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "EMotionFXConfig.h"
#include "AnimGraphNode.h"


namespace EMotionFX
{
    /**
     *
     */
    class EMFX_API BlendTreeSmoothingNode
        : public AnimGraphNode
    {
    public:
        AZ_RTTI(BlendTreeSmoothingNode, "{80D8C793-3CD4-4216-B804-CC00EAD20FAA}", AnimGraphNode)
        AZ_CLASS_ALLOCATOR_DECL

        enum
        {
            INPUTPORT_DEST              = 0,
            OUTPUTPORT_RESULT           = 0
        };

        enum
        {
            PORTID_INPUT_DEST           = 0,
            PORTID_OUTPUT_RESULT        = 0
        };

        class EMFX_API UniqueData
            : public AnimGraphNodeData
        {
            EMFX_ANIMGRAPHOBJECTDATA_IMPLEMENT_LOADSAVE
        public:
            AZ_CLASS_ALLOCATOR_DECL

            UniqueData(AnimGraphNode* node, AnimGraphInstance* animGraphInstance);

            void Update() override;

        public:
            float m_frameDeltaTime = 0.0f;
            float m_currentValue = 0.0f;
        };

        BlendTreeSmoothingNode();
        ~BlendTreeSmoothingNode();

        bool InitAfterLoading(AnimGraph* animGraph) override;

        void Rewind(AnimGraphInstance* animGraphInstance) override;
        AZ::Color GetVisualColor() const override;
        bool GetSupportsDisable() const override;

        const char* GetPaletteName() const override;
        AnimGraphObject::ECategory GetPaletteCategory() const override;

        AnimGraphObjectData* CreateUniqueData(AnimGraphInstance* animGraphInstance) override { return aznew UniqueData(this, animGraphInstance); }

        void SetInterpolationSpeed(float interpolationSpeed);
        void SetStartVAlue(float startValue);
        void SetUseStartVAlue(bool useStartValue);

        static void Reflect(AZ::ReflectContext* context);

    private:
        void Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
        AZ::Crc32 GetStartValueVisibility() const;

        float               m_interpolationSpeed = 0.75f;
        float               m_startValue = 0.0f;
        float               m_snapTolerance = 0.01f;
        bool                m_useStartValue = false;
    };
} // namespace EMotionFX
