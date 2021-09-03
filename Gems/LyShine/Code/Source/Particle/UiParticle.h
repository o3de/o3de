/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Math/Color.h>
#include <AzCore/Math/Matrix4x4.h>
#include <AzCore/Math/Vector2.h>

#include <LmbrCentral/Rendering/MaterialAsset.h>

#include <Animation/AnimSplineTrack.h>
#include <Animation/CompoundSplineTrack.h>

#include <LyShine/UiRenderFormats.h>

class UiParticle
{
public: //types

    struct UiParticleInitialParameters
    {
        float lifetime;
        AZ::Vector2 position;
        AZ::Vector2 initialEmitterOffset;
        AZ::Vector2 initialVelocity;
        AZ::Vector2 acceleration;
        float rotation;
        float angularVelocity;
        AZ::Vector2 pivot;
        AZ::Vector2 size;
        AZ::Color color;
        int spriteSheetCellIndex;
    };

    struct UiParticleUpdateParameters
    {
        bool isParticleInfinite;
        bool isVelocityCartesian;
        bool isAccelerationCartesian;
        bool isSpeedMultiplierUsed;
        C2DSplineTrack* speedMultiplier;
    };

    struct UiParticleRenderParameters
    {
        ISprite* sprite;
        AZ::Vector2* particleOffset;
        bool isVelocityCartesian;
        bool isRelativeToEmitter;
        bool isParticleInfinite;
        bool isAspectRatioLocked;
        bool isRotationVelocityBased;
        bool isWidthMultiplierUsed;
        bool isHeightMultiplierUsed;
        bool isColorMultiplierUsed;
        bool isAlphaMultiplierUsed;
        bool isColorOverrideUsed;
        bool isAlphaOverrideUsed;
        AZ::Color colorOverride;
        float alphaOverride;
        float alphaFadeMultiplier;
        C2DSplineTrack* sizeWidthMultiplier;
        C2DSplineTrack* sizeHeightMultiplier;
        UiCompoundSplineTrack* colorMultiplier;
        C2DSplineTrack* alphaMultiplier;
        int spritesheetStartFrame;
        int spritesheetFrameRange;
        float spritesheetFrameDelay;
        bool spritesheetCellIndexAnimated;
        bool spritesheetCellIndexAnimationLooped;
    };

public:
    void Init(UiParticle::UiParticleInitialParameters* emitter);
    void Update(float timeStep, const UiParticleUpdateParameters& updateParameters);

    //! Fill out the four vertices for the particle. 
    //! Returns false if the vertex was not added because it was fully transparent.
    bool FillVertices(LyShine::UiPrimitiveVertex* outputVertices, const UiParticleRenderParameters& renderParameters, const AZ::Matrix4x4& transform);

    bool IsActive(bool infiniteLifetime) const;

private:

    float m_particleAge;
    float m_particleLifetime;

    AZ::Vector2 m_emitterInitialOffset;
    AZ::Vector2 m_position;
    AZ::Vector2 m_pivot;
    float m_rotation;

    int m_spriteCellIndex;

    AZ::Vector2 m_size;

    AZ::Vector2 m_positionDifference;
    AZ::Vector2 m_velocity;
    AZ::Vector2 m_accelerationBasedVelocity;
    AZ::Vector2 m_acceleration;
    float m_angularVelocity;

    AZ::Color m_color;
};
