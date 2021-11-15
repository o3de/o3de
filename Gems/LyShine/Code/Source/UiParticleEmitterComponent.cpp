/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "UiParticleEmitterComponent.h"
#include "EditorPropertyTypes.h"
#include "Sprite.h"
#include "RenderGraph.h"

#include <AzCore/Math/Crc.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/std/sort.h>
#include <AzCore/Time/ITime.h>

#include <LyShine/ISprite.h>

#include <LyShine/IDraw2d.h>
#include <LyShine/ISprite.h>
#include <LyShine/Bus/UiCanvasBus.h>
#include <LyShine/Bus/UiTransform2dBus.h>

#include "UiElementComponent.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
// STATIC MEMBER DATA
////////////////////////////////////////////////////////////////////////////////////////////////////

// There are 6 indices per particle and the indices are 16 bit
const AZ::u32 UiParticleEmitterComponent::m_activeParticlesLimit = 65536 / 6;

const float UiParticleEmitterComponent::m_emitRateLimit = m_activeParticlesLimit * 10;

////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
UiParticleEmitterComponent::UiParticleEmitterComponent()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiParticleEmitterComponent::~UiParticleEmitterComponent()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiParticleEmitterComponent::OnCanvasSizeOrScaleChange(AZ::EntityId canvasEntityId)
{
    // Only clear particles if the canvas that resized is the one that this particle component is on.
    AZ::EntityId canvasId;
    EBUS_EVENT_ID_RESULT(canvasId, GetEntityId(), UiElementBus, GetCanvasEntityId);
    if (canvasEntityId == canvasId)
    {
        ClearActiveParticles();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiParticleEmitterComponent::GetIsEmitting()
{
    return m_isEmitting;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiParticleEmitterComponent::SetIsEmitting(bool emitParticles)
{
    if (emitParticles)
    {
        m_nextEmitTime = (m_isHitParticleCountOnActivate ? -m_particleLifetime : 0.0f);
        m_emitterAge = 0.0f;
        m_random.SetSeed(m_isRandomSeedFixed ? m_randomSeed : aznumeric_cast<int64_t>(AZ::GetElapsedTimeMs()));
    }
    m_isEmitting = emitParticles;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiParticleEmitterComponent::GetIsRandomSeedFixed()
{
    return m_isRandomSeedFixed;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiParticleEmitterComponent::SetIsRandomSeedFixed(bool randomSeedFixed)
{
    m_isRandomSeedFixed = randomSeedFixed;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
int UiParticleEmitterComponent::GetRandomSeed()
{
    return m_randomSeed;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiParticleEmitterComponent::SetRandomSeed(int randomSeed)
{
    m_randomSeed = randomSeed;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiParticleEmitterComponent::GetIsParticlePositionRelativeToEmitter()
{
    return m_isPositionRelativeToEmitter;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiParticleEmitterComponent::SetIsParticlePositionRelativeToEmitter(bool relativeToEmitter)
{
    m_isPositionRelativeToEmitter = relativeToEmitter;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiParticleEmitterComponent::GetParticleEmitRate()
{
    return m_emitRate;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiParticleEmitterComponent::SetParticleEmitRate(float particleEmitRate)
{
    m_emitRate = AZ::GetMax<float>(0.01f, particleEmitRate);
    ResetParticleBuffers();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiParticleEmitterComponent::GetIsEmitOnActivate()
{
    return m_isEmitOnActivate;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiParticleEmitterComponent::SetIsEmitOnActivate(bool emitOnActivate)
{
    m_isEmitOnActivate = emitOnActivate;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiParticleEmitterComponent::GetIsHitParticleCountOnActivate()
{
    return m_isHitParticleCountOnActivate;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiParticleEmitterComponent::SetIsHitParticleCountOnActivate(bool hitParticleCountOnActivate)
{
    m_isHitParticleCountOnActivate = hitParticleCountOnActivate;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiParticleEmitterComponent::GetIsEmitterLifetimeInfinite()
{
    return m_isEmitterLifetimeInfinite;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiParticleEmitterComponent::SetIsEmitterLifetimeInfinite(bool emitterLifetimeInfinite)
{
    m_isEmitterLifetimeInfinite = emitterLifetimeInfinite;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiParticleEmitterComponent::GetEmitterLifetime()
{
    return m_emitterLifetime;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiParticleEmitterComponent::SetEmitterLifetime(float emitterLifetime)
{
    m_emitterLifetime = emitterLifetime;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiParticleEmitterComponent::GetIsParticleCountLimited()
{
    return m_isParticleCountLimited;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiParticleEmitterComponent::SetIsParticleCountLimited(bool particleCountLimited)
{
    m_isParticleCountLimited = particleCountLimited;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::u32 UiParticleEmitterComponent::GetMaxParticles()
{
    return m_maxParticles;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiParticleEmitterComponent::SetMaxParticles(AZ::u32 maxParticles)
{
    m_maxParticles = AZ::GetClamp<AZ::u32>(maxParticles, 1, m_activeParticlesLimit);
    ResetParticleBuffers();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiParticleEmitterInterface::EmitShape UiParticleEmitterComponent::GetEmitterShape()
{
    return m_emitShape;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiParticleEmitterComponent::SetEmitterShape(UiParticleEmitterInterface::EmitShape emitterShape)
{
    m_emitShape = emitterShape;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiParticleEmitterComponent::GetIsEmitOnEdge()
{
    return m_isEmitOnEdge;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiParticleEmitterComponent::SetIsEmitOnEdge(bool emitOnEdge)
{
    m_isEmitOnEdge = emitOnEdge;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiParticleEmitterComponent::GetInsideEmitDistance()
{
    return m_insideDistance;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiParticleEmitterComponent::SetInsideEmitDistance(float insideEmitDistance)
{
    m_insideDistance = insideEmitDistance;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiParticleEmitterComponent::GetOutsideEmitDistance()
{
    return m_outsideDistance;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiParticleEmitterComponent::SetOutsideEmitDistance(float outsideEmitDistance)
{
    m_outsideDistance = outsideEmitDistance;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiParticleEmitterInterface::ParticleInitialDirectionType UiParticleEmitterComponent::GetParticleInitialDirectionType()
{
    return m_particleInitialDirectionType;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiParticleEmitterComponent::SetParticleInitialDirectionType(UiParticleEmitterInterface::ParticleInitialDirectionType initialDirectionType)
{
    m_particleInitialDirectionType = initialDirectionType;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiParticleEmitterComponent::GetEmitAngle()
{
    return m_emitAngle;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiParticleEmitterComponent::SetEmitAngle(float emitAngle)
{
    m_emitAngle = emitAngle;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiParticleEmitterComponent::GetEmitAngleVariation()
{
    return m_emitAngleVariation;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiParticleEmitterComponent::SetEmitAngleVariation(float emitAngleVariation)
{
    m_emitAngleVariation = emitAngleVariation;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiParticleEmitterComponent::GetIsParticleLifetimeInfinite()
{
    return m_isParticleLifetimeInfinite;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiParticleEmitterComponent::SetIsParticleLifetimeInfinite(bool infiniteLifetime)
{
    m_isParticleLifetimeInfinite = infiniteLifetime;
    ResetParticleBuffers();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiParticleEmitterComponent::GetParticleLifetime()
{
    return m_particleLifetime;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiParticleEmitterComponent::SetParticleLifetime(float lifetime)
{
    m_particleLifetime = AZ::GetMax<float>(0.01f, lifetime);
    ResetParticleBuffers();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiParticleEmitterComponent::GetParticleLifetimeVariation()
{
    return m_particleLifetimeVariation;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiParticleEmitterComponent::SetParticleLifetimeVariation(float lifetimeVariation)
{
    m_particleLifetimeVariation = lifetimeVariation;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
ISprite* UiParticleEmitterComponent::GetSprite()
{
    return m_sprite;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiParticleEmitterComponent::SetSprite(ISprite* sprite)
{
    if (m_sprite)
    {
        m_sprite->Release();
        m_spritePathname.SetAssetPath("");
    }

    m_sprite = sprite;

    if (m_sprite)
    {
        m_sprite->AddRef();
        m_spritePathname.SetAssetPath(m_sprite->GetPathname().c_str());
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZStd::string UiParticleEmitterComponent::GetSpritePathname()
{
    return m_spritePathname.GetAssetPath();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiParticleEmitterComponent::SetSpritePathname(AZStd::string spritePath)
{
    m_spritePathname.SetAssetPath(spritePath.c_str());

    OnSpritePathnameChange();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiParticleEmitterComponent::GetIsSpriteSheetAnimated()
{
    return m_isSpriteSheetAnimated;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiParticleEmitterComponent::SetIsSpriteSheetAnimated(bool isSpriteSheetAnimated)
{
    m_isSpriteSheetAnimated = isSpriteSheetAnimated;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiParticleEmitterComponent::GetIsSpriteSheetAnimationLooped()
{
    return m_isSpriteSheetAnimationLooped;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiParticleEmitterComponent::SetIsSpriteSheetAnimationLooped(bool isSpriteSheetAnimationLooped)
{
    m_isSpriteSheetAnimationLooped = isSpriteSheetAnimationLooped;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiParticleEmitterComponent::GetIsSpriteSheetIndexRandom()
{
    return m_isSpriteSheetIndexRandom;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiParticleEmitterComponent::SetIsSpriteSheetIndexRandom(bool isSpriteSheetIndexRandom)
{
    m_isSpriteSheetIndexRandom = isSpriteSheetIndexRandom;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
int UiParticleEmitterComponent::GetSpriteSheetCellIndex()
{
    return m_spriteSheetCellIndex;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiParticleEmitterComponent::SetSpriteSheetCellIndex(int spriteSheetIndex)
{
    m_spriteSheetCellIndex = spriteSheetIndex;

    if (m_sprite)
    {
        const AZ::u32 numCells = static_cast<AZ::u32>(m_sprite->GetSpriteSheetCells().size());
        m_spriteSheetCellIndex = AZ::GetMin(numCells, m_spriteSheetCellIndex);
        m_spriteSheetCellEndIndex = AZ::GetMax(m_spriteSheetCellIndex, m_spriteSheetCellEndIndex);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
int UiParticleEmitterComponent::GetSpriteSheetCellEndIndex()
{
    return m_spriteSheetCellEndIndex;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiParticleEmitterComponent::SetSpriteSheetCellEndIndex(int spriteSheetEndIndex)
{
    m_spriteSheetCellEndIndex = spriteSheetEndIndex;

    if (m_sprite)
    {
        const AZ::u32 numCells = static_cast<AZ::u32>(m_sprite->GetSpriteSheetCells().size());
        m_spriteSheetCellEndIndex = AZ::GetMin(numCells, m_spriteSheetCellEndIndex);
        m_spriteSheetCellIndex = AZ::GetMin(m_spriteSheetCellIndex, m_spriteSheetCellEndIndex);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiParticleEmitterComponent::GetSpriteSheetFrameDelay()
{
    return m_spriteSheetFrameDelay;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiParticleEmitterComponent::SetSpriteSheetFrameDelay(float spriteSheetFrameDelay)
{
    m_spriteSheetFrameDelay = spriteSheetFrameDelay;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiParticleEmitterComponent::GetIsParticleAspectRatioLocked()
{
    return m_isParticleAspectRatioLocked;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiParticleEmitterComponent::SetIsParticleAspectRatioLocked(bool lockAspectRatio)
{
    m_isParticleAspectRatioLocked = lockAspectRatio;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Vector2 UiParticleEmitterComponent::GetParticlePivot()
{
    return m_particlePivot;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiParticleEmitterComponent::SetParticlePivot(AZ::Vector2 particlePivot)
{
    m_particlePivot = particlePivot;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Vector2 UiParticleEmitterComponent::GetParticleSize()
{
    return m_particleSize;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiParticleEmitterComponent::SetParticleSize(AZ::Vector2 particleSize)
{
    m_particleSize = particleSize;
    if (m_particleSize.GetY() > 0.0f)
    {
        m_currentAspectRatio = m_particleSize.GetX() / m_particleSize.GetY();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiParticleEmitterComponent::GetParticleWidth()
{
    return m_particleSize.GetX();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiParticleEmitterComponent::SetParticleWidth(float width)
{
    m_particleSize.SetX(AZ::GetMax<float>(width, 0.1f));
    if (m_isParticleAspectRatioLocked && m_currentAspectRatio > 0)
    {
        m_particleSize.SetY(m_particleSize.GetX() / m_currentAspectRatio);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiParticleEmitterComponent::GetParticleWidthVariation()
{
    return m_particleSizeVariation.GetX();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiParticleEmitterComponent::SetParticleWidthVariation(float widthVariation)
{
    m_particleSizeVariation.SetX(widthVariation);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiParticleEmitterComponent::GetParticleHeight()
{
    return m_particleSize.GetY();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiParticleEmitterComponent::SetParticleHeight(float height)
{
    m_particleSize.SetY(AZ::GetMax<float>(height, 0.1f));
    if (m_isParticleAspectRatioLocked)
    {
        m_particleSize.SetX(m_particleSize.GetY() * m_currentAspectRatio);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiParticleEmitterComponent::GetParticleHeightVariation()
{
    return m_particleSizeVariation.GetY();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiParticleEmitterComponent::SetParticleHeightVariation(float heightVariation)
{
    m_particleSizeVariation.SetY(heightVariation);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiParticleEmitterInterface::ParticleCoordinateType UiParticleEmitterComponent::GetParticleMovementCoordinateType()
{
    return m_particleMovementCoordinateType;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiParticleEmitterComponent::SetParticleMovementCoordinateType(UiParticleEmitterInterface::ParticleCoordinateType particleMovementCoordinateType)
{
    m_particleMovementCoordinateType = particleMovementCoordinateType;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiParticleEmitterInterface::ParticleCoordinateType UiParticleEmitterComponent::GetParticleAccelerationCoordinateType()
{
    return m_particleAccelerationCoordinateType;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiParticleEmitterComponent::SetParticleAccelerationCoordinateType(UiParticleEmitterInterface::ParticleCoordinateType particleAccelerationCoordinateType)
{
    m_particleAccelerationCoordinateType = particleAccelerationCoordinateType;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Vector2 UiParticleEmitterComponent::GetParticleInitialVelocity()
{
    return m_particleInitialVelocity;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiParticleEmitterComponent::SetParticleInitialVelocity(AZ::Vector2 initialVelocity)
{
    m_particleInitialVelocity = initialVelocity;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Vector2 UiParticleEmitterComponent::GetParticleInitialVelocityVariation()
{
    return m_particleInitialVelocityVariation;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiParticleEmitterComponent::SetParticleInitialVelocityVariation(AZ::Vector2 initialVelocityVariation)
{
    m_particleInitialVelocityVariation = initialVelocityVariation;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiParticleEmitterComponent::GetParticleSpeed()
{
    return m_particleSpeed;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiParticleEmitterComponent::SetParticleSpeed(float speed)
{
    m_particleSpeed = speed;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiParticleEmitterComponent::GetParticleSpeedVariation()
{
    return m_particleSpeedVariation;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiParticleEmitterComponent::SetParticleSpeedVariation(float speedVariation)
{
    m_particleSpeedVariation = speedVariation;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Vector2 UiParticleEmitterComponent::GetParticleAcceleration()
{
    return m_particleAcceleration;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiParticleEmitterComponent::SetParticleAcceleration(AZ::Vector2 acceleration)
{
    m_particleAcceleration = acceleration;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiParticleEmitterComponent::GetIsParticleRotationFromVelocity()
{
    return m_isParticleRotationFromVelocity;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiParticleEmitterComponent::SetIsParticleRotationFromVelocity(bool rotationFromVelocity)
{
    m_isParticleRotationFromVelocity = rotationFromVelocity;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiParticleEmitterComponent::GetIsParticleInitialRotationFromInitialVelocity()
{
    return m_isParticleInitialRotationFromInitialVelocity;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiParticleEmitterComponent::SetIsParticleInitialRotationFromInitialVelocity(bool rotationFromVelocity)
{
    m_isParticleInitialRotationFromInitialVelocity = rotationFromVelocity;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiParticleEmitterComponent::GetParticleInitialRotation()
{
    return m_particleInitialRotation;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiParticleEmitterComponent::SetParticleInitialRotation(float initialRotation)
{
    m_particleInitialRotation = initialRotation;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiParticleEmitterComponent::GetParticleInitialRotationVariation()
{
    return m_particleInitialRotationVariation;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiParticleEmitterComponent::SetParticleInitialRotationVariation(float initialRotationVariation)
{
    m_particleInitialRotationVariation = initialRotationVariation;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiParticleEmitterComponent::GetParticleRotationSpeed()
{
    return m_particleRotationSpeed;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiParticleEmitterComponent::SetParticleRotationSpeed(float rotationSpeed)
{
    m_particleRotationSpeed = rotationSpeed;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiParticleEmitterComponent::GetParticleRotationSpeedVariation()
{
    return m_particleRotationSpeedVariation;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiParticleEmitterComponent::SetParticleRotationSpeedVariation(float rotationSpeedVariation)
{
    m_particleRotationSpeedVariation = rotationSpeedVariation;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Color UiParticleEmitterComponent::GetParticleColor()
{
    return m_particleColor;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiParticleEmitterComponent::SetParticleColor(AZ::Color color)
{
    m_particleColor = color;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiParticleEmitterComponent::GetParticleColorBrightnessVariation()
{
    return m_particleColorBrightnessVariation;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiParticleEmitterComponent::SetParticleColorBrightnessVariation(float brightnessVariation)
{
    m_particleColorBrightnessVariation = AZ::GetClamp<float>(brightnessVariation, 0.0f, 1.0f);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiParticleEmitterComponent::GetParticleColorTintVariation()
{
    return m_particleColorTintVariation;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiParticleEmitterComponent::SetParticleColorTintVariation(float tintVariation)
{
    m_particleColorTintVariation = AZ::GetClamp<float>(tintVariation, 0.0f, 1.0f);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiParticleEmitterComponent::GetParticleAlpha()
{
    return m_particleAlpha;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiParticleEmitterComponent::SetParticleAlpha(float alpha)
{
    m_particleAlpha = AZ::GetClamp<float>(alpha, 0.0f, 1.0f);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiParticleEmitterComponent::InGamePostActivate()
{
    if (m_isEmitOnActivate)
    {
        SetIsEmitting(true);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiParticleEmitterComponent::Render(LyShine::IRenderGraph* renderGraph)
{
    AZ::u32 particlesToRender = AZ::GetMin<AZ::u32>(static_cast<AZ::u32>(m_particleContainer.size()), m_particleBufferSize);
    if (particlesToRender == 0)
    {
        return;
    }

    AZ::Matrix4x4 transform = AZ::Matrix4x4::CreateIdentity();

    AZ::Vector2 emitterOffset = AZ::Vector2::CreateZero();
    if (m_isPositionRelativeToEmitter)
    {
        UiTransformInterface::RectPoints points;
        EBUS_EVENT_ID(GetEntityId(), UiTransformBus, GetCanvasSpacePointsNoScaleRotate, points);
        emitterOffset = (points.TopLeft() + points.BottomRight()) * 0.5f;
        EBUS_EVENT_ID(GetEntityId(), UiTransformBus, GetTransformToViewport, transform);
    }
    else
    {
        AZ::EntityId canvasID;
        EBUS_EVENT_ID_RESULT(canvasID, GetEntityId(), UiElementBus, GetCanvasEntityId);
        EBUS_EVENT_ID_RESULT(transform, canvasID, UiCanvasBus, GetCanvasToViewportMatrix);
    }

    AZ::Data::Instance<AZ::RPI::Image> image;
    if (m_sprite)
    {
        CSprite* sprite = static_cast<CSprite*>(m_sprite); // LYSHINE_ATOM_TODO - find a different solution from downcasting - GHI #3570
        if (sprite)
        {
            image = sprite->GetImage();
        }
    }

    bool isClampTextureMode = true;
    bool isTextureSRGB = false;
    bool isTexturePremultipliedAlpha = false;

    UiParticle::UiParticleRenderParameters renderParameters;
    renderParameters.isVelocityCartesian = IsMovementCoordinateTypeCartesian();
    renderParameters.particleOffset = &emitterOffset;
    renderParameters.sprite = m_sprite;
    renderParameters.isRelativeToEmitter = m_isPositionRelativeToEmitter;
    renderParameters.isParticleInfinite = m_isParticleLifetimeInfinite;
    renderParameters.isAspectRatioLocked = m_isParticleAspectRatioLocked;
    renderParameters.isRotationVelocityBased = m_isParticleRotationFromVelocity;
    renderParameters.isColorOverrideUsed = m_isColorOverridden;
    renderParameters.isAlphaOverrideUsed = m_isAlphaOverridden;
    renderParameters.colorOverride = m_overrideColor;
    renderParameters.alphaOverride = m_overrideAlpha;
    renderParameters.alphaFadeMultiplier = renderGraph->GetAlphaFade();
    renderParameters.isWidthMultiplierUsed = (m_particleWidthMultiplier.size() > 0);
    renderParameters.isHeightMultiplierUsed = (m_particleHeightMultiplier.size() > 0);
    renderParameters.isColorMultiplierUsed = (m_particleColorMultiplier.size() > 0);
    renderParameters.isAlphaMultiplierUsed = (m_particleAlphaMultiplier.size() > 0);
    renderParameters.sizeWidthMultiplier = &m_particleWidthMultiplierCurve;
    renderParameters.sizeHeightMultiplier = &m_particleHeightMultiplierCurve;
    renderParameters.colorMultiplier = &m_particleColorMultiplierCurve;
    renderParameters.alphaMultiplier = &m_particleAlphaMultiplierCurve;
    renderParameters.spritesheetStartFrame = m_spriteSheetCellIndex;
    renderParameters.spritesheetFrameRange = m_spriteSheetCellEndIndex - m_spriteSheetCellIndex;
    renderParameters.spritesheetFrameDelay = m_spriteSheetFrameDelay;
    renderParameters.spritesheetCellIndexAnimated = m_isSpriteSheetAnimated;
    renderParameters.spritesheetCellIndexAnimationLooped = m_isSpriteSheetAnimationLooped;

    const int verticesPerParticle = 4;
    const int indicesPerParticle = 6;

    AZ::u32 totalParticlesInserted = 0;
    AZ::u32 totalVerticesInserted = 0;

    // particlesToRender is the max particles we will render, we could render less if some have zero alpha
    for (AZ::u32 i = 0; i < particlesToRender; ++i)
    {
        SVF_P2F_C4B_T2F_F4B* firstVertexOfParticle = &m_cachedPrimitive.m_vertices[totalVerticesInserted];

        if (m_particleContainer[i].FillVertices(firstVertexOfParticle, renderParameters, transform))
        {
            totalParticlesInserted++;
            totalVerticesInserted += verticesPerParticle;
        }
    }

    m_cachedPrimitive.m_numVertices = totalVerticesInserted;
    m_cachedPrimitive.m_numIndices = totalParticlesInserted * indicesPerParticle;
    LyShine::RenderGraph* lyRenderGraph = static_cast<LyShine::RenderGraph*>(renderGraph); // LYSHINE_ATOM_TODO - find a different solution from downcasting - GHI #3570
    if (lyRenderGraph)
    {
        lyRenderGraph->AddPrimitiveAtom(&m_cachedPrimitive, image, isClampTextureMode, isTextureSRGB, isTexturePremultipliedAlpha, m_blendMode);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiParticleEmitterComponent::Update(float deltaTime)
{
    bool particlesExistedBeforeUpdate = m_particleContainer.size() > 0;

    // Update existing particles
    UiParticle::UiParticleUpdateParameters particleUpdateParameters;
    particleUpdateParameters.isVelocityCartesian = IsMovementCoordinateTypeCartesian();
    particleUpdateParameters.isAccelerationCartesian = (m_particleAccelerationCoordinateType == ParticleCoordinateType::Cartesian);
    particleUpdateParameters.isSpeedMultiplierUsed = (m_particleSpeedMultiplier.size() > 0);
    particleUpdateParameters.speedMultiplier = &m_particleSpeedMultiplierCurve;
    particleUpdateParameters.isParticleInfinite = m_isParticleLifetimeInfinite;
    for (int currentParticleIndex = 0; currentParticleIndex < m_particleContainer.size(); )
    {
        m_particleContainer[currentParticleIndex].Update(deltaTime, particleUpdateParameters);
        if (!m_particleContainer[currentParticleIndex].IsActive(m_isParticleLifetimeInfinite))
        {
            // Move the last active particle to the current position and pop_back to avoid vector shifting all
            // following elements.
            m_particleContainer[currentParticleIndex] = m_particleContainer.back();
            m_particleContainer.pop_back();
        }
        else
        {
            currentParticleIndex++;
        }
    }

    if (m_isEmitting)
    {
        m_emitterAge += deltaTime;
        if (!m_isEmitterLifetimeInfinite && m_emitterAge > m_emitterLifetime)
        {
            SetIsEmitting(false);
            m_emitterAge = m_emitterLifetime;
        }

        bool isTimeToEmit = (m_nextEmitTime <= m_emitterAge);

        AZ::u32 currentMaxParticles = m_isParticleCountLimited ? m_maxParticles : m_activeParticlesLimit;

        // Emit new particles
        if (isTimeToEmit && m_particleContainer.size() < currentMaxParticles)
        {
            AZ::Vector2 emitterPosition = AZ::Vector2::CreateZero();
            UiTransformInterface::RectPoints points;
            EBUS_EVENT_ID(GetEntityId(), UiTransformBus, GetCanvasSpacePointsNoScaleRotate, points);
            emitterPosition = (points.TopLeft() + points.BottomRight()) * 0.5f;

            if (m_nextEmitTime + m_particleLifetime + m_particleLifetimeVariation < m_emitterAge)
            {
                // if we have a large delta, don't emit particles that would have already gone through their full lifetime
                m_nextEmitTime = m_emitterAge - (m_particleLifetime + m_particleLifetimeVariation);
            }

            AZ::Matrix4x4 transform = AZ::Matrix4x4::CreateIdentity();
            AZ::Vector2 scale = AZ::Vector2::CreateOne();

            if (!m_isPositionRelativeToEmitter)
            {
                EBUS_EVENT_ID(GetEntityId(), UiTransformBus, GetTransformToCanvasSpace, transform);
                AZ::Vector3 transformScale = transform.RetrieveScale();
                scale.Set(transformScale.GetX(), transformScale.GetY());
            }

            UiParticle newParticle;
            UiParticle::UiParticleInitialParameters particleParameters;
            AZ::Vector3 emitterPositionV3(emitterPosition.GetX(), emitterPosition.GetY(), 1.0f);
            emitterPositionV3 = transform * emitterPositionV3;
            particleParameters.initialEmitterOffset = AZ::Vector2(emitterPositionV3.GetX(), emitterPositionV3.GetY());
            particleParameters.acceleration = m_particleAcceleration * scale;
            particleParameters.pivot = m_particlePivot;
            float emitRate = 1.0f / m_emitRate;
            while (isTimeToEmit && m_particleContainer.size() < currentMaxParticles)
            {
                AZ::Vector3 positionV3 = GetRandomParticlePosition();
                positionV3 = transform * positionV3;
                particleParameters.position = AZ::Vector2(positionV3.GetX(), positionV3.GetY());
                particleParameters.initialVelocity = GetRandomParticleVelocity(particleParameters.position, emitterPosition) * scale;
                particleParameters.rotation = GetRandomParticleRotation(particleParameters.initialVelocity, particleParameters.position);

                particleParameters.angularVelocity = AZ::DegToRad(m_particleRotationSpeed) + AZ::DegToRad(m_particleRotationSpeedVariation) * (2.0f * m_random.GetRandomFloat() - 1.0f);

                particleParameters.lifetime = AZ::GetMax<float>(0.01f, m_particleLifetime + m_particleLifetimeVariation * (2.0f * m_random.GetRandomFloat() - 1.0f));

                float sizeYVariation = m_particleSizeVariation.GetY() * (2.0f * m_random.GetRandomFloat() - 1.0f);
                float sizeX = m_particleSize.GetX() + (m_isParticleAspectRatioLocked ? sizeYVariation * m_currentAspectRatio : m_particleSizeVariation.GetX() * (2.0f * m_random.GetRandomFloat() - 1.0f));
                float sizeY = m_particleSize.GetY() + sizeYVariation;
                particleParameters.size.Set(sizeX, sizeY);
                particleParameters.size *= scale;

                float particleColorRed = AZ::GetClamp<float>(m_particleColor.GetR() + m_particleColorTintVariation * (2.0f * m_random.GetRandomFloat() - 1.0f), 0.0f, 1.0f);
                float particleColorGreen = AZ::GetClamp<float>(m_particleColor.GetG() + m_particleColorTintVariation * (2.0f * m_random.GetRandomFloat() - 1.0f), 0.0f, 1.0f);
                float particleColorBlue = AZ::GetClamp<float>(m_particleColor.GetB() + m_particleColorTintVariation * (2.0f * m_random.GetRandomFloat() - 1.0f), 0.0f, 1.0f);
                particleParameters.color.Set(particleColorRed, particleColorGreen, particleColorBlue, 1.0f);
                particleParameters.color = particleParameters.color * (1 - m_particleColorBrightnessVariation * m_random.GetRandomFloat());
                particleParameters.color.SetA(m_particleAlpha);

                particleParameters.spriteSheetCellIndex = m_spriteSheetCellIndex + (m_isSpriteSheetIndexRandom ? static_cast<AZ::u32>((m_spriteSheetCellEndIndex - m_spriteSheetCellIndex) * m_random.GetRandomFloat()) : 0);

                newParticle.Init(&particleParameters);
                newParticle.Update(m_emitterAge - m_nextEmitTime, particleUpdateParameters);

                m_particleContainer.push_back(newParticle);

                m_nextEmitTime += emitRate;
                isTimeToEmit = (m_nextEmitTime <= m_emitterAge);
            }
        }

        if (isTimeToEmit)
        {
            m_nextEmitTime = m_emitterAge;
        }
    }

    // Currently we mark the render graph dirty whenever a particle emitter is updated and has any
    // active particles.
    // A future optimization could be that we only mark it dirty if new particles were emitted or 
    // particles were removed. At other times we could update the vertices in m_cachedPrimitive
    // without regenerating the graph. This would require some way to register to get the vertices updated
    // during the canvas render in the case when the render graph was not being regenerated.
    bool particlesExistAfterUpdate = m_particleContainer.size() > 0;
    if (particlesExistedBeforeUpdate || particlesExistAfterUpdate)
    {
        MarkRenderGraphDirty();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiParticleEmitterComponent::OnUiElementFixup(AZ::EntityId canvasEntityId, AZ::EntityId /*parentEntityId*/)
{
    bool isElementEnabled = false;
    EBUS_EVENT_ID_RESULT(isElementEnabled, GetEntityId(), UiElementBus, GetAreElementAndAncestorsEnabled);
    if (isElementEnabled)
    {
        UiCanvasUpdateNotificationBus::Handler::BusConnect(canvasEntityId);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiParticleEmitterComponent::OnUiElementAndAncestorsEnabledChanged(bool areElementAndAncestorsEnabled)
{
    if (areElementAndAncestorsEnabled)
    {
        AZ::EntityId canvasEntityId;
        EBUS_EVENT_ID_RESULT(canvasEntityId, GetEntityId(), UiElementBus, GetCanvasEntityId);
        if (canvasEntityId.IsValid())
        {
            UiCanvasUpdateNotificationBus::Handler::BusConnect(canvasEntityId);
        }
    }
    else
    {
        UiCanvasUpdateNotificationBus::Handler::BusDisconnect();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiParticleEmitterComponent::ResetOverrides()
{
    m_isColorOverridden = false;
    m_isAlphaOverridden = false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiParticleEmitterComponent::SetOverrideColor(const AZ::Color& color)
{
    m_overrideColor = color;
    m_isColorOverridden = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiParticleEmitterComponent::SetOverrideAlpha(float alpha)
{
    m_overrideAlpha = alpha;
    m_isAlphaOverridden = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC STATIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

void UiParticleEmitterComponent::Reflect(AZ::ReflectContext* context)
{
    AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

    if (serializeContext)
    {
        serializeContext->Class<UiParticleEmitterComponent, AZ::Component>()
            ->Version(1)
            ->Field("EmitOnActivate", &UiParticleEmitterComponent::m_isEmitOnActivate)
            ->Field("HitParticleCountOnActivate", &UiParticleEmitterComponent::m_isHitParticleCountOnActivate)
            ->Field("IsEmitterLifetimeInfinite", &UiParticleEmitterComponent::m_isEmitterLifetimeInfinite)
            ->Field("EmitterLifetime", &UiParticleEmitterComponent::m_emitterLifetime)
            ->Field("EmitRate", &UiParticleEmitterComponent::m_emitRate)
            ->Field("EmitShape", &UiParticleEmitterComponent::m_emitShape)
            ->Field("IsParticleCountLimited", &UiParticleEmitterComponent::m_isParticleCountLimited)
            ->Field("MaxParticles", &UiParticleEmitterComponent::m_maxParticles)
            ->Field("IsRandomSeedFixed", &UiParticleEmitterComponent::m_isRandomSeedFixed)
            ->Field("RandomSeed", &UiParticleEmitterComponent::m_randomSeed)
            ->Field("IsEmitOnEdge", &UiParticleEmitterComponent::m_isEmitOnEdge)
            ->Field("ParticleInitialDirectionType", &UiParticleEmitterComponent::m_particleInitialDirectionType)
            ->Field("EmitInsideDistance", &UiParticleEmitterComponent::m_insideDistance)
            ->Field("EmitOutsideDistance", &UiParticleEmitterComponent::m_outsideDistance)
            ->Field("EmitAngle", &UiParticleEmitterComponent::m_emitAngle)
            ->Field("EmitAngleVariation", &UiParticleEmitterComponent::m_emitAngleVariation)
            ->Field("IsParticleLifetimeInfinite", &UiParticleEmitterComponent::m_isParticleLifetimeInfinite)
            ->Field("ParticleLife", &UiParticleEmitterComponent::m_particleLifetime)
            ->Field("ParticleLifeVariation", &UiParticleEmitterComponent::m_particleLifetimeVariation)
            ->Field("SpritePathname", &UiParticleEmitterComponent::m_spritePathname)
            ->Field("IsSpriteSheetAnimated", &UiParticleEmitterComponent::m_isSpriteSheetAnimated)
            ->Field("IsSpriteSheetAnimationLooped", &UiParticleEmitterComponent::m_isSpriteSheetAnimationLooped)
            ->Field("IsSpriteSheetIndexRandom", &UiParticleEmitterComponent::m_isSpriteSheetIndexRandom)
            ->Field("SpriteSheetIndex", &UiParticleEmitterComponent::m_spriteSheetCellIndex)
            ->Field("SpriteSheetEndIndex", &UiParticleEmitterComponent::m_spriteSheetCellEndIndex)
            ->Field("SpriteSheetFrameDelay", &UiParticleEmitterComponent::m_spriteSheetFrameDelay)
            ->Field("BlendMode", &UiParticleEmitterComponent::m_blendMode)
            ->Field("IsPositionRelativeToEmitter", &UiParticleEmitterComponent::m_isPositionRelativeToEmitter)
            ->Field("ParticleMovementCoordinateType", &UiParticleEmitterComponent::m_particleMovementCoordinateType)
            ->Field("ParticleInitialVelocity", &UiParticleEmitterComponent::m_particleInitialVelocity)
            ->Field("ParticleInitialVelocityVariation", &UiParticleEmitterComponent::m_particleInitialVelocityVariation)
            ->Field("ParticleSpeed", &UiParticleEmitterComponent::m_particleSpeed)
            ->Field("ParticleSpeedVariation", &UiParticleEmitterComponent::m_particleSpeedVariation)
            ->Field("ParticleAccelerationCoordinateType", &UiParticleEmitterComponent::m_particleAccelerationCoordinateType)
            ->Field("ParticleAcceleration", &UiParticleEmitterComponent::m_particleAcceleration)
            ->Field("IsParticleRotationFromVelocity", &UiParticleEmitterComponent::m_isParticleRotationFromVelocity)
            ->Field("IsParticleInitialRotationFromInitialVelocity", &UiParticleEmitterComponent::m_isParticleInitialRotationFromInitialVelocity)
            ->Field("ParticleInitialRotation", &UiParticleEmitterComponent::m_particleInitialRotation)
            ->Field("ParticleInitialRotationVariation", &UiParticleEmitterComponent::m_particleInitialRotationVariation)
            ->Field("ParticleRotation", &UiParticleEmitterComponent::m_particleRotationSpeed)
            ->Field("ParticleRotationVariation", &UiParticleEmitterComponent::m_particleRotationSpeedVariation)
            ->Field("ParticleAspectRatioLocked", &UiParticleEmitterComponent::m_isParticleAspectRatioLocked)
            ->Field("ParticlePivot", &UiParticleEmitterComponent::m_particlePivot)
            ->Field("ParticleSize", &UiParticleEmitterComponent::m_particleSize)
            ->Field("ParticleSizeVariation", &UiParticleEmitterComponent::m_particleSizeVariation)
            ->Field("ParticleColor", &UiParticleEmitterComponent::m_particleColor)
            ->Field("ParticleColorBrightnessVariation", &UiParticleEmitterComponent::m_particleColorBrightnessVariation)
            ->Field("ParticleColorTintVariation", &UiParticleEmitterComponent::m_particleColorTintVariation)
            ->Field("ParticleAlpha", &UiParticleEmitterComponent::m_particleAlpha)
            ->Field("ParticleSpeedMultiplier", &UiParticleEmitterComponent::m_particleSpeedMultiplier)
            ->Field("ParticleWidthMultiplier", &UiParticleEmitterComponent::m_particleWidthMultiplier)
            ->Field("ParticleHeightMultiplier", &UiParticleEmitterComponent::m_particleHeightMultiplier)
            ->Field("ParticleColorMultiplier", &UiParticleEmitterComponent::m_particleColorMultiplier)
            ->Field("ParticleAlphaMultiplier", &UiParticleEmitterComponent::m_particleAlphaMultiplier);

        AZ::EditContext* ec = serializeContext->GetEditContext();
        if (ec)
        {
            auto editInfo = ec->Class<UiParticleEmitterComponent>("ParticleEmitter", "A visual component that emits 2D particles.");

            editInfo->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::Category, "UI")
                ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("UI", 0x27ff46b0))
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

            {
                editInfo->ClassElement(AZ::Edit::ClassElements::Group, "Emitter Settings")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true);
                editInfo->DataElement(AZ::Edit::UIHandlers::CheckBox, &UiParticleEmitterComponent::m_isEmitOnActivate, "Emit on activate", "Emitter starts emitting on activate.");
                editInfo->DataElement(AZ::Edit::UIHandlers::CheckBox, &UiParticleEmitterComponent::m_isHitParticleCountOnActivate, "Hit particle count on activate", "Emitter hits the particle count when it starts emitting.");
                editInfo->DataElement(AZ::Edit::UIHandlers::CheckBox, &UiParticleEmitterComponent::m_isEmitterLifetimeInfinite, "Infinite life time", "The life time of the emitter is infinite")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC("RefreshEntireTree", 0xefbc823c));
                editInfo->DataElement("EmitterLifetime", &UiParticleEmitterComponent::m_emitterLifetime, "Emitter lifetime", "The amount of time (in seconds) that the emitter emits.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &UiParticleEmitterComponent::IsEmitterLifetimeFinite)
                    ->Attribute(AZ::Edit::Attributes::Min, 0.01f);
                editInfo->DataElement("EmitRate", &UiParticleEmitterComponent::m_emitRate, "Emit rate", "The amount of particles emitted per second.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.01f)
                    ->Attribute(AZ::Edit::Attributes::Max, m_emitRateLimit)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiParticleEmitterComponent::ResetParticleBuffers);
                editInfo->DataElement(AZ::Edit::UIHandlers::CheckBox, &UiParticleEmitterComponent::m_isParticleCountLimited, "Particle count limit", "Sets whether there is a limit on the amount of active particles.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &UiParticleEmitterComponent::IsParticleLimitToggleable)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC("RefreshEntireTree", 0xefbc823c));
                editInfo->DataElement("MaxParticles", &UiParticleEmitterComponent::m_maxParticles, "Active particles limit", "The maximum amount of particles that can be active.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &UiParticleEmitterComponent::IsParticleLimitRequired)
                    ->Attribute(AZ::Edit::Attributes::Min, 1)
                    ->Attribute(AZ::Edit::Attributes::Max, m_activeParticlesLimit)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC("RefreshAttributesAndValues", 0xcbc2147c))
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiParticleEmitterComponent::ResetParticleBuffers);
                editInfo->DataElement(AZ::Edit::UIHandlers::CheckBox, &UiParticleEmitterComponent::m_isRandomSeedFixed, "Fixed random seed", "Sets whether the random seed for this emitter is fixed. If unchecked, a random seed will be automatically generated each time the emitter starts emitting.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC("RefreshEntireTree", 0xefbc823c));
                editInfo->DataElement("RandomSeed", &UiParticleEmitterComponent::m_randomSeed, "Random seed", "The seed to use for the particle emitter.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &UiParticleEmitterComponent::m_isRandomSeedFixed);
                editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiParticleEmitterComponent::m_emitShape, "Emitter shape", "The shape of the emitter.")
                    ->EnumAttribute(UiParticleEmitterInterface::EmitShape::Point, "Point")
                    ->EnumAttribute(UiParticleEmitterInterface::EmitShape::Circle, "Circle")
                    ->EnumAttribute(UiParticleEmitterInterface::EmitShape::Quad, "Quad")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC("RefreshEntireTree", 0xefbc823c));
                editInfo->DataElement("IsEmitOnEdge", &UiParticleEmitterComponent::m_isEmitOnEdge, "Emit on edge", "The particles are emitted from the edge of the emitter shape.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &UiParticleEmitterComponent::IsShapeWithEdge)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC("RefreshEntireTree", 0xefbc823c));
                editInfo->DataElement("EmitInsideDistance", &UiParticleEmitterComponent::m_insideDistance, "Emit inside distance", "The distance inside the emitter shape edge that particles should be emitted.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &UiParticleEmitterComponent::IsEmittingFromEdge);
                editInfo->DataElement("EmitOutsideDistance", &UiParticleEmitterComponent::m_outsideDistance, "Emit outside distance", "The distance outside the emitter shape edge that particles should be emitted.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &UiParticleEmitterComponent::IsEmittingFromEdge);
                editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiParticleEmitterComponent::m_particleInitialDirectionType, "Initial direction type", "Sets how the initial direction is calculated.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &UiParticleEmitterComponent::CanEmitFromCenter)
                    ->EnumAttribute(UiParticleEmitterInterface::ParticleInitialDirectionType::RelativeToEmitAngle, "Relative to emit angle")
                    ->EnumAttribute(UiParticleEmitterInterface::ParticleInitialDirectionType::RelativeToEmitterCenter, "Relative to emitter center")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC("RefreshEntireTree", 0xefbc823c));
                editInfo->DataElement("EmitAngle", &UiParticleEmitterComponent::m_emitAngle, "Emit angle", "The angle to emit particles, measured clockwise in degrees from straight up.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &UiParticleEmitterComponent::IsEmitAngleRequired);
                editInfo->DataElement(AZ::Edit::UIHandlers::Slider, &UiParticleEmitterComponent::m_emitAngleVariation, "Emit angle variation", "The spread of particles emitted about the emit angle in degrees.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &UiParticleEmitterComponent::IsMovementCoordinateTypeCartesian)
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 180.0f);
            }

            {
                editInfo->ClassElement(AZ::Edit::ClassElements::Group, "Particle Settings")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true);
                editInfo->DataElement("IsParticleLifetimeInfinite", &UiParticleEmitterComponent::m_isParticleLifetimeInfinite, "Infinite life time", "The life time of the emitted particles is infinite.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiParticleEmitterComponent::CheckMaxParticleValidity)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC("RefreshEntireTree", 0xefbc823c));
                editInfo->DataElement("ParticleLife", &UiParticleEmitterComponent::m_particleLifetime, "Life time", "The life time of the emitted particles in seconds.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &UiParticleEmitterComponent::IsParticleLifetimeFinite)
                    ->Attribute(AZ::Edit::Attributes::Min, 0.01f)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiParticleEmitterComponent::ResetParticleBuffers);
                editInfo->DataElement("ParticleLifeVariation", &UiParticleEmitterComponent::m_particleLifetimeVariation, "Life time variation", "The random variation of the life time of the emitted particles.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &UiParticleEmitterComponent::IsParticleLifetimeFinite)
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f);
                editInfo->DataElement("Sprite", &UiParticleEmitterComponent::m_spritePathname, "Sprite pathname", "The sprite path.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiParticleEmitterComponent::OnSpritePathnameChange)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC("RefreshEntireTree", 0xefbc823c));
                editInfo->DataElement(AZ::Edit::UIHandlers::CheckBox, &UiParticleEmitterComponent::m_isSpriteSheetAnimated, "Animated sprite sheet", "The sprite sheet cell index is animated over time.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &UiParticleEmitterComponent::IsSpriteTypeSpriteSheet)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC("RefreshEntireTree", 0xefbc823c));
                editInfo->DataElement(AZ::Edit::UIHandlers::CheckBox, &UiParticleEmitterComponent::m_isSpriteSheetAnimationLooped, "Loop sprite sheet animation", "The sprite sheet animation is looped.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &UiParticleEmitterComponent::m_isSpriteSheetAnimated);
                editInfo->DataElement(AZ::Edit::UIHandlers::CheckBox, &UiParticleEmitterComponent::m_isSpriteSheetIndexRandom, "Random sprite sheet index", "The sprite sheet cell index is randomly chosen from the given range.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &UiParticleEmitterComponent::IsSpriteTypeSpriteSheet)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC("RefreshEntireTree", 0xefbc823c));
                editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiParticleEmitterComponent::m_spriteSheetCellIndex, "Sprite sheet Index", "Sprite sheet index. Defines which cell in a sprite sheet is displayed.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &UiParticleEmitterComponent::IsSpriteTypeSpriteSheet)
                    ->Attribute(AZ::Edit::Attributes::NameLabelOverride, &UiParticleEmitterComponent::GetSpriteSheetIndexPropertyLabel)
                    ->Attribute("EnumValues", &UiParticleEmitterComponent::PopulateSpriteSheetIndexStringList)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiParticleEmitterComponent::OnSpriteSheetCellIndexChanged)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC("RefreshAttributesAndValues", 0xcbc2147c));
                editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiParticleEmitterComponent::m_spriteSheetCellEndIndex, "Sprite sheet end frame", "Defines which cell in a sprite sheet is displayed.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &UiParticleEmitterComponent::IsSpriteSheetCellRangeRequired)
                    ->Attribute("EnumValues", &UiParticleEmitterComponent::PopulateSpriteSheetIndexStringList)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiParticleEmitterComponent::OnSpriteSheetCellEndIndexChanged)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC("RefreshAttributesAndValues", 0xcbc2147c));
                editInfo->DataElement("SpriteSheetFrameDelay", &UiParticleEmitterComponent::m_spriteSheetFrameDelay, "Sprite sheet frame delay", "Number of seconds to delay until the next frame.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &UiParticleEmitterComponent::m_isSpriteSheetAnimated)
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f);
                editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiParticleEmitterComponent::m_blendMode, "Blend mode", "The blend mode used to draw the particles")
                    ->EnumAttribute(LyShine::BlendMode::Normal, "Normal")
                    ->EnumAttribute(LyShine::BlendMode::Add, "Add")
                    ->EnumAttribute(LyShine::BlendMode::Screen, "Screen")
                    ->EnumAttribute(LyShine::BlendMode::Darken, "Darken")
                    ->EnumAttribute(LyShine::BlendMode::Lighten, "Lighten");
            }

            {
                editInfo->ClassElement(AZ::Edit::ClassElements::Group, "Particle Movement")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true);
                editInfo->DataElement("IsPositionRelativeToEmitter", &UiParticleEmitterComponent::m_isPositionRelativeToEmitter, "Relative to emitter", "The particles move relative to the emitter.");
                editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiParticleEmitterComponent::m_particleMovementCoordinateType, "Movement co-ordinate type", "The co-ordinate system to use for particle movement.")
                    ->EnumAttribute(UiParticleEmitterInterface::ParticleCoordinateType::Cartesian, "Cartesian")
                    ->EnumAttribute(UiParticleEmitterInterface::ParticleCoordinateType::Polar, "Polar")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC("RefreshEntireTree", 0xefbc823c));
                editInfo->DataElement("ParticleInitialVelocity", &UiParticleEmitterComponent::m_particleInitialVelocity, "Initial velocity", "The initial velocity of the emitted particles.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &UiParticleEmitterComponent::IsMovementCoordinateTypePolar);
                editInfo->DataElement("ParticleInitialVelocityVariation", &UiParticleEmitterComponent::m_particleInitialVelocityVariation, "Initial Velocity Variation", "The random variation in the initial velocity of emitted particles.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &UiParticleEmitterComponent::IsMovementCoordinateTypePolar)
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f);
                editInfo->DataElement("ParticleSpeed", &UiParticleEmitterComponent::m_particleSpeed, "Speed", "The speed of the emitted particles.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &UiParticleEmitterComponent::IsMovementCoordinateTypeCartesian);
                editInfo->DataElement("ParticleSpeedVariation", &UiParticleEmitterComponent::m_particleSpeedVariation, "Speed variation", "The random variation in speed of the emitted particles.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &UiParticleEmitterComponent::IsMovementCoordinateTypeCartesian);
                editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiParticleEmitterComponent::m_particleAccelerationCoordinateType, "Acceleration co-ordinate type", "The co-ordinate system to use for particle acceleration.")
                    ->EnumAttribute(UiParticleEmitterInterface::ParticleCoordinateType::Cartesian, "Cartesian")
                    ->EnumAttribute(UiParticleEmitterInterface::ParticleCoordinateType::Polar, "Polar")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC("RefreshEntireTree", 0xefbc823c));
                editInfo->DataElement("ParticleAcceleration", &UiParticleEmitterComponent::m_particleAcceleration, "Acceleration", "The acceleration of the particles.");
                editInfo->DataElement("IsParticleRotationFromVelocity", &UiParticleEmitterComponent::m_isParticleRotationFromVelocity, "Orientation velocity based", "The particle orientation is based on the current velocity.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC("RefreshEntireTree", 0xefbc823c));
                editInfo->DataElement("IsParticleInitialRotationFromInitialVelocity", &UiParticleEmitterComponent::m_isParticleInitialRotationFromInitialVelocity, "Initial orientation velocity based", "The particle orientation is based on the current velocity.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &UiParticleEmitterComponent::IsRotationRequired)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC("RefreshEntireTree", 0xefbc823c));
                editInfo->DataElement("ParticleInitialRotation", &UiParticleEmitterComponent::m_particleInitialRotation, "Initial rotation", "The initial rotation of the emitted particles in degrees.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &UiParticleEmitterComponent::IsInitialRotationRequired);
                editInfo->DataElement("ParticleIntiialRotationVariation", &UiParticleEmitterComponent::m_particleInitialRotationVariation, "Initial rotation variation", "The random variation in the initial rotation of the emitted particles in degrees.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &UiParticleEmitterComponent::IsInitialRotationRequired)
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f);
                editInfo->DataElement("ParticleRotation", &UiParticleEmitterComponent::m_particleRotationSpeed, "Rotation speed", "The rotation speed of the emitted particles.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &UiParticleEmitterComponent::IsRotationRequired);
                editInfo->DataElement("ParticleRotationVariation", &UiParticleEmitterComponent::m_particleRotationSpeedVariation, "Rotation speed variation", "The random variation in the rotation speed of the emitted particles.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &UiParticleEmitterComponent::IsRotationRequired)
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f);
            }

            {
                editInfo->ClassElement(AZ::Edit::ClassElements::Group, "Particle Size")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true);
                editInfo->DataElement("ParticleAspectRatioLocked", &UiParticleEmitterComponent::m_isParticleAspectRatioLocked, "Lock aspect ratio", "Locks the size of the particles to the current aspect ratio.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC("RefreshEntireTree", 0xefbc823c));
                editInfo->DataElement("ParticlePivot", &UiParticleEmitterComponent::m_particlePivot, "Pivot", "The pivot of the emitted particles.");
                editInfo->DataElement("ParticleSize", &UiParticleEmitterComponent::m_particleSize, "Size", "The size of the emitted particles.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiParticleEmitterComponent::OnParticleSizeChange)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC("RefreshAttributesAndValues", 0xcbc2147c))
                    ->Attribute(AZ::Edit::Attributes::Min, 0.1f);
                editInfo->DataElement("ParticleSizeVariation", &UiParticleEmitterComponent::m_particleSizeVariation, "Size variation", "The random variation in size of the emitted particles.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f);
            }

            {
                editInfo->ClassElement(AZ::Edit::ClassElements::Group, "Particle Color")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true);
                editInfo->DataElement("ParticleColor", &UiParticleEmitterComponent::m_particleColor, "Color", "The color of the emitted particles.");
                editInfo->DataElement("ParticleColorBrightnessVariation", &UiParticleEmitterComponent::m_particleColorBrightnessVariation, "Color brightness variation", "The color brightness random variation of the emitted particles.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 1.0f);
                editInfo->DataElement("ParticleColorTintVariation", &UiParticleEmitterComponent::m_particleColorTintVariation, "Color tint variation", "The color tint random variation of the emitted particles.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 1.0f);
                editInfo->DataElement("ParticleAlpha", &UiParticleEmitterComponent::m_particleAlpha, "Alpha", "The initial alpha of the emitted particles.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 1.0f);
            }

            {
                editInfo->ClassElement(AZ::Edit::ClassElements::Group, "Timelines")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true);
                editInfo->DataElement("ParticleSpeedMultiplier", &UiParticleEmitterComponent::m_particleSpeedMultiplier, "Speed multiplier", "The speed multiplier over time. Time range is [0,1] where 0 is the start of the particle lifetime and 1 is the end of the particle lifetime.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiParticleEmitterComponent::OnSpeedMultiplierChange)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC("RefreshAttributesAndValues", 0xcbc2147c))
                    ->Attribute(AZ::Edit::Attributes::Visibility, &UiParticleEmitterComponent::IsParticleLifetimeFinite)
                    ->ElementAttribute(AZ::Edit::Attributes::NameLabelOverride, "Keyframe");
                editInfo->DataElement("ParticleWidthMultiplier", &UiParticleEmitterComponent::m_particleWidthMultiplier, "Width multiplier", "The width multiplier over time. Time range is [0,1] where 0 is the start of the particle lifetime and 1 is the end of the particle lifetime.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiParticleEmitterComponent::OnSizeXMultiplierChange)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC("RefreshAttributesAndValues", 0xcbc2147c))
                    ->Attribute(AZ::Edit::Attributes::NameLabelOverride, &UiParticleEmitterComponent::GetParticleWidthMultiplierPropertyLabel)
                    ->Attribute(AZ::Edit::Attributes::DescriptionTextOverride, &UiParticleEmitterComponent::GetParticleWidthMultiplierPropertyDescription)
                    ->Attribute(AZ::Edit::Attributes::Visibility, &UiParticleEmitterComponent::IsParticleLifetimeFinite)
                    ->ElementAttribute(AZ::Edit::Attributes::NameLabelOverride, "Keyframe");
                editInfo->DataElement("ParticleHeightMultiplier", &UiParticleEmitterComponent::m_particleHeightMultiplier, "Height multiplier", "The height multiplier over time. Time range is [0,1] where 0 is the start of the particle lifetime and 1 is the end of the particle lifetime.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiParticleEmitterComponent::OnSizeYMultiplierChange)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC("RefreshAttributesAndValues", 0xcbc2147c))
                    ->Attribute(AZ::Edit::Attributes::Visibility, &UiParticleEmitterComponent::IsAspectRatioUnlocked)
                    ->Attribute(AZ::Edit::Attributes::Visibility, &UiParticleEmitterComponent::IsParticleLifetimeFinite)
                    ->ElementAttribute(AZ::Edit::Attributes::NameLabelOverride, "Keyframe");
                editInfo->DataElement("ParticleColorMultiplier", &UiParticleEmitterComponent::m_particleColorMultiplier, "Color multiplier", "The color multiplier over time. Time range is [0,1] where 0 is the start of the particle lifetime and 1 is the end of the particle lifetime.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiParticleEmitterComponent::OnColorMultiplierChange)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC("RefreshAttributesAndValues", 0xcbc2147c))
                    ->Attribute(AZ::Edit::Attributes::Visibility, &UiParticleEmitterComponent::IsParticleLifetimeFinite)
                    ->ElementAttribute(AZ::Edit::Attributes::NameLabelOverride, "Keyframe");
                editInfo->DataElement("ParticleAlphaMultiplier", &UiParticleEmitterComponent::m_particleAlphaMultiplier, "Alpha multiplier", "The alpha multiplier over time. Time range is [0,1] where 0 is the start of the particle lifetime and 1 is the end of the particle lifetime.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiParticleEmitterComponent::OnAlphaMultiplierChange)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC("RefreshAttributesAndValues", 0xcbc2147c))
                    ->Attribute(AZ::Edit::Attributes::Visibility, &UiParticleEmitterComponent::IsParticleLifetimeFinite)
                    ->ElementAttribute(AZ::Edit::Attributes::NameLabelOverride, "Keyframe");
            }
        }
    }

    AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
    if (behaviorContext)
    {
        behaviorContext->Enum<(int)UiParticleEmitterInterface::EmitShape::Point>("eUiEmitShape_Point")
            ->Enum<(int)UiParticleEmitterInterface::EmitShape::Circle>("eUiEmitShape_Circle")
            ->Enum<(int)UiParticleEmitterInterface::EmitShape::Quad>("eUiEmitShape_Quad")
            ->Enum<(int)UiParticleEmitterInterface::ParticleCoordinateType::Cartesian>("eUiParticleCoordinateType_Cartesian")
            ->Enum<(int)UiParticleEmitterInterface::ParticleCoordinateType::Polar>("eUiParticleCoordinateType_Polar")
            ->Enum<(int)UiParticleEmitterInterface::ParticleInitialDirectionType::RelativeToEmitAngle>("eUiParticleInitialDirectionType_RelativeToEmitAngle")
            ->Enum<(int)UiParticleEmitterInterface::ParticleInitialDirectionType::RelativeToEmitterCenter>("eUiParticleInitialDirectionType_RelativeToEmitterCenter");

        behaviorContext->EBus<UiParticleEmitterBus>("UiParticleEmitterBus")
            ->Event("GetIsEmitting", &UiParticleEmitterBus::Events::GetIsEmitting)
            ->Event("SetIsEmitting", &UiParticleEmitterBus::Events::SetIsEmitting)
            ->Event("GetIsRandomSeedFixed", &UiParticleEmitterBus::Events::GetIsRandomSeedFixed)
            ->Event("SetIsRandomSeedFixed", &UiParticleEmitterBus::Events::SetIsRandomSeedFixed)
            ->Event("GetRandomSeed", &UiParticleEmitterBus::Events::GetRandomSeed)
            ->Event("SetRandomSeed", &UiParticleEmitterBus::Events::SetRandomSeed)
            ->Event("GetIsParticlePositionRelativeToEmitter", &UiParticleEmitterBus::Events::GetIsParticlePositionRelativeToEmitter)
            ->Event("SetIsParticlePositionRelativeToEmitter", &UiParticleEmitterBus::Events::SetIsParticlePositionRelativeToEmitter)
            ->Event("GetParticleEmitRate", &UiParticleEmitterBus::Events::GetParticleEmitRate)
            ->Event("SetParticleEmitRate", &UiParticleEmitterBus::Events::SetParticleEmitRate)
            ->Event("GetIsEmitOnActivate", &UiParticleEmitterBus::Events::GetIsEmitOnActivate)
            ->Event("SetIsEmitOnActivate", &UiParticleEmitterBus::Events::SetIsEmitOnActivate)
            ->Event("GetIsHitParticleCountOnActivate", &UiParticleEmitterBus::Events::GetIsHitParticleCountOnActivate)
            ->Event("SetIsHitParticleCountOnActivate", &UiParticleEmitterBus::Events::SetIsHitParticleCountOnActivate)
            ->Event("GetIsEmitterLifetimeInfinite", &UiParticleEmitterBus::Events::GetIsEmitterLifetimeInfinite)
            ->Event("SetIsEmitterLifetimeInfinite", &UiParticleEmitterBus::Events::SetIsEmitterLifetimeInfinite)
            ->Event("GetEmitterLifetime", &UiParticleEmitterBus::Events::GetEmitterLifetime)
            ->Event("SetEmitterLifetime", &UiParticleEmitterBus::Events::SetEmitterLifetime)
            ->Event("GetIsParticleCountLimited", &UiParticleEmitterBus::Events::GetIsParticleCountLimited)
            ->Event("SetIsParticleCountLimited", &UiParticleEmitterBus::Events::SetIsParticleCountLimited)
            ->Event("GetMaxParticles", &UiParticleEmitterBus::Events::GetMaxParticles)
            ->Event("SetMaxParticles", &UiParticleEmitterBus::Events::SetMaxParticles)
            ->Event("GetEmitterShape", &UiParticleEmitterBus::Events::GetEmitterShape)
            ->Event("SetEmitterShape", &UiParticleEmitterBus::Events::SetEmitterShape)
            ->Event("GetIsEmitOnEdge", &UiParticleEmitterBus::Events::GetIsEmitOnEdge)
            ->Event("SetIsEmitOnEdge", &UiParticleEmitterBus::Events::SetIsEmitOnEdge)
            ->Event("GetInsideEmitDistance", &UiParticleEmitterBus::Events::GetInsideEmitDistance)
            ->Event("SetInsideEmitDistance", &UiParticleEmitterBus::Events::SetInsideEmitDistance)
            ->Event("GetOutsideEmitDistance", &UiParticleEmitterBus::Events::GetOutsideEmitDistance)
            ->Event("SetOutsideEmitDistance", &UiParticleEmitterBus::Events::SetOutsideEmitDistance)
            ->Event("GetParticleInitialDirectionType", &UiParticleEmitterBus::Events::GetParticleInitialDirectionType)
            ->Event("SetParticleInitialDirectionType", &UiParticleEmitterBus::Events::SetParticleInitialDirectionType)
            ->Event("GetEmitAngle", &UiParticleEmitterBus::Events::GetEmitAngle)
            ->Event("SetEmitAngle", &UiParticleEmitterBus::Events::SetEmitAngle)
            ->Event("GetEmitAngleVariation", &UiParticleEmitterBus::Events::GetEmitAngleVariation)
            ->Event("SetEmitAngleVariation", &UiParticleEmitterBus::Events::SetEmitAngleVariation)
            ->Event("GetIsParticleLifetimeInfinite", &UiParticleEmitterBus::Events::GetIsParticleLifetimeInfinite)
            ->Event("SetIsParticleLifetimeInfinite", &UiParticleEmitterBus::Events::SetIsParticleLifetimeInfinite)
            ->Event("GetParticleLifetime", &UiParticleEmitterBus::Events::GetParticleLifetime)
            ->Event("SetParticleLifetime", &UiParticleEmitterBus::Events::SetParticleLifetime)
            ->Event("GetParticleLifetimeVariation", &UiParticleEmitterBus::Events::GetParticleLifetimeVariation)
            ->Event("SetParticleLifetimeVariation", &UiParticleEmitterBus::Events::SetParticleLifetimeVariation)
            ->Event("GetSpritePathname", &UiParticleEmitterBus::Events::GetSpritePathname)
            ->Event("SetSpritePathname", &UiParticleEmitterBus::Events::SetSpritePathname)
            ->Event("GetIsSpriteSheetAnimated", &UiParticleEmitterBus::Events::GetIsSpriteSheetAnimated)
            ->Event("SetIsSpriteSheetAnimated", &UiParticleEmitterBus::Events::SetIsSpriteSheetAnimated)
            ->Event("GetIsSpriteSheetAnimationLooped", &UiParticleEmitterBus::Events::GetIsSpriteSheetAnimationLooped)
            ->Event("SetIsSpriteSheetAnimationLooped", &UiParticleEmitterBus::Events::SetIsSpriteSheetAnimationLooped)
            ->Event("GetIsSpriteSheetIndexRandom", &UiParticleEmitterBus::Events::GetIsSpriteSheetIndexRandom)
            ->Event("SetIsSpriteSheetIndexRandom", &UiParticleEmitterBus::Events::SetIsSpriteSheetIndexRandom)
            ->Event("GetSpriteSheetCellIndex", &UiParticleEmitterBus::Events::GetSpriteSheetCellIndex)
            ->Event("SetSpriteSheetCellIndex", &UiParticleEmitterBus::Events::SetSpriteSheetCellIndex)
            ->Event("GetSpriteSheetCellEndIndex", &UiParticleEmitterBus::Events::GetSpriteSheetCellEndIndex)
            ->Event("SetSpriteSheetCellEndIndex", &UiParticleEmitterBus::Events::SetSpriteSheetCellEndIndex)
            ->Event("GetSpriteSheetFrameDelay", &UiParticleEmitterBus::Events::GetSpriteSheetFrameDelay)
            ->Event("SetSpriteSheetFrameDelay", &UiParticleEmitterBus::Events::SetSpriteSheetFrameDelay)
            ->Event("GetIsParticleAspectRatioLocked", &UiParticleEmitterBus::Events::GetIsParticleAspectRatioLocked)
            ->Event("SetIsParticleAspectRatioLocked", &UiParticleEmitterBus::Events::SetIsParticleAspectRatioLocked)
            ->Event("GetParticlePivot", &UiParticleEmitterBus::Events::GetParticlePivot)
            ->Event("SetParticlePivot", &UiParticleEmitterBus::Events::SetParticlePivot)
            ->Event("GetParticleSize", &UiParticleEmitterBus::Events::GetParticleSize)
            ->Event("SetParticleSize", &UiParticleEmitterBus::Events::SetParticleSize)
            ->Event("GetParticleWidth", &UiParticleEmitterBus::Events::GetParticleWidth)
            ->Event("SetParticleWidth", &UiParticleEmitterBus::Events::SetParticleWidth)
            ->Event("GetParticleHeight", &UiParticleEmitterBus::Events::GetParticleHeight)
            ->Event("SetParticleHeight", &UiParticleEmitterBus::Events::SetParticleHeight)
            ->Event("GetParticleWidthVariation", &UiParticleEmitterBus::Events::GetParticleWidthVariation)
            ->Event("SetParticleWidthVariation", &UiParticleEmitterBus::Events::SetParticleWidthVariation)
            ->Event("GetParticleHeightVariation", &UiParticleEmitterBus::Events::GetParticleHeightVariation)
            ->Event("SetParticleHeightVariation", &UiParticleEmitterBus::Events::SetParticleHeightVariation)
            ->Event("GetParticleMovementCoordinateType", &UiParticleEmitterBus::Events::GetParticleMovementCoordinateType)
            ->Event("SetParticleMovementCoordinateType", &UiParticleEmitterBus::Events::SetParticleMovementCoordinateType)
            ->Event("GetParticleAccelerationMovementSpace", &UiParticleEmitterBus::Events::GetParticleAccelerationCoordinateType)
            ->Event("SetParticleAccelerationMovementSpace", &UiParticleEmitterBus::Events::SetParticleAccelerationCoordinateType)
            ->Event("GetParticleInitialVelocity", &UiParticleEmitterBus::Events::GetParticleInitialVelocity)
            ->Event("SetParticleInitialVelocity", &UiParticleEmitterBus::Events::SetParticleInitialVelocity)
            ->Event("GetParticleSpeed", &UiParticleEmitterBus::Events::GetParticleSpeed)
            ->Event("SetParticleSpeed", &UiParticleEmitterBus::Events::SetParticleSpeed)
            ->Event("GetParticleSpeedVariation", &UiParticleEmitterBus::Events::GetParticleSpeedVariation)
            ->Event("SetParticleSpeedVariation", &UiParticleEmitterBus::Events::SetParticleSpeedVariation)
            ->Event("GetParticleAcceleration", &UiParticleEmitterBus::Events::GetParticleAcceleration)
            ->Event("SetParticleAcceleration", &UiParticleEmitterBus::Events::SetParticleAcceleration)
            ->Event("GetIsParticleRotationFromVelocity", &UiParticleEmitterBus::Events::GetIsParticleRotationFromVelocity)
            ->Event("SetIsParticleRotationFromVelocity", &UiParticleEmitterBus::Events::SetIsParticleRotationFromVelocity)
            ->Event("GetIsParticleInitialRotationFromInitialVelocity", &UiParticleEmitterBus::Events::GetIsParticleInitialRotationFromInitialVelocity)
            ->Event("SetIsParticleInitialRotationFromInitialVelocity", &UiParticleEmitterBus::Events::SetIsParticleInitialRotationFromInitialVelocity)
            ->Event("GetParticleInitialRotation", &UiParticleEmitterBus::Events::GetParticleInitialRotation)
            ->Event("SetParticleInitialRotation", &UiParticleEmitterBus::Events::SetParticleInitialRotation)
            ->Event("GetParticleInitialRotationVariation", &UiParticleEmitterBus::Events::GetParticleInitialRotationVariation)
            ->Event("SetParticleInitialRotationVariation", &UiParticleEmitterBus::Events::SetParticleInitialRotationVariation)
            ->Event("GetParticleRotationSpeed", &UiParticleEmitterBus::Events::GetParticleRotationSpeed)
            ->Event("SetParticleRotationSpeed", &UiParticleEmitterBus::Events::SetParticleRotationSpeed)
            ->Event("GetParticleRotationSpeedVariation", &UiParticleEmitterBus::Events::GetParticleRotationSpeedVariation)
            ->Event("SetParticleRotationSpeedVariation", &UiParticleEmitterBus::Events::SetParticleRotationSpeedVariation)
            ->Event("GetParticleColor", &UiParticleEmitterBus::Events::GetParticleColor)
            ->Event("SetParticleColor", &UiParticleEmitterBus::Events::SetParticleColor)
            ->Event("GetParticleColorBrightnessVariation", &UiParticleEmitterBus::Events::GetParticleColorBrightnessVariation)
            ->Event("SetParticleColorBrightnessVariation", &UiParticleEmitterBus::Events::SetParticleColorBrightnessVariation)
            ->Event("GetParticleColorTintVariation", &UiParticleEmitterBus::Events::GetParticleColorTintVariation)
            ->Event("SetParticleColorTintVariation", &UiParticleEmitterBus::Events::SetParticleColorTintVariation)
            ->Event("GetParticleAlpha", &UiParticleEmitterBus::Events::GetParticleAlpha)
            ->Event("SetParticleAlpha", &UiParticleEmitterBus::Events::SetParticleAlpha);

        behaviorContext->Class<UiParticleEmitterComponent>()->RequestBus("UiParticleEmitterBus");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PROTECTED MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiParticleEmitterComponent::Init()
{
    // If this is called from RC.exe for example these pointers will not be set. In that case
    // we only need to be able to load, init and save the component. It will never be
    // activated.
    if (!(gEnv && gEnv->pLyShine))
    {
        return;
    }

    if (!m_sprite && !m_spritePathname.GetAssetPath().empty())
    {
        m_sprite = gEnv->pLyShine->LoadSprite(m_spritePathname.GetAssetPath().c_str());
    }

    m_currentAspectRatio = m_particleSize.GetX() / m_particleSize.GetY();
    m_currentParticleSize = m_particleSize;
    CreateMultiplierCurve(m_particleWidthMultiplierCurve, m_particleWidthMultiplier);
    CreateMultiplierCurve(m_particleHeightMultiplierCurve, m_particleHeightMultiplier);
    CreateMultiplierCurve(m_particleSpeedMultiplierCurve, m_particleSpeedMultiplier);
    CreateMultiplierCurve(m_particleColorMultiplierCurve, m_particleColorMultiplier);
    CreateMultiplierCurve(m_particleAlphaMultiplierCurve, m_particleAlphaMultiplier);

    m_cachedPrimitive.m_indices = nullptr;
    m_cachedPrimitive.m_vertices = nullptr;

    ResetParticleBuffers();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiParticleEmitterComponent::Activate()
{
    UiParticleEmitterBus::Handler::BusConnect(GetEntityId());
    UiInitializationBus::Handler::BusConnect(GetEntityId());
    UiRenderBus::Handler::BusConnect(GetEntityId());
    UiVisualBus::Handler::BusConnect(GetEntityId());
    UiCanvasSizeNotificationBus::Handler::BusConnect();
    UiElementNotificationBus::Handler::BusConnect(GetEntityId());

    AZ::EntityId canvasEntityId;
    EBUS_EVENT_ID_RESULT(canvasEntityId, GetEntityId(), UiElementBus, GetCanvasEntityId);
    if (canvasEntityId.IsValid())
    {
        bool isElementEnabled = false;
        EBUS_EVENT_ID_RESULT(isElementEnabled, GetEntityId(), UiElementBus, GetAreElementAndAncestorsEnabled);
        if (isElementEnabled)
        {
            UiCanvasUpdateNotificationBus::Handler::BusConnect(canvasEntityId);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiParticleEmitterComponent::Deactivate()
{
    UiParticleEmitterBus::Handler::BusDisconnect();
    UiInitializationBus::Handler::BusDisconnect();
    UiRenderBus::Handler::BusDisconnect();
    UiCanvasUpdateNotificationBus::Handler::BusDisconnect();
    UiVisualBus::Handler::BusDisconnect();
    UiCanvasSizeNotificationBus::Handler::BusDisconnect();
    UiElementNotificationBus::Handler::BusDisconnect();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiParticleEmitterComponent::ClearActiveParticles()
{
    m_particleContainer.clear();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Vector3 UiParticleEmitterComponent::GetRandomParticlePosition()
{
    AZ::Vector2 randomPosition = AZ::Vector2::CreateZero();

    if (m_emitShape == EmitShape::Point)
    {
        UiTransformInterface::RectPoints points;
        EBUS_EVENT_ID(GetEntityId(), UiTransformBus, GetCanvasSpacePointsNoScaleRotate, points);
        AZ::Vector2 centerPoint = (points.TopLeft() + points.BottomRight()) * 0.5f;

        if (IsMovementCoordinateTypeCartesian())
        {
            randomPosition = centerPoint;
        }
        else
        {
            const float angleOffsetToUp = -90.0f;
            float emitAngle = AZ::DegToRad(m_emitAngle + angleOffsetToUp) + AZ::DegToRad(m_emitAngleVariation) * (2.0f * m_random.GetRandomFloat() - 1.0f);
            float xDir = cos(emitAngle);
            float yDir = sin(emitAngle);
            randomPosition.Set(xDir, yDir);
            randomPosition += centerPoint;
        }
    }
    else if (m_emitShape == EmitShape::Circle)
    {
        UiTransformInterface::RectPoints points;
        EBUS_EVENT_ID(GetEntityId(), UiTransformBus, GetCanvasSpacePointsNoScaleRotate, points);

        AZ::Vector2 centerPoint = (points.TopLeft() + points.BottomRight()) * 0.5f;

        float halfWidth = (points.TopRight().GetX() - points.TopLeft().GetX()) * 0.5f;
        float halfHeight = (points.BottomLeft().GetY() - points.TopLeft().GetY()) * 0.5f;

        float unitCircleRandomRadius = m_random.GetRandomFloat();
        float circleDistance = sqrt(unitCircleRandomRadius);
        float uniformDistance = unitCircleRandomRadius;
        float majorRadius = AZ::GetMax<float>(halfWidth, halfHeight);
        float insideRadius = (m_isEmitOnEdge ? majorRadius - m_insideDistance : 0.0f);
        float percentageFromEdge = insideRadius / majorRadius;
        float randomDistanceFromCenter = circleDistance + (uniformDistance - circleDistance) * percentageFromEdge;

        float theta = m_random.GetRandomFloat() * AZ::Constants::TwoPi;
        float sinTheta = sin(theta);
        float cosTheta = cos(theta);
        if (m_isEmitOnEdge)
        {
            float minorRadius = AZ::GetMin<float>(halfWidth, halfHeight);
            float insideDistance = AZ::GetMin<float>(m_insideDistance, minorRadius);
            AZ::Vector2 curvenormal(halfHeight* cosTheta, halfWidth* sinTheta);
            curvenormal.NormalizeSafe();
            AZ::Vector2 edgeOffset = curvenormal * -1.0f * insideDistance + curvenormal * (insideDistance + m_outsideDistance) * randomDistanceFromCenter;
            randomPosition.Set(centerPoint.GetX() + halfWidth * cosTheta + edgeOffset.GetX(), centerPoint.GetY() + halfHeight * sinTheta + edgeOffset.GetY());
        }
        else
        {
            randomPosition.Set(centerPoint.GetX() + randomDistanceFromCenter * halfWidth * cosTheta, centerPoint.GetY() + halfHeight * randomDistanceFromCenter * sinTheta);
        }
    }
    else if (m_emitShape == EmitShape::Quad)
    {
        UiTransformInterface::RectPoints points;
        EBUS_EVENT_ID(GetEntityId(), UiTransformBus, GetCanvasSpacePointsNoScaleRotate, points);

        AZ::Vector2 shapeTopLeft = points.TopLeft();

        float width = points.TopRight().GetX() - points.TopLeft().GetX();
        float height = points.BottomLeft().GetY() - points.TopLeft().GetY();

        float x = m_random.GetRandomFloat();
        float y = m_random.GetRandomFloat();

        if (m_isEmitOnEdge)
        {
            // To keep the random emit points in a uniform distribution, this calculates a quad with the area
            // covering the entire edge with thickness defined by the inside and outisde distance. A random point
            // is chosen within this that is then mapped to the edge of the quad emit shape based on the x distance.

            float horizontalEdgeWidth = (width + 2 * m_outsideDistance);
            float verticalEdgeHeight = (height - m_insideDistance * 2);
            float randomTotalX = x * (horizontalEdgeWidth * 2 + (height - m_insideDistance * 2) * 2);

            if (randomTotalX < horizontalEdgeWidth) // top edge
            {
                float segmentX = randomTotalX / horizontalEdgeWidth;
                float segmentY = y;
                randomPosition.SetX(shapeTopLeft.GetX() - m_outsideDistance + segmentX * (width + 2 * m_outsideDistance));
                randomPosition.SetY(shapeTopLeft.GetY() - m_outsideDistance + segmentY * (m_insideDistance + m_outsideDistance));
            }
            else if (randomTotalX < horizontalEdgeWidth * 2) // bottom edge
            {
                float segmentX = ((randomTotalX - horizontalEdgeWidth) / horizontalEdgeWidth);
                float segmentY = y;
                randomPosition.SetX(shapeTopLeft.GetX() - m_outsideDistance + segmentX * (width + 2 * m_outsideDistance));
                randomPosition.SetY(shapeTopLeft.GetY() + (height - m_insideDistance) + segmentY * (m_insideDistance + m_outsideDistance));
            }
            else if (randomTotalX < (horizontalEdgeWidth * 2 + (height - m_insideDistance * 2))) // left edge
            {
                float segmentX = y;
                float segmentY = (randomTotalX - horizontalEdgeWidth * 2) / verticalEdgeHeight;
                randomPosition.SetX(shapeTopLeft.GetX() - m_outsideDistance + segmentX * (m_insideDistance + m_outsideDistance));
                randomPosition.SetY(shapeTopLeft.GetY() + m_insideDistance + segmentY * (height - 2 * m_insideDistance));
            }
            else // right edge
            {
                float segmentX = y;
                float segmentY = (randomTotalX - horizontalEdgeWidth * 2 - verticalEdgeHeight) / verticalEdgeHeight;
                randomPosition.SetX(shapeTopLeft.GetX() + (width - m_insideDistance) + segmentX * (m_insideDistance + m_outsideDistance));
                randomPosition.SetY(shapeTopLeft.GetY() + m_insideDistance + segmentY * (height - 2 * m_insideDistance));
            }
        }
        else
        {
            randomPosition.Set(shapeTopLeft.GetX() + width * x, shapeTopLeft.GetY() + height * y);
        }
    }

    AZ::Vector3 randomPositionV3(randomPosition.GetX(), randomPosition.GetY(), 1.0f);

    return randomPositionV3;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Vector2 UiParticleEmitterComponent::GetRandomParticleVelocity(const AZ::Vector2& particlePosition, const AZ::Vector2& emitterPosition)
{
    if (IsMovementCoordinateTypePolar())
    {
        return m_particleInitialVelocity + m_particleInitialVelocityVariation * (2.0f * m_random.GetRandomFloat() - 1.0f);
    }

    AZ::Vector2 randomVelocity;
    const float angleOffsetToUp = -90.0f;
    if (m_particleInitialDirectionType == ParticleInitialDirectionType::RelativeToEmitterCenter)
    {
        AZ::Vector2 fromShapeCenter = particlePosition - emitterPosition;
        fromShapeCenter.NormalizeSafe();
        float emitAngle = AZ::DegToRad(m_emitAngleVariation) * (2.0f * m_random.GetRandomFloat() - 1.0f);
        float cosAngle = cosf(emitAngle);
        float sinAngle = sinf(emitAngle);
        randomVelocity.SetX(fromShapeCenter.GetX() * cosAngle + fromShapeCenter.GetY() * sinAngle);
        randomVelocity.SetY(fromShapeCenter.GetX() * (-sinAngle) + fromShapeCenter.GetY() * cosAngle);
    }
    else
    {
        float emitAngle = AZ::DegToRad(m_emitAngle + angleOffsetToUp) + AZ::DegToRad(m_emitAngleVariation) * (2.0f * m_random.GetRandomFloat() - 1.0f);
        float xDir = cosf(emitAngle);
        float yDir = sinf(emitAngle);
        randomVelocity.Set(xDir, yDir);
    }

    randomVelocity = randomVelocity * (m_particleSpeed + m_particleSpeedVariation * (2.0f * m_random.GetRandomFloat() - 1.0f));

    return randomVelocity;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiParticleEmitterComponent::GetRandomParticleRotation(const AZ::Vector2& initialVelocity, const AZ::Vector2& initialPosition)
{
    if (m_isParticleRotationFromVelocity)
    {
        return 0;
    }
    else if (m_isParticleInitialRotationFromInitialVelocity && IsMovementCoordinateTypeCartesian())
    {
        return atan2(initialVelocity.GetY(), initialVelocity.GetX()) + AZ::DegToRad(90.0f);
    }
    else if (m_isParticleInitialRotationFromInitialVelocity && IsMovementCoordinateTypePolar())
    {
        AZ::Vector2 offset = initialPosition;
        float radius = AZ::GetMax<float>(offset.GetLength(), 0.1f);
        float newRadius = radius + initialVelocity.GetX();
        if (newRadius > 0.0f)
        {
            offset = (initialPosition / radius) * newRadius;
            float angle = (initialVelocity.GetY()) / newRadius;
            AZ::Vector2 nextPosition(offset.GetX() * cos(angle) + offset.GetY() * sin(angle), (-offset.GetX()) * sin(angle) + offset.GetY() * cos(angle));
            return atan2(nextPosition.GetY() - initialPosition.GetY(), nextPosition.GetX() - initialPosition.GetX()) + AZ::DegToRad(90.0f);
        }
        else
        {
            return 0.0f;
        }
    }
    else
    {
        return AZ::DegToRad(m_particleInitialRotation) + AZ::DegToRad(m_particleInitialRotationVariation) * (2.0f * m_random.GetRandomFloat() - 1.0f);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiParticleEmitterComponent::CreateMultiplierCurve(C2DSplineTrack& curve, const AZStd::vector<ParticleFloatKeyframe>& pointList)
{
    auto pointsIterator = pointList.begin();
    while (pointsIterator != pointList.end())
    {
        const ParticleFloatKeyframe& currentPoint = *pointsIterator;

        I2DBezierKey newKey;
        newKey.time = currentPoint.time;
        newKey.flags = 0;
        newKey.value = Vec2(currentPoint.time, currentPoint.multiplier);

        int newKeyIndex = curve.CreateKey(currentPoint.time);
        curve.SetKey(newKeyIndex, &newKey);
        int keyFlags = curve.GetKeyFlags(newKeyIndex);
        SetCurveKeyTangentFlags(keyFlags, currentPoint.inTangent, currentPoint.outTangent);
        curve.SetKeyFlags(newKeyIndex, keyFlags);
        pointsIterator++;
    }

    if (pointList.size() == 0)
    {
        curve.SetValue(0.0f, 1.0f);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiParticleEmitterComponent::CreateMultiplierCurve(UiCompoundSplineTrack& curve, const AZStd::vector<ParticleColorKeyframe>& pointList)
{
    CUiAnimParamType animType[4];
    animType[0] = EUiAnimParamType::eUiAnimParamType_Float;
    animType[1] = EUiAnimParamType::eUiAnimParamType_Float;
    animType[2] = EUiAnimParamType::eUiAnimParamType_Float;
    animType[3] = EUiAnimParamType::eUiAnimParamType_Float;

    const int curveDimensions = 3;
    curve = UiCompoundSplineTrack(curveDimensions, EUiAnimValue::eUiAnimValue_RGB, animType);

    if (pointList.size() > 0)
    {
        auto pointsIterator = pointList.begin();
        while (pointsIterator != pointList.end())
        {
            const ParticleColorKeyframe& currentPoint = *pointsIterator;
            for (int i = 0; i < curveDimensions; i++)
            {
                I2DBezierKey newKey;
                newKey.time = currentPoint.time;
                newKey.flags = 0;
                newKey.value = Vec2(currentPoint.time, currentPoint.color.GetElement(i));

                IUiAnimTrack* track = curve.GetSubTrack(i);
                int newKeyIndex = track->CreateKey(currentPoint.time);
                track->SetKey(newKeyIndex, &newKey);
                int keyFlags = track->GetKeyFlags(newKeyIndex);
                SetCurveKeyTangentFlags(keyFlags, currentPoint.inTangent, currentPoint.outTangent);
                track->SetKeyFlags(newKeyIndex, keyFlags);
            }
            pointsIterator++;
        }
    }
    else
    {
        AZ::Color col(1.0f, 1.0f, 1.0f, 1.0f);
        curve.SetValue(0.0f, col);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
int UiParticleEmitterComponent::GetCurveIndividualTangentFlags(ParticleKeyframeTangentType tangentType)
{
    int flags = 0;

    if (tangentType == ParticleKeyframeTangentType::EaseIn || tangentType == ParticleKeyframeTangentType::EaseOut)
    {
        flags = SPLINE_KEY_TANGENT_ZERO;
    }
    else if (tangentType == ParticleKeyframeTangentType::Linear)
    {
        flags = SPLINE_KEY_TANGENT_LINEAR;
    }
    else
    {
        flags = SPLINE_KEY_TANGENT_STEP;
    }

    return flags;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiParticleEmitterComponent::SetCurveKeyTangentFlags(int& flags, ParticleKeyframeTangentType inTangent, ParticleKeyframeTangentType outTangent)
{
    flags &= ~(SPLINE_KEY_TANGENT_IN_MASK | SPLINE_KEY_TANGENT_OUT_MASK);

    flags |= (GetCurveIndividualTangentFlags(inTangent) << SPLINE_KEY_TANGENT_IN_SHIFT);
    flags |= (GetCurveIndividualTangentFlags(outTangent) << SPLINE_KEY_TANGENT_OUT_SHIFT);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiParticleEmitterComponent::SortMultipliersByTime(AZStd::vector<ParticleFloatKeyframe>& pointList)
{
    AZStd::sort(pointList.begin(), pointList.end(),
        [](const ParticleFloatKeyframe& key1, const ParticleFloatKeyframe& key2) { return key1.time < key2.time; });
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiParticleEmitterComponent::ResetParticleBuffers()
{
    // if this cached primitive is in any list in the render graph then mark the render graph as dirty
    if (m_cachedPrimitive.m_next)
    {
        MarkRenderGraphDirty();
    }

    if (m_isParticleLifetimeInfinite)
    {
        // there must be a limit on active particles if the lifetime is infinite so the active particles limit can be used directly here
        m_particleBufferSize = m_maxParticles;
    }
    else
    {
        AZ::u32 calculatedMaxParticles = static_cast<AZ::u32>(m_emitRate * (m_particleLifetime + m_particleLifetimeVariation));
        m_particleBufferSize = AZ::GetClamp<AZ::u32>(calculatedMaxParticles, 1, m_activeParticlesLimit);
    }

    const int indicesPerParticle = 6;
    AZ::u32 numIndices = m_particleBufferSize * indicesPerParticle;
    if (m_cachedPrimitive.m_indices)
    {
        delete [] m_cachedPrimitive.m_indices;
    }
    m_cachedPrimitive.m_indices = new uint16[numIndices];

    const uint16 verticesPerParticle = 4;
    uint16 baseIndex = 0;
    for (AZ::u32 i = 0; i < numIndices; i += indicesPerParticle)
    {
        m_cachedPrimitive.m_indices[i + 0] = 0 + baseIndex;
        m_cachedPrimitive.m_indices[i + 1] = 1 + baseIndex;
        m_cachedPrimitive.m_indices[i + 2] = 3 + baseIndex;
        m_cachedPrimitive.m_indices[i + 3] = 2 + baseIndex;
        m_cachedPrimitive.m_indices[i + 4] = 3 + baseIndex;
        m_cachedPrimitive.m_indices[i + 5] = 1 + baseIndex;
        baseIndex += verticesPerParticle;
    }

    AZ::u32 numVertices = m_particleBufferSize * verticesPerParticle;

    if (m_cachedPrimitive.m_vertices)
    {
        delete [] m_cachedPrimitive.m_vertices;
    }
    m_cachedPrimitive.m_vertices = new SVF_P2F_C4B_T2F_F4B[numVertices];

    m_particleContainer.clear();
    m_particleContainer.reserve(m_particleBufferSize);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiParticleEmitterComponent::IsEmitterLifetimeFinite()
{
    return !m_isEmitterLifetimeInfinite;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiParticleEmitterComponent::IsParticleLifetimeFinite()
{
    return !m_isParticleLifetimeInfinite;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiParticleEmitterComponent::IsParticleLimitRequired()
{
    return (m_isParticleCountLimited || m_isParticleLifetimeInfinite);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiParticleEmitterComponent::IsParticleLimitToggleable()
{
    // if the particle life time is infinite, then there should be a limit on the amount of active particles
    return !m_isParticleLifetimeInfinite;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiParticleEmitterComponent::IsEmitAngleRequired()
{
    return (IsMovementCoordinateTypeCartesian() && m_particleInitialDirectionType == ParticleInitialDirectionType::RelativeToEmitAngle);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiParticleEmitterComponent::CanEmitFromCenter()
{
    return (IsMovementCoordinateTypeCartesian() && m_emitShape != EmitShape::Point);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiParticleEmitterComponent::IsInitialRotationRequired()
{
    return !m_isParticleInitialRotationFromInitialVelocity && !m_isParticleRotationFromVelocity;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiParticleEmitterComponent::IsRotationRequired()
{
    return !m_isParticleRotationFromVelocity;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiParticleEmitterComponent::IsEmitFromGivenAngle()
{
    return (m_emitShape == EmitShape::Point);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiParticleEmitterComponent::IsShapeWithEdge()
{
    return (m_emitShape == EmitShape::Circle || m_emitShape == EmitShape::Quad);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiParticleEmitterComponent::IsEmittingFromEdge()
{
    return (IsShapeWithEdge() && m_isEmitOnEdge);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiParticleEmitterComponent::CheckMaxParticleValidity()
{
    if (m_isParticleLifetimeInfinite)
    {
        m_isParticleCountLimited = true;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiParticleEmitterComponent::OnSpritePathnameChange()
{
    ISprite* newSprite = nullptr;

    if (!m_spritePathname.GetAssetPath().empty())
    {
        // Load the new texture.
        newSprite = gEnv->pLyShine->LoadSprite(m_spritePathname.GetAssetPath().c_str());
    }

    SAFE_RELEASE(m_sprite);
    m_sprite = newSprite;

    m_spriteSheetCellIndex = 0;
    if (IsSpriteTypeSpriteSheet())
    {
        m_spriteSheetCellEndIndex = static_cast<AZ::u32>(m_sprite->GetSpriteSheetCells().size() - 1);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiParticleEmitterComponent::OnSpriteSheetCellIndexChanged()
{
    m_spriteSheetCellEndIndex = AZ::GetMax(m_spriteSheetCellIndex, m_spriteSheetCellEndIndex);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiParticleEmitterComponent::OnSpriteSheetCellEndIndexChanged()
{
    m_spriteSheetCellIndex = AZ::GetMin(m_spriteSheetCellIndex, m_spriteSheetCellEndIndex);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiParticleEmitterComponent::OnParticleSizeChange()
{
    if (m_isParticleAspectRatioLocked && m_currentAspectRatio > 0)
    {
        if (m_particleSize.GetX() != m_currentParticleSize.GetX())
        {
            m_particleSize.SetY(m_particleSize.GetX() / m_currentAspectRatio);
        }
        else if (m_particleSize.GetY() != m_currentParticleSize.GetY())
        {
            m_particleSize.SetX(m_particleSize.GetY() * m_currentAspectRatio);
        }
    }
    else if (m_particleSize.GetY() > 0)
    {
        m_currentAspectRatio = m_particleSize.GetX() / m_particleSize.GetY();
    }
    m_currentParticleSize = m_particleSize;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiParticleEmitterComponent::OnSizeXMultiplierChange()
{
    SortMultipliersByTime(m_particleWidthMultiplier);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiParticleEmitterComponent::OnSizeYMultiplierChange()
{
    SortMultipliersByTime(m_particleHeightMultiplier);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiParticleEmitterComponent::OnSpeedMultiplierChange()
{
    SortMultipliersByTime(m_particleSpeedMultiplier);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiParticleEmitterComponent::OnColorMultiplierChange()
{
    AZStd::sort(m_particleColorMultiplier.begin(), m_particleColorMultiplier.end(),
        [](const ParticleColorKeyframe& key1, const ParticleColorKeyframe& key2) { return key1.time < key2.time; });
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiParticleEmitterComponent::OnAlphaMultiplierChange()
{
    SortMultipliersByTime(m_particleAlphaMultiplier);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiParticleEmitterComponent::IsSpriteTypeSpriteSheet()
{
    return (m_sprite && m_sprite->GetSpriteSheetCells().size() > 1);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiParticleEmitterComponent::IsSpriteSheetCellRangeRequired()
{
    return (IsSpriteTypeSpriteSheet() && (m_isSpriteSheetAnimated || m_isSpriteSheetIndexRandom));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiParticleEmitterComponent::IsMovementCoordinateTypeCartesian()
{
    return (m_particleMovementCoordinateType == ParticleCoordinateType::Cartesian);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiParticleEmitterComponent::IsMovementCoordinateTypePolar()
{
    return (m_particleMovementCoordinateType == ParticleCoordinateType::Polar);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiParticleEmitterComponent::IsAspectRatioUnlocked()
{
    return !m_isParticleAspectRatioLocked;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const char* UiParticleEmitterComponent::GetSpriteSheetIndexPropertyLabel()
{
    const char* label = "Sprite sheet Index";

    if (IsSpriteSheetCellRangeRequired())
    {
        label = "Sprite sheet start frame";
    }

    return label;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const char* UiParticleEmitterComponent::GetParticleWidthMultiplierPropertyLabel()
{
    const char* label = "Width multiplier";

    if (m_isParticleAspectRatioLocked)
    {
        label = "Size multiplier";
    }

    return label;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const char* UiParticleEmitterComponent::GetParticleWidthMultiplierPropertyDescription()
{
    const char* label = "The width multiplier over time. Time range is [0,1] where 0 is the start of the particle lifetime and 1 is the end of the particle lifetime.";

    if (m_isParticleAspectRatioLocked)
    {
        label = "The size multiplier over time. Time range is [0,1] where 0 is the start of the particle lifetime and 1 is the end of the particle lifetime.";
    }

    return label;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiParticleEmitterComponent::AZu32ComboBoxVec UiParticleEmitterComponent::PopulateSpriteSheetIndexStringList()
{
    // There may not be a sprite loaded for this component
    if (m_sprite)
    {
        const AZ::u32 numCells = static_cast<AZ::u32>(m_sprite->GetSpriteSheetCells().size());

        if (numCells != 0)
        {
            return LyShine::GetEnumSpriteIndexList(GetEntityId(), 0, numCells - 1);
        }
    }
    return LyShine::AZu32ComboBoxVec();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiParticleEmitterComponent::MarkRenderGraphDirty()
{
    // tell the canvas to invalidate the render graph
    AZ::EntityId canvasEntityId;
    EBUS_EVENT_ID_RESULT(canvasEntityId, GetEntityId(), UiElementBus, GetCanvasEntityId);
    EBUS_EVENT_ID(canvasEntityId, UiCanvasComponentImplementationBus, MarkRenderGraphDirty);
}
