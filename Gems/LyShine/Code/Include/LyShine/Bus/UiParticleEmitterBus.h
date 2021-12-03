/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Math/Color.h>
#include <AzFramework/Entity/EntityContextBus.h>

class ISprite;

////////////////////////////////////////////////////////////////////////////////////////////////////
//! Bus for making requests to the UiParticleEmitterComponent
class UiParticleEmitterInterface
    : public AZ::ComponentBus
{
public: // types

    enum class EmitShape
    {
        Point,      //!< particles are emitted from the emitter position along the given angle
        Circle,     //!< particles are emitted from a circle whose radius is the minimum of the element width and height
        Quad        //!< particles are emitted from the quad with the same size as the element width and height
    };

    enum class ParticleCoordinateType
    {
        Cartesian,  //!< particles move using X,Y co-ordinates
        Polar       //!< particles move using X: radial speed, Y: angular speed
    };

    enum class ParticleInitialDirectionType
    {
        RelativeToEmitAngle,    //!< particle initial direction is based on the emit angle
        RelativeToEmitterCenter //!< particle initial direction is directed away from the emitter shape center
    };

    enum class ParticleKeyframeTangentType
    {
        EaseIn,     //!< A zero/flat tangent, a keyframe with ease in + ease out would act like an x^3 curve at the origin
        EaseOut,    //!< A zero/flat tangent, a keyframe with ease in + ease out would act like an x^3 curve at the origin
        Linear,     //!< The curve moves linearly from this keyframe towards the next/previous keyframe
        Step        //!< The curve jumps from the current keyframe value to the next/previous keyframe value
    };

    struct ParticleFloatKeyframe
    {
        AZ_TYPE_INFO(ParticleFloatKeyframe, "{85DF04FE-F614-47C1-B88A-A9F7D4826F6F}");

        float time = 0.0f;
        float multiplier = 1.0f;
        ParticleKeyframeTangentType inTangent = ParticleKeyframeTangentType::EaseIn;
        ParticleKeyframeTangentType outTangent = ParticleKeyframeTangentType::EaseOut;
    };

    struct ParticleColorKeyframe
    {
        AZ_TYPE_INFO(ParticleColorKeyframe, "{22B0CBC0-21A5-44E3-90C5-9BFEF6E4E3C5}");

        float time = 0.0f;
        AZ::Color color = AZ::Color(1.0f, 1.0f, 1.0f, 1.0f);
        ParticleKeyframeTangentType inTangent = ParticleKeyframeTangentType::EaseIn;
        ParticleKeyframeTangentType outTangent = ParticleKeyframeTangentType::EaseOut;
    };

public: // member functions

    virtual ~UiParticleEmitterInterface() {}

    //! Gets whether the emitter is emitting particles
    virtual bool GetIsEmitting() = 0;

    //! Sets whether the emitter is emitting particles
    virtual void SetIsEmitting(bool emitParticles) = 0;

    //! Gets whether the emitter uses a fixed random seed
    virtual bool GetIsRandomSeedFixed() = 0;

    //! Sets whether the emitter uses a fixed random seed
    virtual void SetIsRandomSeedFixed(bool randomSeedFixed) = 0;

    //! Gets the emitter random seed
    virtual int GetRandomSeed() = 0;

    //! Sets the emitter random seed
    virtual void SetRandomSeed(int randomSeed) = 0;

    //! Gets whether the particles move relative to the emitter position
    virtual bool GetIsParticlePositionRelativeToEmitter() = 0;

    //! Sets whether the particles move relative to the emitter position
    virtual void SetIsParticlePositionRelativeToEmitter(bool relativeToEmitter) = 0;

    //! Gets the amount of particles emitted per second
    virtual float GetParticleEmitRate() = 0;

    //! Sets the amount of particles emitted per second
    virtual void SetParticleEmitRate(float particleEmitRate) = 0;

    //! Gets whether the emitter starts emitting on activate
    virtual bool GetIsEmitOnActivate() = 0;

    //! Sets whether the emitter starts emitting on activate
    virtual void SetIsEmitOnActivate(bool emitOnActivate) = 0;

    //! Gets whether the average particle count is reached as soon as the emitter starts emitting
    virtual bool GetIsHitParticleCountOnActivate() = 0;

    //! Sets whether the average particle count is reached as soon as the emitter starts emitting
    virtual void SetIsHitParticleCountOnActivate(bool hitParticleCountOnActivate) = 0;

    //! Gets whether the emitter lifetime is infinite
    virtual bool GetIsEmitterLifetimeInfinite() = 0;

    //! Sets whether the emitter lifetime is infinite
    virtual void SetIsEmitterLifetimeInfinite(bool emitterLifetimeInfinite) = 0;

    //! Gets the lifetime of the emitter (in seconds)
    virtual float GetEmitterLifetime() = 0;

    //! Sets the lifetime of the emitter (in seconds)
    virtual void SetEmitterLifetime(float emitterLifetime) = 0;

    //! Gets whether there is a cap on the amount of active particles
    virtual bool GetIsParticleCountLimited() = 0;

    //! Sets whether there is a cap on the amount of active particles
    virtual void SetIsParticleCountLimited(bool particleCountLimited) = 0;

    //! Gets the maximum amount of active particles
    virtual AZ::u32 GetMaxParticles() = 0;

    //! Sets the maximum amount of active particles
    virtual void SetMaxParticles(AZ::u32 maxParticles) = 0;

    //! Gets the shape of the emitter
    virtual EmitShape GetEmitterShape() = 0;

    //! Sets the shape of the emitter
    virtual void SetEmitterShape(EmitShape emitterShape) = 0;

    //! Gets whether the particles are emitted on the edge of the emitter shape
    virtual bool GetIsEmitOnEdge() = 0;

    //! Sets whether the particles are emitted on the edge of the emitter shape
    virtual void SetIsEmitOnEdge(bool emitOnEdge) = 0;

    //! Gets the inside distance from the emitter shape edge that particles should emit from
    virtual float GetInsideEmitDistance() = 0;

    //! Sets the inside distance from the emitter shape edge that particles should emit from
    virtual void SetInsideEmitDistance(float insideEmitDistance) = 0;

    //! Gets the outside distance from the emitter shape edge that particles should emit from
    virtual float GetOutsideEmitDistance() = 0;

    //! Sets the outside distance from the emitter shape edge that particles should emit from
    virtual void SetOutsideEmitDistance(float outsideEmitDistance) = 0;

    //! Gets how the initial direction is calculated for Cartesian movement.
    virtual ParticleInitialDirectionType GetParticleInitialDirectionType() = 0;

    //! Sets how the initial direction is calculated for Cartesian movement.
    virtual void SetParticleInitialDirectionType(ParticleInitialDirectionType initialDirectionType) = 0;

    //! Gets the angle that particles are emitted from (in degrees clockwise measured from straight up)
    virtual float GetEmitAngle() = 0;

    //! Sets the angle that particles are emitted from (in degrees clockwise measured from straight up)
    virtual void SetEmitAngle(float emitAngle) = 0;

    //! Gets the variation in the angle that particles are emitted from (in degrees, with 10 degrees variation being +/- 10 degrees about the emit angle)
    virtual float GetEmitAngleVariation() = 0;

    //! Sets the variation in the angle that particles are emitted from (in degrees, with 10 degrees variation being +/- 10 degrees about the emit angle)
    virtual void SetEmitAngleVariation(float emitAngleVariation) = 0;

    //! Gets whether the particle lifetime is infinite
    virtual bool GetIsParticleLifetimeInfinite() = 0;

    //! Sets whether the particle lifetime is infinite
    virtual void SetIsParticleLifetimeInfinite(bool infiniteLifetime) = 0;

    //! Gets the particle life time (in seconds)
    virtual float GetParticleLifetime() = 0;

    //! Sets the particle life time (in seconds)
    virtual void SetParticleLifetime(float lifetime) = 0;

    //! Gets the particle life time variation (in seconds, 1 second variation being up to +/- 1 second about the lifetime)
    virtual float GetParticleLifetimeVariation() = 0;

    //! Sets the particle life time variation (in seconds, 1 second variation being up to +/- 1 second about the lifetime)
    virtual void SetParticleLifetimeVariation(float lifetimeVariation) = 0;

    //! Gets the sprite to be used by the particles
    virtual ISprite* GetSprite() = 0;

    //! Sets the sprite to be used by the particles
    virtual void SetSprite(ISprite* sprite) = 0;

    //! Gets the source location of the image to be displayed by the particles
    virtual AZStd::string GetSpritePathname() = 0;

    //! Sets the source location of the image to be displayed by the particles
    virtual void SetSpritePathname(AZStd::string spritePath) = 0;

    //! Gets whether the sprite-sheet cell index changes over time
    virtual bool GetIsSpriteSheetAnimated() = 0;

    //! Sets whether the sprite-sheet cell index changes over time
    virtual void SetIsSpriteSheetAnimated(bool isSpriteSheetAnimated) = 0;

    //! Gets whether the sprite-sheet cell animation is looped
    virtual bool GetIsSpriteSheetAnimationLooped() = 0;

    //! Sets whether the sprite-sheet cell animation is looped
    virtual void SetIsSpriteSheetAnimationLooped(bool isSpriteSheetAnimationLooped) = 0;

    //! Gets whether the sprite-sheet cell (starting) index is random
    virtual bool GetIsSpriteSheetIndexRandom() = 0;

    //! Sets whether the sprite-sheet cell (starting) index is random
    virtual void SetIsSpriteSheetIndexRandom(bool isSpriteSheetIndexRandom) = 0;

    //! Gets the sprite-sheet cell (starting) index
    virtual int GetSpriteSheetCellIndex() = 0;

    //! Sets the sprite-sheet cell (starting) index
    virtual void SetSpriteSheetCellIndex(int spriteSheetIndex) = 0;

    //! Gets the sprite-sheet cell end index
    virtual int GetSpriteSheetCellEndIndex() = 0;

    //! Sets the sprite-sheet cell end index
    virtual void SetSpriteSheetCellEndIndex(int spriteSheetEndIndex) = 0;

    //! Gets the sprite-sheet cell frame delay (in seconds)
    virtual float GetSpriteSheetFrameDelay() = 0;

    //! Sets the sprite-sheet cell frame delay (in seconds)
    virtual void SetSpriteSheetFrameDelay(float spriteSheetFrameDelay) = 0;

    //! Gets whether the aspect ratio of the particles is locked
    virtual bool GetIsParticleAspectRatioLocked() = 0;

    //! Sets whether the aspect ratio of the particles is locked
    virtual void SetIsParticleAspectRatioLocked(bool aspectRatioLocked) = 0;

    //! Gets the particle pivot (where 0,0 is the top left of the particle and 1,1 is the bottom right of the particle)
    virtual AZ::Vector2 GetParticlePivot() = 0;

    //! Sets the particle pivot (where 0,0 is the top left of the particle and 1,1 is the bottom right of the particle)
    virtual void SetParticlePivot(AZ::Vector2 particlePivot) = 0;

    //! Gets the particle size
    virtual AZ::Vector2 GetParticleSize() = 0;

    //! Sets the particle size
    virtual void SetParticleSize(AZ::Vector2 particleSize) = 0;

    //! Gets the particle width
    virtual float GetParticleWidth() = 0;

    //! Sets the particle width
    virtual void SetParticleWidth(float width) = 0;

    //! Gets the particle width variation (a variation of 1 being up to 1 either side of the given width)
    virtual float GetParticleWidthVariation() = 0;

    //! Sets the particle width variation (a variation of 1 being up to 1 either side of the given width)
    virtual void SetParticleWidthVariation(float widthVariation) = 0;

    //! Gets the particle height
    virtual float GetParticleHeight() = 0;

    //! Sets the particle height
    virtual void SetParticleHeight(float height) = 0;

    //! Gets the particle height variation (a variation of 5 being up to 5 either side of the given height)
    virtual float GetParticleHeightVariation() = 0;

    //! Sets the particle height variation (a variation of 5 being up to 5 either side of the given height)
    virtual void SetParticleHeightVariation(float heightVariation) = 0;

    //! Gets the particle movement co-ordinate type
    virtual ParticleCoordinateType GetParticleMovementCoordinateType() = 0;

    //! Sets the particle movement co-ordinate type
    virtual void SetParticleMovementCoordinateType(ParticleCoordinateType particleMovementCoordinateType) = 0;

    //! Gets the particle acceleration co-ordinate type
    virtual ParticleCoordinateType GetParticleAccelerationCoordinateType() = 0;

    //! Sets the particle acceleration co-ordinate type
    virtual void SetParticleAccelerationCoordinateType(ParticleCoordinateType particleAccelerationCoordinateType) = 0;

    //! Gets the particle initial velocity for Polar movement
    virtual AZ::Vector2 GetParticleInitialVelocity() = 0;

    //! Sets the particle initial velocity for Polar movement
    virtual void SetParticleInitialVelocity(AZ::Vector2 initialVelocity) = 0;

    //! Gets the particle initial velocity variation for Polar movement (a variation of (1,2) being up to (1,2) either side of the initial velocity)
    virtual AZ::Vector2 GetParticleInitialVelocityVariation() = 0;

    //! Sets the particle initial velocity variation for Polar movement (a variation of (1,2) being up to (1,2) either side of the initial velocity)
    virtual void SetParticleInitialVelocityVariation(AZ::Vector2 initialVelocityVariation) = 0;

    //! Gets the particle speed for particles with a random initial direction for Cartesian movement
    virtual float GetParticleSpeed() = 0;

    //! Sets the particle speed for particles with a random initial direction for Cartesian movement
    virtual void SetParticleSpeed(float speed) = 0;

    //! Gets the particle speed variation for particles with a random initial direction for Cartesian movement (a variation of 1 being up to 1 either side of the given speed)
    virtual float GetParticleSpeedVariation() = 0;

    //! Sets the particle speed variation for particles with a random initial direction for Cartesian movement (a variation of 1 being up to 1 either side of the given speed)
    virtual void SetParticleSpeedVariation(float speedVariation) = 0;

    //! Gets the particle acceleration
    virtual AZ::Vector2 GetParticleAcceleration() = 0;

    //! Sets the particle acceleration
    virtual void SetParticleAcceleration(AZ::Vector2 acceleration) = 0;

    //! Gets whether the particle rotation is based on the current velocity
    virtual bool GetIsParticleRotationFromVelocity() = 0;

    //! Sets whether the particle rotation is based on the current velocity
    virtual void SetIsParticleRotationFromVelocity(bool rotationFromVelocity) = 0;

    //! Gets whether the particle initial rotation is based on the initial velocity
    virtual bool GetIsParticleInitialRotationFromInitialVelocity() = 0;

    //! Sets whether the particle initial rotation is based on the initial velocity
    virtual void SetIsParticleInitialRotationFromInitialVelocity(bool rotationFromVelocity) = 0;

    //! Gets the particle initial rotation (in degrees)
    virtual float GetParticleInitialRotation() = 0;

    //! Sets the particle initial rotation (in degrees)
    virtual void SetParticleInitialRotation(float initialRotation) = 0;

    //! Gets the particle initial rotation variation (in degrees, a variation of 10 being up to 10 degrees either side of the initial rotation)
    virtual float GetParticleInitialRotationVariation() = 0;

    //! Sets the particle initial rotation variation (in degrees, a variation of 10 being up to 10 degrees either side of the initial rotation)
    virtual void SetParticleInitialRotationVariation(float initialRotationVariation) = 0;

    //! Gets the particle rotation speed (in degrees per second)
    virtual float GetParticleRotationSpeed() = 0;

    //! Sets the particle rotation speed (in degrees per second)
    virtual void SetParticleRotationSpeed(float rotationSpeed) = 0;

    //! Gets the particle rotation speed variation (in degrees, a variation of 5 being up to 5 either side of the rotation speed)
    virtual float GetParticleRotationSpeedVariation() = 0;

    //! Sets the particle rotation speed variation (in degrees, a variation of 5 being up to 5 either side of the rotation speed)
    virtual void SetParticleRotationSpeedVariation(float rotationSpeedVariation) = 0;

    //! Gets the particle color tint
    virtual AZ::Color GetParticleColor() = 0;

    //! Sets the particle color tint
    virtual void SetParticleColor(AZ::Color color) = 0;

    //! Gets the particle color brightness variation (from 0-1, 0 being no variation, 1 being random variation from the original color down to black)
    virtual float GetParticleColorBrightnessVariation() = 0;

    //! Sets the particle color brightness variation (from 0-1, 0 being no variation, 1 being random variation from the original color down to black)
    virtual void SetParticleColorBrightnessVariation(float brightnessVariation) = 0;

    //! Gets the particle color tint variation
    virtual float GetParticleColorTintVariation() = 0;

    //! Sets the particle color tint variation
    virtual void SetParticleColorTintVariation(float tintVariation) = 0;

    //! Gets the particle alpha
    virtual float GetParticleAlpha() = 0;

    //! Sets the particle alpha
    virtual void SetParticleAlpha(float alpha) = 0;
};

using UiParticleEmitterBus = AZ::EBus<UiParticleEmitterInterface>;
