/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Matrix3x4.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/std/parallel/atomic.h>
#include <GradientSignal/Ebuses/GradientTransformRequestBus.h>
#include <GradientSignal/Ebuses/GradientTransformModifierRequestBus.h>
#include <GradientSignal/Util.h>
#include <LmbrCentral/Dependency/DependencyMonitor.h>
#include <LmbrCentral/Dependency/DependencyNotificationBus.h>

namespace LmbrCentral
{
    template<typename, typename>
    class EditorWrappedComponentBase;
}

namespace GradientSignal
{
    class GradientTransformConfig
        : public AZ::ComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(GradientTransformConfig, AZ::SystemAllocator);
        AZ_RTTI(GradientTransformConfig, "{1106FD53-8B3A-4F97-8051-E34AD70199A5}", AZ::ComponentConfig);

        static void Reflect(AZ::ReflectContext* context);

        bool operator==(const GradientTransformConfig& other) const;
        bool operator!=(const GradientTransformConfig& other) const;

        bool IsAdvancedModeReadOnly() const;
        bool IsReferenceReadOnly() const;
        bool IsBoundsReadOnly() const;
        bool IsTranslateReadOnly() const;
        bool IsRotateReadOnly() const;
        bool IsScaleReadOnly() const;

        bool m_advancedMode = false;

        bool m_allowReference = false;
        AZ::EntityId m_shapeReference;

        bool m_overrideBounds = false;
        AZ::Vector3 m_bounds = AZ::Vector3::CreateOne(); //1m sq default value chosen by design, to start small and expand as needed
        AZ::Vector3 m_center = AZ::Vector3::CreateZero(); // to handle asymmetrical shapes such as polygon prism

        TransformType m_transformType = TransformType::World_ThisEntity;
        bool m_overrideTranslate = false;
        AZ::Vector3 m_translate = AZ::Vector3::CreateZero();
        bool m_overrideRotate = false;
        AZ::Vector3 m_rotate = AZ::Vector3::CreateZero();
        bool m_overrideScale = false;
        AZ::Vector3 m_scale = AZ::Vector3::CreateOne();

        float m_frequencyZoom = 1.0f;

        WrappingType m_wrappingType = WrappingType::None;
        bool m_is3d = false;
    };

    inline constexpr AZ::TypeId GradientTransformComponentTypeId{ "{F0A8F968-F642-4982-8282-8FB8560FDB67}" };

    class GradientTransformComponent
        : public AZ::Component
        , private GradientTransformRequestBus::Handler
        , private LmbrCentral::DependencyNotificationBus::Handler
        , private AZ::TickBus::Handler
        , private GradientTransformModifierRequestBus::Handler
    {
    public:
        friend class EditorGradientTransformComponent;
        template<typename, typename> friend class LmbrCentral::EditorWrappedComponentBase;
        AZ_COMPONENT(GradientTransformComponent, GradientTransformComponentTypeId);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void Reflect(AZ::ReflectContext* context);

        GradientTransformComponent(const GradientTransformConfig& configuration);
        GradientTransformComponent() = default;
        ~GradientTransformComponent() = default;

        // AZ::Component interface
        void Activate() override;
        void Deactivate() override;
        bool ReadInConfig(const AZ::ComponentConfig* baseConfig) override;
        bool WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const override;

        //////////////////////////////////////////////////////////////////////////
        // GradientTransformRequestBus
        const GradientTransform& GetGradientTransform() const override;

        //////////////////////////////////////////////////////////////////////////
        // DependencyNotificationBus
        void OnCompositionChanged() override;

        //////////////////////////////////////////////////////////////////////////
        // AZ::TickBus::Handler
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

        void UpdateFromShape(bool notifyDependentsOfChange);

        AZ::EntityId GetShapeEntityId() const;

    protected:
        //////////////////////////////////////////////////////////////////////////
        // GradientTransformModifierRequestBus
        bool GetAllowReference() const override;
        void SetAllowReference(bool value) override;

        AZ::EntityId GetShapeReference() const override;
        void SetShapeReference(AZ::EntityId shapeReference) override;

        bool GetOverrideBounds() const override;
        void SetOverrideBounds(bool value) override;

        AZ::Vector3 GetBounds() const override;
        void SetBounds(const AZ::Vector3& bounds) override;

        AZ::Vector3 GetCenter() const override;
        void SetCenter(const AZ::Vector3& center) override;

        TransformType GetTransformType() const override;
        void SetTransformType(TransformType type) override;

        bool GetOverrideTranslate() const override;
        void SetOverrideTranslate(bool value) override;

        AZ::Vector3 GetTranslate() const override;
        void SetTranslate(const AZ::Vector3& translate) override;

        bool GetOverrideRotate() const override;
        void SetOverrideRotate(bool value) override;

        AZ::Vector3 GetRotate() const override;
        void SetRotate(const AZ::Vector3& rotate) override;

        bool GetOverrideScale() const override;
        void SetOverrideScale(bool value) override;

        AZ::Vector3 GetScale() const override;
        void SetScale(const AZ::Vector3& scale) override;

        float GetFrequencyZoom() const override;
        void SetFrequencyZoom(float frequencyZoom) override;

        WrappingType GetWrappingType() const override;
        void SetWrappingType(WrappingType type) override;

        bool GetIs3D() const override;
        void SetIs3D(bool value) override;

        bool GetAdvancedMode() const override;
        void SetAdvancedMode(bool value) override;

    private:
        mutable AZStd::recursive_mutex m_cacheMutex;
        GradientTransformConfig m_configuration;
        LmbrCentral::DependencyMonitor m_dependencyMonitor;
        AZStd::atomic_bool m_dirty{ false };
        GradientTransform m_gradientTransform;
    };
} //namespace GradientSignal
