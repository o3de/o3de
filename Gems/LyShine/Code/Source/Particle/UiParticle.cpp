/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "UiParticle.h"

#include "UiParticleEmitterComponent.h"

#include <LyShine/ISprite.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiParticle::Init(UiParticle::UiParticleInitialParameters* initialParams)
{
    m_particleAge = 0.0f;
    m_particleLifetime = initialParams->lifetime;

    m_emitterInitialOffset = initialParams->initialEmitterOffset;
    m_position = initialParams->position;
    m_positionDifference = AZ::Vector2::CreateZero();
    m_velocity = initialParams->initialVelocity;
    m_accelerationBasedVelocity = AZ::Vector2::CreateZero();
    m_acceleration = initialParams->acceleration;

    m_rotation = initialParams->rotation;
    m_angularVelocity = initialParams->angularVelocity;
    m_pivot = initialParams->pivot;

    m_size = initialParams->size;
    m_color = initialParams->color;
    m_spriteCellIndex = initialParams->spriteSheetCellIndex;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiParticle::Update(float deltaTime, const UiParticleUpdateParameters& updateParameters)
{
    AZ::Vector2 previousPosition = m_position;
    float velocityStrength = 1.0f;
    if (updateParameters.isSpeedMultiplierUsed)
    {
        float particleLifetimePercentage = (updateParameters.isParticleInfinite ? 0.0f : m_particleAge / m_particleLifetime);
        updateParameters.speedMultiplier->GetValue(particleLifetimePercentage, velocityStrength);
    }
    AZ::Vector2 currentVelocity = m_velocity * velocityStrength;
    if (updateParameters.isVelocityCartesian)
    {
        m_position += currentVelocity * deltaTime;
    }
    else
    {
        AZ::Vector2 offset = m_position - m_emitterInitialOffset;
        float radius = AZ::GetMax<float>(offset.GetLength(), 0.1f);
        float newRadius = radius + currentVelocity.GetX() * deltaTime;
        if (newRadius > 0.0f)
        {
            offset = (offset / radius) * newRadius;
            float angle = (currentVelocity.GetY() * deltaTime) / newRadius;
            m_position.SetX(offset.GetX() * cos(angle) + offset.GetY() * sin(angle));
            m_position.SetY((-offset.GetX()) * sin(angle) + offset.GetY() * cos(angle));
            m_position += m_emitterInitialOffset;
        }
        else
        {
            m_position = m_emitterInitialOffset;
        }
    }

    if (updateParameters.isAccelerationCartesian)
    {
        m_position += m_accelerationBasedVelocity * deltaTime;
        m_position += 0.5f * deltaTime * deltaTime * m_acceleration;
    }
    else
    {
        AZ::Vector2 offset = m_position - m_emitterInitialOffset;
        float radius = AZ::GetMax<float>(offset.GetLength(), 0.1f);
        float newRadius = radius + m_accelerationBasedVelocity.GetX() * deltaTime;
        if (newRadius > 0.0f)
        {
            offset = (offset / radius) * newRadius;
            float angle = (m_accelerationBasedVelocity.GetY() * deltaTime) / newRadius;
            m_position.SetX(offset.GetX() * cos(angle) + offset.GetY() * sin(angle));
            m_position.SetY((-offset.GetX()) * sin(angle) + offset.GetY() * cos(angle));
            m_position += m_emitterInitialOffset;
        }
        else
        {
            m_position = m_emitterInitialOffset;
        }
    }

    m_positionDifference = m_position - previousPosition;
    m_accelerationBasedVelocity += m_acceleration * deltaTime;
    m_rotation += m_angularVelocity * deltaTime;
    m_particleAge += deltaTime;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiParticle::FillVertices(LyShine::UiPrimitiveVertex* outputVertices, const UiParticleRenderParameters& renderParameters, const AZ::Matrix4x4& transform)
{
    float particleLifetimePercentage = (renderParameters.isParticleInfinite ? 0.0f : m_particleAge / m_particleLifetime);
    float alphaStrength = 1.0f;

    AZ::Color currentColor = m_color;

    if (renderParameters.isColorOverrideUsed)
    {
        currentColor = renderParameters.colorOverride;
    }
    if (renderParameters.isAlphaOverrideUsed)
    {
        currentColor.SetA(renderParameters.alphaOverride);
    }

    if (renderParameters.isAlphaMultiplierUsed)
    {
        renderParameters.alphaMultiplier->GetValue(particleLifetimePercentage, alphaStrength);
    }
    AZ::u8 currentAlpha = static_cast<AZ::u8>(currentColor.GetA8() * alphaStrength * renderParameters.alphaFadeMultiplier);

    if (currentAlpha == 0)
    {
        return false;
    }

    int currentIndex = m_spriteCellIndex;
    if (renderParameters.spritesheetCellIndexAnimated)
    {
        int unwrappedIndex = m_spriteCellIndex + static_cast<int>(m_particleAge / renderParameters.spritesheetFrameDelay);
        int clampedIndex = unwrappedIndex - renderParameters.spritesheetStartFrame;
        if (!renderParameters.spritesheetCellIndexAnimationLooped)
        {
            clampedIndex = AZ::GetClamp<int>(clampedIndex, 0, renderParameters.spritesheetFrameRange);
        }
        int rangeIncludingEndFrame = renderParameters.spritesheetFrameRange + 1;
        currentIndex = renderParameters.spritesheetStartFrame + (clampedIndex % rangeIncludingEndFrame);
    }

    const UiTransformInterface::RectPoints& uvCoords = (renderParameters.sprite ? renderParameters.sprite->GetCellUvCoords(currentIndex) : UiTransformInterface::RectPoints(0, 1, 0, 1));

    const AZ::Vector2 uvs[4] =
    {
        uvCoords.TopLeft(),
        uvCoords.TopRight(),
        uvCoords.BottomRight(),
        uvCoords.BottomLeft(),
    };

    float widthMultiplier = 1.0f;
    float heightMultiplier = 1.0f;
    if (renderParameters.isWidthMultiplierUsed)
    {
        renderParameters.sizeWidthMultiplier->GetValue(particleLifetimePercentage, widthMultiplier);
    }

    if (renderParameters.isAspectRatioLocked)
    {
        heightMultiplier = widthMultiplier;
    }
    else if (renderParameters.isHeightMultiplierUsed)
    {
        renderParameters.sizeHeightMultiplier->GetValue(particleLifetimePercentage, heightMultiplier);
    }

    AZ::Color colorStrength(1.0f, 1.0f, 1.0f, 1.0f);
    if (renderParameters.isColorMultiplierUsed)
    {
        renderParameters.colorMultiplier->GetValue(particleLifetimePercentage, colorStrength);
    }
    currentColor = currentColor * colorStrength;
    uint32 packedColor = (currentAlpha << 24) | (currentColor.GetR8() << 16) | (currentColor.GetG8() << 8) | currentColor.GetB8();

    AZ::Vector2 unitQuadQuarters[4];
    unitQuadQuarters[0].Set(0.0f - m_pivot.GetX(), 0.0f - m_pivot.GetY());
    unitQuadQuarters[1].Set(1.0f - m_pivot.GetX(), 0.0f - m_pivot.GetY());
    unitQuadQuarters[2].Set(1.0f - m_pivot.GetX(), 1.0f - m_pivot.GetY());
    unitQuadQuarters[3].Set(0.0f - m_pivot.GetX(), 1.0f - m_pivot.GetY());

    float sinRotation = sin(m_rotation);
    float cosRotation = cos(m_rotation);

    AZ::Vector2 particleDirectionVectors[2];
    if (renderParameters.isRotationVelocityBased)
    {
        particleDirectionVectors[1] = m_positionDifference.GetNormalizedSafe() * -1.0f;
        particleDirectionVectors[0] = particleDirectionVectors[1].GetPerpendicular() * -1.0f;
    }
    else
    {
        particleDirectionVectors[0].Set(cosRotation, sinRotation);
        particleDirectionVectors[1].Set(-sinRotation, cosRotation);
    }

    AZ::Vector2 particlePosition = m_position;
    if (renderParameters.isRelativeToEmitter)
    {
        particlePosition += (*renderParameters.particleOffset) - m_emitterInitialOffset;
    }

    const int verticesPerParticle = 4;
    for (int i = 0; i < verticesPerParticle; ++i)
    {
        AZ::Vector2 cornerVector = particlePosition + (unitQuadQuarters[i].GetX() * particleDirectionVectors[0] * m_size.GetX() * widthMultiplier) + (unitQuadQuarters[i].GetY() * particleDirectionVectors[1] * m_size.GetY() * heightMultiplier);

        AZ::Vector3 point3(cornerVector.GetX(), cornerVector.GetY(), 1.0f);
        point3 = transform * point3;

        outputVertices[i].xy = Vec2(point3.GetX(), point3.GetY());
        outputVertices[i].color.dcolor = packedColor;
        outputVertices[i].st = Vec2(uvs[i].GetX(), uvs[i].GetY());
        outputVertices[i].texIndex = 0;
        outputVertices[i].texHasColorChannel = 1;
        outputVertices[i].texIndex2 = 0;
        outputVertices[i].pad = 0;
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiParticle::IsActive(bool infiniteLifetime) const
{
    return (m_particleAge < m_particleLifetime || infiniteLifetime);
}
