/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <LyShine/Bus/UiCanvasBus.h>
#include <LyShine/Bus/UiParticleEmitterBus.h>
#include <LyShine/Bus/UiInitializationBus.h>
#include <LyShine/Bus/UiElementBus.h>
#include <LyShine/Bus/UiRenderBus.h>
#include <LyShine/Bus/UiCanvasUpdateNotificationBus.h>
#include <LyShine/Bus/UiVisualBus.h>
#include <LyShine/UiComponentTypes.h>
#include <LyShine/IRenderGraph.h>

#include <AzCore/Component/Component.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Math/Random.h>

#include <LmbrCentral/Rendering/MaterialAsset.h>

#include <Particle/UiParticle.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
class UiParticleEmitterComponent
    : public AZ::Component
    , public UiCanvasSizeNotificationBus::Handler
    , public UiParticleEmitterBus::Handler
    , public UiInitializationBus::Handler
    , public UiRenderBus::Handler
    , public UiCanvasUpdateNotificationBus::Handler
    , public UiElementNotificationBus::Handler
    , public UiVisualBus::Handler
{
public: // member functions

    AZ_COMPONENT(UiParticleEmitterComponent, LyShine::UiParticleEmitterComponentUuid, AZ::Component);

    UiParticleEmitterComponent();
    ~UiParticleEmitterComponent() override;

    // UiCanvasSizeNotificationInterface
    void OnCanvasSizeOrScaleChange(AZ::EntityId canvasEntityId) override;
    // ~UiCanvasSizeNotificationBusInterface

    // UiParticleEmitterInterface
    bool GetIsEmitting() override;
    void SetIsEmitting(bool emitParticles) override;
    bool GetIsRandomSeedFixed() override;
    void SetIsRandomSeedFixed(bool randomSeedFixed) override;
    int GetRandomSeed() override;
    void SetRandomSeed(int randomSeed) override;
    bool GetIsParticlePositionRelativeToEmitter() override;
    void SetIsParticlePositionRelativeToEmitter(bool relativeToEmitter) override;
    float GetParticleEmitRate() override;
    void SetParticleEmitRate(float particleEmitRate) override;
    bool GetIsEmitOnActivate() override;
    void SetIsEmitOnActivate(bool emitOnActivate) override;
    bool GetIsHitParticleCountOnActivate() override;
    void SetIsHitParticleCountOnActivate(bool hitParticleCountOnActivate) override;
    bool GetIsEmitterLifetimeInfinite() override;
    void SetIsEmitterLifetimeInfinite(bool emitterLifetimeInfinite) override;
    float GetEmitterLifetime() override;
    void SetEmitterLifetime(float emitterLifetime) override;
    bool GetIsParticleCountLimited() override;
    void SetIsParticleCountLimited(bool particleCountLimited) override;
    AZ::u32 GetMaxParticles() override;
    void SetMaxParticles(AZ::u32 maxParticles) override;
    EmitShape GetEmitterShape() override;
    void SetEmitterShape(EmitShape emitterShape) override;
    bool GetIsEmitOnEdge() override;
    void SetIsEmitOnEdge(bool emitOnEdge) override;
    float GetInsideEmitDistance() override;
    void SetInsideEmitDistance(float insideEmitDistance) override;
    float GetOutsideEmitDistance() override;
    void SetOutsideEmitDistance(float outsideEmitDistance) override;
    ParticleInitialDirectionType GetParticleInitialDirectionType() override;
    void SetParticleInitialDirectionType(ParticleInitialDirectionType initialDirectionType) override;
    float GetEmitAngle() override;
    void SetEmitAngle(float emitAngle) override;
    float GetEmitAngleVariation() override;
    void SetEmitAngleVariation(float emitAngleVariation) override;
    bool GetIsParticleLifetimeInfinite() override;
    void SetIsParticleLifetimeInfinite(bool infiniteLifetime) override;
    float GetParticleLifetime() override;
    void SetParticleLifetime(float lifetime) override;
    float GetParticleLifetimeVariation() override;
    void SetParticleLifetimeVariation(float lifetimeVariation) override;
    ISprite* GetSprite() override;
    void SetSprite(ISprite* sprite) override;
    AZStd::string GetSpritePathname() override;
    void SetSpritePathname(AZStd::string spritePath) override;
    bool GetIsSpriteSheetAnimated() override;
    void SetIsSpriteSheetAnimated(bool isSpriteSheetAnimated) override;
    bool GetIsSpriteSheetAnimationLooped() override;
    void SetIsSpriteSheetAnimationLooped(bool isSpriteSheetAnimationLooped) override;
    bool GetIsSpriteSheetIndexRandom() override;
    void SetIsSpriteSheetIndexRandom(bool isSpriteSheetIndexRandom) override;
    int GetSpriteSheetCellIndex() override;
    void SetSpriteSheetCellIndex(int spriteSheetIndex) override;
    int GetSpriteSheetCellEndIndex() override;
    void SetSpriteSheetCellEndIndex(int spriteSheetEndIndex) override;
    float GetSpriteSheetFrameDelay() override;
    void SetSpriteSheetFrameDelay(float spriteSheetFrameDelay) override;
    bool GetIsParticleAspectRatioLocked() override;
    void SetIsParticleAspectRatioLocked(bool aspectRatioLocked) override;
    AZ::Vector2 GetParticlePivot() override;
    void SetParticlePivot(AZ::Vector2 particlePivot) override;
    AZ::Vector2 GetParticleSize() override;
    void SetParticleSize(AZ::Vector2 particleSize) override;
    float GetParticleWidth() override;
    void SetParticleWidth(float width) override;
    float GetParticleWidthVariation() override;
    void SetParticleWidthVariation(float widthVariation) override;
    float GetParticleHeight() override;
    void SetParticleHeight(float height) override;
    float GetParticleHeightVariation() override;
    void SetParticleHeightVariation(float heightVariation) override;
    ParticleCoordinateType GetParticleMovementCoordinateType() override;
    void SetParticleMovementCoordinateType(ParticleCoordinateType particleVelocityMovement) override;
    ParticleCoordinateType GetParticleAccelerationCoordinateType() override;
    void SetParticleAccelerationCoordinateType(ParticleCoordinateType particleAccelerationMovement) override;
    AZ::Vector2 GetParticleInitialVelocity() override;
    void SetParticleInitialVelocity(AZ::Vector2 initialVelocity) override;
    AZ::Vector2 GetParticleInitialVelocityVariation() override;
    void SetParticleInitialVelocityVariation(AZ::Vector2 initialVelocityVariation) override;
    float GetParticleSpeed() override;
    void SetParticleSpeed(float speed) override;
    float GetParticleSpeedVariation() override;
    void SetParticleSpeedVariation(float speedVariation) override;
    AZ::Vector2 GetParticleAcceleration() override;
    void SetParticleAcceleration(AZ::Vector2 acceleration) override;
    bool GetIsParticleRotationFromVelocity() override;
    void SetIsParticleRotationFromVelocity(bool rotationFromVelocity) override;
    bool GetIsParticleInitialRotationFromInitialVelocity() override;
    void SetIsParticleInitialRotationFromInitialVelocity(bool rotationFromVelocity) override;
    float GetParticleInitialRotation() override;
    void SetParticleInitialRotation(float initialRotation) override;
    float GetParticleInitialRotationVariation() override;
    void SetParticleInitialRotationVariation(float initialRotationVariation) override;
    float GetParticleRotationSpeed() override;
    void SetParticleRotationSpeed(float rotationSpeed) override;
    float GetParticleRotationSpeedVariation() override;
    void SetParticleRotationSpeedVariation(float rotationSpeedVariation) override;
    AZ::Color GetParticleColor() override;
    void SetParticleColor(AZ::Color color) override;
    float GetParticleColorBrightnessVariation() override;
    void SetParticleColorBrightnessVariation(float brightnessVariation) override;
    float GetParticleColorTintVariation() override;
    void SetParticleColorTintVariation(float tintVariation) override;
    float GetParticleAlpha() override;
    void SetParticleAlpha(float alpha) override;
    // ~UiParticleEmitterInterface

    // UiInitializationInterface
    void InGamePostActivate() override;
    // ~UiInitializationInterface

    // UiRenderInterface
    void Render(LyShine::IRenderGraph* renderGraph) override;
    // ~UiRenderInterface

    // UiCanvasUpdateNotification
    void Update(float deltaTime) override;
    // ~UiCanvasUpdateNotification

    // UiElementNotifications
    void OnUiElementFixup(AZ::EntityId canvasEntityId, AZ::EntityId parentEntityId) override;
    void OnUiElementAndAncestorsEnabledChanged(bool areElementAndAncestorsEnabled) override;
    // ~UiElementNotifications

    // UiVisualInterface
    void ResetOverrides() override;
    void SetOverrideColor(const AZ::Color& color) override;
    void SetOverrideAlpha(float alpha) override;
    // ~UiVisualInterface

public:  // static member functions

    static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("UiParticleEmitterService"));
        provided.push_back(AZ_CRC("UiVisualService", 0xa864fdf8));
    }

    static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("UiTransformService", 0x3a838e34));
    }

    static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("UiVisualService", 0xa864fdf8));
    }

    static void Reflect(AZ::ReflectContext* context);

protected: // member functions

    // AZ::Component
    void Init() override;
    void Activate() override;
    void Deactivate() override;
    // ~AZ::Component

    void ClearActiveParticles();
    AZ::Vector3 GetRandomParticlePosition();
    AZ::Vector2 GetRandomParticleVelocity(const AZ::Vector2& particlePosition, const AZ::Vector2& emitterPosition);
    float GetRandomParticleRotation(const AZ::Vector2& initialVelocity, const AZ::Vector2& initialPosition);
    void CreateMultiplierCurve(C2DSplineTrack& curve, const AZStd::vector<ParticleFloatKeyframe>& pointList);
    void CreateMultiplierCurve(UiCompoundSplineTrack& curve, const AZStd::vector<ParticleColorKeyframe>& pointList);
    int GetCurveIndividualTangentFlags(ParticleKeyframeTangentType tangent);
    void SetCurveKeyTangentFlags(int& flags, ParticleKeyframeTangentType inTangent, ParticleKeyframeTangentType outTangent);
    void SortMultipliersByTime(AZStd::vector<ParticleFloatKeyframe>& pointList);
    void ResetParticleBuffers();

    bool IsEmitterLifetimeFinite();
    bool IsParticleLifetimeFinite();
    bool IsParticleLimitRequired();
    bool IsParticleLimitToggleable();
    bool IsEmitAngleRequired();
    bool CanEmitFromCenter();
    bool IsInitialRotationRequired();
    bool IsRotationRequired();
    bool IsEmitFromGivenAngle();
    bool IsShapeWithEdge();
    bool IsEmittingFromEdge();
    const char* GetSpriteSheetIndexPropertyLabel();
    const char* GetParticleWidthMultiplierPropertyLabel();
    const char* GetParticleWidthMultiplierPropertyDescription();
    bool IsSpriteTypeSpriteSheet();
    bool IsSpriteSheetCellRangeRequired();
    bool IsMovementCoordinateTypeCartesian();
    bool IsMovementCoordinateTypePolar();
    bool IsAspectRatioUnlocked();
    void CheckMaxParticleValidity();
    void OnSpritePathnameChange();
    void OnSpriteSheetCellIndexChanged();
    void OnSpriteSheetCellEndIndexChanged();
    void OnParticleSizeChange();
    void OnSizeXMultiplierChange();
    void OnSizeYMultiplierChange();
    void OnSpeedMultiplierChange();
    void OnColorMultiplierChange();
    void OnAlphaMultiplierChange();

    using AZu32ComboBoxVec = AZStd::vector<AZStd::pair<AZ::u32, AZStd::string> >;
    AZu32ComboBoxVec PopulateSpriteSheetIndexStringList();

    //! Mark the render graph as dirty, this should be done when any change is made affects the structure of the graph
    void MarkRenderGraphDirty();

protected: // data

    AZ_DISABLE_COPY_MOVE(UiParticleEmitterComponent);

    static const AZ::u32 m_activeParticlesLimit;
    static const float m_emitRateLimit;

    bool m_isRandomSeedFixed                        = false;
    int m_randomSeed                                = 0;
    bool m_isPositionRelativeToEmitter              = false;
    float m_emitRate                                = 300.0f;
    bool m_isEmitOnActivate                         = true;
    bool m_isHitParticleCountOnActivate             = false;
    bool m_isEmitterLifetimeInfinite                = true;
    float m_emitterLifetime                         = 1.0f;
    bool m_isParticleCountLimited                   = false;
    AZ::u32 m_maxParticles                          = 100;
    EmitShape m_emitShape                           = EmitShape::Point;
    bool m_isEmitOnEdge                             = false;
    float m_insideDistance                          = 0.0f;
    float m_outsideDistance                         = 0.0f;
    float m_emitAngle                               = 0.0f;
    float m_emitAngleVariation                      = 180.0f;

    bool m_isParticleLifetimeInfinite               = false;
    float m_particleLifetime                        = 2.0f;
    float m_particleLifetimeVariation               = 0.5f;
    AzFramework::SimpleAssetReference<LmbrCentral::TextureAsset> m_spritePathname;
    bool m_isSpriteSheetAnimated                    = false;
    bool m_isSpriteSheetAnimationLooped             = true;
    bool m_isSpriteSheetIndexRandom                 = false;
    AZ::u32 m_spriteSheetCellIndex                  = 0;
    AZ::u32 m_spriteSheetCellEndIndex               = 0;
    float m_spriteSheetFrameDelay                   = 0.0f;
    LyShine::BlendMode m_blendMode                  = LyShine::BlendMode::Normal;

    ISprite* m_sprite = nullptr;

    bool m_isParticleAspectRatioLocked              = true;
    AZ::Vector2 m_particlePivot                     = AZ::Vector2(0.5f, 0.5f);
    AZ::Vector2 m_particleSize                      = AZ::Vector2(5.0f, 5.0f);
    AZ::Vector2 m_particleSizeVariation             = AZ::Vector2(0.0f, 0.0f);
    AZStd::vector<ParticleFloatKeyframe> m_particleWidthMultiplier;
    AZStd::vector<ParticleFloatKeyframe> m_particleHeightMultiplier;
    C2DSplineTrack m_particleWidthMultiplierCurve;
    C2DSplineTrack m_particleHeightMultiplierCurve;

    ParticleCoordinateType m_particleMovementCoordinateType     = ParticleCoordinateType::Cartesian;
    ParticleCoordinateType m_particleAccelerationCoordinateType = ParticleCoordinateType::Cartesian;
    // Initial velocity for Polar movement
    AZ::Vector2 m_particleInitialVelocity           = AZ::Vector2(0.0f, 0.0f);
    AZ::Vector2 m_particleInitialVelocityVariation  = AZ::Vector2(0.0f, 0.0f);
    // Initial speed for Cartesian movement
    float m_particleSpeed                           = 45.0f;
    float m_particleSpeedVariation                  = 30.0f;
    ParticleInitialDirectionType m_particleInitialDirectionType = ParticleInitialDirectionType::RelativeToEmitAngle; // used with Cartesian movement to calculate direction
    AZ::Vector2 m_particleAcceleration              = AZ::Vector2(0.0f, 40.0f);
    bool m_isParticleRotationFromVelocity           = false;
    bool m_isParticleInitialRotationFromInitialVelocity = false;
    float m_particleInitialRotation                 = 0.0f;
    float m_particleInitialRotationVariation        = 0.0f;
    float m_particleRotationSpeed                   = 0.0f;
    float m_particleRotationSpeedVariation          = 0.0f;
    AZStd::vector<ParticleFloatKeyframe> m_particleSpeedMultiplier;
    C2DSplineTrack m_particleSpeedMultiplierCurve;

    AZ::Color m_particleColor                       = AZ::Color(1.0f, 1.0f, 1.0f, 1.0f); // alpha is ignored
    float m_particleColorBrightnessVariation        = 0.0f;
    float m_particleColorTintVariation              = 0.0f;
    AZStd::vector<ParticleColorKeyframe> m_particleColorMultiplier;
    UiCompoundSplineTrack m_particleColorMultiplierCurve;
    float m_particleAlpha                           = 1.0f; // alpha separated as it's more likely to be animated
    AZStd::vector<ParticleFloatKeyframe> m_particleAlphaMultiplier;
    C2DSplineTrack m_particleAlphaMultiplierCurve;

    bool m_isColorOverridden                        = false;
    bool m_isAlphaOverridden                        = false;
    AZ::Color m_overrideColor                       = AZ::Color(1.0f, 1.0f, 1.0f, 1.0f);
    float m_overrideAlpha                           = 1.0f;

    float m_emitterAge                              = 0.0f;
    float m_nextEmitTime                            = 0.0f;

    bool m_isEmitting                               = false;

    float m_currentAspectRatio                      = 1.0f;
    AZ::Vector2 m_currentParticleSize               = AZ::Vector2(5.0f, 5.0f);

    AZ::SimpleLcgRandom m_random;

    AZStd::vector<UiParticle> m_particleContainer;

    AZ::u32 m_particleBufferSize                    = 0;
    LyShine::UiPrimitive m_cachedPrimitive;
};
