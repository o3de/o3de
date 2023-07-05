/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#if defined(CARBONATED)
#include <AzFramework/Components/TransformComponent.h>
#include <AzFramework/Visibility/EntityBoundsUnionBus.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Quaternion.h>
#include <AzFramework/Network/NetBindingHandlerBus.h>
#include <AzFramework/Network/NetBindingSystemBus.h>
#include <AzFramework/Network/NetworkContext.h>

#include <GridMate/Replica/ReplicaChunk.h>
#include <GridMate/Replica/ReplicaFunctions.h>
#include <GridMate/Replica/DataSet.h>
#include <GridMate/Serialize/CompressionMarshal.h>


namespace AZ
{
    class BehaviorTransformNotificationBusHandler : public TransformNotificationBus::Handler, public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(BehaviorTransformNotificationBusHandler, "{9CEF4DAB-F359-4A3E-9856-7780281E0DAA}", AZ::SystemAllocator
            , OnTransformChanged
            , OnParentChanged
            , OnChildAdded
            , OnChildRemoved
        );

        void OnTransformChanged(const Transform& localTM, const Transform& worldTM) override
        {
            Call(FN_OnTransformChanged, localTM, worldTM);
        }

        void OnParentChanged(EntityId oldParent, EntityId newParent) override
        {
            Call(FN_OnParentChanged, oldParent, newParent);
        }

        void OnChildAdded(EntityId child) override
        {
            Call(FN_OnChildAdded, child);
        }

        void OnChildRemoved(EntityId child) override
        {
            Call(FN_OnChildRemoved, child);
        }
    };

    void TransformConfigConstructor(TransformConfig* self, ScriptDataContext& dc)
    {
        const int numArgs = dc.GetNumArguments();
        if (numArgs == 0)
        {
            new(self) TransformConfig();
        }
        else if (numArgs == 1 && dc.IsClass<Transform>(0))
        {
            Transform transform;
            dc.ReadArg(0, transform);

            new(self) TransformConfig(transform);
        }
        else
        {
            dc.GetScriptContext()->Error(AZ::ScriptContext::ErrorType::Error, true, "Invalid constructor call. Valid overrides are: TransformConfig(), TransfomConfig(Transform)");

            new(self) TransformConfig();
        }
    }

} // namespace AZ

namespace AzFramework
{
    //=========================================================================
    // TransformReplicaChunk
    // [3/9/2016]
    //=========================================================================
    class TransformReplicaChunk
        : public GridMate::ReplicaChunkBase
    {
    public:
        AZ_CLASS_ALLOCATOR(TransformReplicaChunk, AZ::SystemAllocator, 0);

        static const char* GetChunkName() { return "TransformReplicaChunk"; }

        TransformReplicaChunk()
            : m_parentId("ParentId")
            , m_localTranslation("LocalTranslationData")
            , m_localRotation("LocalRotationData")
            , m_localScale("LocalScaleData")
        {
#if defined(CARBONATED)
            m_localTranslation.GetThrottler().SetThreshold(AZ::Vector3(0.01f, 0.01f, 0.01f));
            m_localScale.GetThrottler().SetThreshold(AZ::Vector3(0.01f, 0.01f, 0.01f));
#else
            m_localTranslation.GetThrottler().SetThreshold(AZ::Vector3(0.005f, 0.005f, 0.005f));
            m_localScale.GetThrottler().SetThreshold(AZ::Vector3(0.001f, 0.001f, 0.001f));
#endif
        }

        bool IsReplicaMigratable() override
        {
            return true;
        }

        void SetInitialTM(const AZ::Transform& t)
        {
            m_initialWorldTM = t;
        }

        void SetLocalTM(const AZ::Transform& t)
        {
            // Gruber patch begin // VMED -- missed ExtractScaleExact, CreateFromTransform are replaced
            AZ::Transform copy = t;
            const float uniformScale = copy.GetUniformScale();
            m_localScale.Set(AZ::Vector3(uniformScale, uniformScale, uniformScale));
#if defined(CARBONATED)
            AZ_Assert(copy.GetTranslation().IsFinite(), "SetLocalTM: Transform is invalid");
#endif
            m_localTranslation.Set(copy.GetTranslation());
            m_localRotation.Set(copy.GetRotation());
            // Gruber patch end // VMED
        }

        AZ::Transform GetLocalTransform() const
        {
            AZ::Transform newXform = AZ::Transform::CreateFromQuaternionAndTranslation(m_localRotation.Get(), m_localTranslation.Get());
            // Gruber patch begin // VMED -- missed MultiplyByScale is replaced
            newXform.MultiplyByUniformScale(m_localScale.Get().GetX()); // local scale is setup as uniform scale
            //  Gruber patch end // VMED
            return newXform;
        }

        unsigned int GetLocalTime()
        {
            return GetReplicaManager()->GetTime().m_localTime;
        }

        // parentId (can have no parent)
        DataSet<AZ::u64>::BindInterface<TransformComponent, &TransformComponent::OnNewNetParentData> m_parentId;

        // transform
        DataSet<AZ::Vector3, GridMate::Marshaler<AZ::Vector3>, GridMate::EpsilonThrottle<AZ::Vector3>>::BindInterface<TransformComponent, &TransformComponent::OnNewPositionData> m_localTranslation;
        DataSet<AZ::Quaternion, GridMate::Marshaler<AZ::Quaternion>, GridMate::BasicThrottle<AZ::Quaternion>>::BindInterface<TransformComponent, &TransformComponent::OnNewRotationData> m_localRotation;
        DataSet<AZ::Vector3, GridMate::Marshaler<AZ::Vector3>, GridMate::EpsilonThrottle<AZ::Vector3>>::BindInterface<TransformComponent, &TransformComponent::OnNewScaleData> m_localScale;

        AZ::Transform m_initialWorldTM;

        class Descriptor
            : public ExternalChunkDescriptor<TransformReplicaChunk>
        {
        public:
            ReplicaChunkBase* CreateFromStream(UnmarshalContext& context) override
            {
                // Pre/Post construct allow DataSets and RPCs to bind to the chunk.
                TransformReplicaChunk* transformChunk = aznew TransformReplicaChunk;
                context.m_iBuf->Read(transformChunk->m_initialWorldTM);
                return transformChunk;
            }

            void DiscardCtorStream(UnmarshalContext& context) override
            {
                AZ::Transform discard;
                context.m_iBuf->Read(discard);
            }

            void MarshalCtorData(ReplicaChunkBase* chunk, WriteBuffer& wb) override
            {
                TransformReplicaChunk* transformChunk = static_cast<TransformReplicaChunk*>(chunk);
                TransformComponent* transformComponent = static_cast<TransformComponent*>(transformChunk->GetHandler());
                if (transformComponent)
                {
                    wb.Write(transformComponent->GetWorldTM());
                }
                else
                {
                    wb.Write(transformChunk->m_initialWorldTM);
                }
            }
        };
    };

    //=========================================================================
    bool TransformComponentVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
    {
        if (classElement.GetVersion() < 3)
        {
            // IsStatic field added at v3.
            // It should be false for old versions of TransformComponent.
            classElement.AddElementWithData(context, "IsStatic", false);
        }

        // note the == on the following line.  Do not add to this block.  If you add an "InterpolateScale" back in, then
        // consider erasing this block.
        // The version was bumped from 3 to 4 to ensure this code runs.
        // if you add the field back in, then increment the version number again.
        if (classElement.GetVersion() == 3)
        {
            // a field was temporarily added to this specific version, then was removed.
            // However, some data may have been exported with this field present, so 
            // remove it if its found, but only in this version which the change was present in, so that
            // future re-additions of it won't remove it (as long as they bump the version number.)
            classElement.RemoveElementByName(AZ_CRC("InterpolateScale", 0x9d00b831));
        }
        

        return true;
    }

    //=========================================================================
    // TransformComponent
    // [8/9/2013]
    //=========================================================================
    TransformComponent::TransformComponent()
        : m_parentTM(nullptr)
        , m_parentActive(false)
        , m_onNewParentKeepWorldTM(true)
        , m_parentActivationTransformMode(ParentActivationTransformMode::MaintainOriginalRelativeTransform)
        , m_isStatic(false)
        , m_interpolatePosition(AZ::InterpolationMode::NoInterpolation)
        , m_interpolateRotation(AZ::InterpolationMode::NoInterpolation)
#if defined(CARBONATED)
        , m_isClientSimulated(false)
#endif
    {
        m_localTM = AZ::Transform::CreateIdentity();
        m_worldTM = AZ::Transform::CreateIdentity();
    }

    TransformComponent::TransformComponent(const TransformComponent& copy)
        : m_localTM(copy.m_localTM)
        , m_worldTM(copy.m_worldTM)
        , m_parentId(copy.m_parentId)
        , m_parentTM(copy.m_parentTM)
        , m_parentActive(copy.m_parentActive)
        , m_notificationBus(nullptr)
        , m_onNewParentKeepWorldTM(copy.m_onNewParentKeepWorldTM)
        , m_parentActivationTransformMode(copy.m_parentActivationTransformMode)
        , m_replicaChunk(nullptr)
        , m_isStatic(copy.m_isStatic)
        , m_interpolatePosition(copy.m_interpolatePosition)
        , m_interpolateRotation(copy.m_interpolateRotation)
        , m_netTargetTranslation()
        , m_netTargetRotation()
        , m_netTargetScale(copy.m_netTargetScale)        
    {
        CreateSamples();
        if (copy.m_netTargetTranslation)
        {
            m_netTargetTranslation->SetNewTarget(copy.m_netTargetTranslation->GetTargetValue(), copy.m_netTargetTranslation->GetTargetTimestamp());
        }
        if (copy.m_netTargetRotation)
        {
            m_netTargetRotation->SetNewTarget(copy.m_netTargetRotation->GetTargetValue(), copy.m_netTargetRotation->GetTargetTimestamp());
        }

        SetSyncEnabled(copy.m_isSyncEnabled);
    }


    void TransformComponent::CreateTranslationSample()
    {
        switch(m_interpolatePosition)
        {
        case AZ::InterpolationMode::LinearInterpolation:
            m_netTargetTranslation = AZStd::make_unique<AZ::LinearlyInterpolatedSample<AZ::Vector3>>();
            break;
        case AZ::InterpolationMode::NoInterpolation:
        default:
            m_netTargetTranslation = AZStd::make_unique<AZ::UninterpolatedSample<AZ::Vector3>>();
            break;
        }
    }

    void TransformComponent::CreateRotationSample()
    {
        switch (m_interpolateRotation)
        {
        case AZ::InterpolationMode::LinearInterpolation:
            m_netTargetRotation = AZStd::make_unique<AZ::LinearlyInterpolatedSample<AZ::Quaternion>>();
            break;
        case AZ::InterpolationMode::NoInterpolation:
        default:
            m_netTargetRotation = AZStd::make_unique<AZ::UninterpolatedSample<AZ::Quaternion>>();
            break;
        }
    }

    void TransformComponent::CreateSamples()
    {
        if (m_netTargetTranslation)
        {
            auto target = m_netTargetTranslation->GetTargetValue();
            auto timeStamp = m_netTargetTranslation->GetTargetTimestamp();

            CreateTranslationSample();

            m_netTargetTranslation->SetNewTarget(target, timeStamp);
        }
        else
        {
            CreateTranslationSample();
        }

        if (m_netTargetRotation)
        {
            auto target = m_netTargetRotation->GetTargetValue();
            auto timeStamp = m_netTargetRotation->GetTargetTimestamp();

            CreateRotationSample();

            m_netTargetRotation->SetNewTarget(target, timeStamp);
        }
        else
        {
            CreateRotationSample();
        }
    }

    TransformComponent::~TransformComponent()
    {
    }

    bool TransformComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (auto config = azrtti_cast<const AZ::TransformConfig*>(baseConfig))
        {
            m_localTM = config->m_localTransform;
            m_worldTM = config->m_worldTransform;
            m_parentId = config->m_parentId;
            m_parentActivationTransformMode = config->m_parentActivationTransformMode;
            SetSyncEnabled(config->m_netSyncEnabled);
            m_interpolatePosition = config->m_interpolatePosition;
            m_interpolateRotation = config->m_interpolateRotation;
            m_isStatic = config->m_isStatic;
            return true;
        }
        return false;
    }

    bool TransformComponent::WriteOutConfig(AZ::ComponentConfig* baseConfig) const
    {
        if (auto config = azrtti_cast<AZ::TransformConfig*>(baseConfig))
        {
            config->m_localTransform = m_localTM;
            config->m_worldTransform = m_worldTM;
            config->m_parentId = m_parentId;
            config->m_parentActivationTransformMode = m_parentActivationTransformMode;
            config->m_netSyncEnabled = IsSyncEnabled();
            config->m_interpolatePosition = m_interpolatePosition;
            config->m_interpolateRotation = m_interpolateRotation;
            config->m_isStatic = m_isStatic;
            return true;
        }
        return false;
    }

    void TransformComponent::Activate()
    {
        AZ::TransformBus::Handler::BusConnect(m_entity->GetId());
        AZ::TransformNotificationBus::Bind(m_notificationBus, m_entity->GetId());


        const bool keepWorldTm = (m_parentActivationTransformMode == ParentActivationTransformMode::MaintainCurrentWorldTransform || !m_parentId.IsValid());
        SetParentImpl(m_parentId, keepWorldTm);
    }

    void TransformComponent::Deactivate()
    {
        EBUS_EVENT_ID(m_parentId, AZ::TransformNotificationBus, OnChildRemoved, GetEntityId());

        UnbindFromNetwork();

        m_notificationBus = nullptr;
        if (m_parentId.IsValid())
        {
            AZ::TransformNotificationBus::Handler::BusDisconnect();
            AZ::TransformHierarchyInformationBus::Handler::BusDisconnect();
            AZ::EntityBus::Handler::BusDisconnect();
        }
        AZ::TransformBus::Handler::BusDisconnect();
    }

    void TransformComponent::BindTransformChangedEventHandler(AZ::TransformChangedEvent::Handler& handler)
    {
        handler.Connect(m_transformChangedEvent);
    }

    void TransformComponent::BindParentChangedEventHandler(AZ::ParentChangedEvent::Handler& handler)
    {
        handler.Connect(m_parentChangedEvent);
    }

    void TransformComponent::BindChildChangedEventHandler(AZ::ChildChangedEvent::Handler& handler)
    {
        handler.Connect(m_childChangedEvent);
    }

    void TransformComponent::NotifyChildChangedEvent(AZ::ChildChangeType changeType, AZ::EntityId entityId)
    {
        m_childChangedEvent.Signal(changeType, entityId);
    }


    void TransformComponent::SetLocalTM(const AZ::Transform& tm)
    {
        if (AreMoveRequestsAllowed())
        {
            SetLocalTMImpl(tm);
            UpdateReplicaChunk();
        }
    }

    void TransformComponent::SetWorldTM(const AZ::Transform& tm)
    {
        if (AreMoveRequestsAllowed())
        {
            SetWorldTMImpl(tm);
            UpdateReplicaChunk();
        }
    }

    void TransformComponent::SetParent(AZ::EntityId id)
    {
        if (!IsNetworkControlled())
        {
            SetParentImpl(id, true);

            UpdateReplicaChunk();
        }
    }

    void TransformComponent::SetParentRelative(AZ::EntityId id)
    {
        if (!IsNetworkControlled())
        {
            SetParentImpl(id, m_isStatic);

            UpdateReplicaChunk();
        }
    }

    void TransformComponent::SetWorldTranslation(const AZ::Vector3& newPosition)
    {
        AZ::Transform newWorldTransform = m_worldTM;
        newWorldTransform.SetTranslation(newPosition);
        SetWorldTM(newWorldTransform);
    }

    void TransformComponent::SetLocalTranslation(const AZ::Vector3& newPosition)
    {
        AZ::Transform newLocalTransform = m_localTM;
        newLocalTransform.SetTranslation(newPosition);
        SetLocalTM(newLocalTransform);
    }

    AZ::Vector3 TransformComponent::GetWorldTranslation()
    {
        return m_worldTM.GetTranslation();
    }

    AZ::Vector3 TransformComponent::GetLocalTranslation()
    {
        return m_localTM.GetTranslation();
    }

    void TransformComponent::MoveEntity(const AZ::Vector3& offset)
    {
        const AZ::Vector3& worldPosition = m_worldTM.GetTranslation();
        SetWorldTranslation(worldPosition + offset);
    }

    void TransformComponent::SetWorldX(float x)
    {
        const AZ::Vector3& worldPosition = m_worldTM.GetTranslation();
        SetWorldTranslation(AZ::Vector3(x, worldPosition.GetY(), worldPosition.GetZ()));
    }

    void TransformComponent::SetWorldY(float y)
    {
        const AZ::Vector3& worldPosition = m_worldTM.GetTranslation();
        SetWorldTranslation(AZ::Vector3(worldPosition.GetX(), y, worldPosition.GetZ()));
    }

    void TransformComponent::SetWorldZ(float z)
    {
        const AZ::Vector3& worldPosition = m_worldTM.GetTranslation();
        SetWorldTranslation(AZ::Vector3(worldPosition.GetX(), worldPosition.GetY(), z));
    }

    float TransformComponent::GetWorldX()
    {
        return GetWorldTranslation().GetX();
    }

    float TransformComponent::GetWorldY()
    {
        return GetWorldTranslation().GetY();
    }

    float TransformComponent::GetWorldZ()
    {
        return GetWorldTranslation().GetZ();
    }

    void TransformComponent::SetLocalX(float x)
    {
        AZ::Vector3 newLocalTranslation = m_localTM.GetTranslation();
        newLocalTranslation.SetX(x);
        SetLocalTranslation(newLocalTranslation);
    }

    void TransformComponent::SetLocalY(float y)
    {
        AZ::Vector3 newLocalTranslation = m_localTM.GetTranslation();
        newLocalTranslation.SetY(y);
        SetLocalTranslation(newLocalTranslation);
    }

    void TransformComponent::SetLocalZ(float z)
    {
        AZ::Vector3 newLocalTranslation = m_localTM.GetTranslation();
        newLocalTranslation.SetZ(z);
        SetLocalTranslation(newLocalTranslation);
    }

    float TransformComponent::GetLocalX()
    {
        float localX = m_localTM.GetTranslation().GetX();
        return localX;
    }

    float TransformComponent::GetLocalY()
    {
        float localY = m_localTM.GetTranslation().GetY();
        return localY;
    }

    float TransformComponent::GetLocalZ()
    {
        float localZ = m_localTM.GetTranslation().GetZ();
        return localZ;
    }

#if 0
    void TransformComponent::SetRotation(const AZ::Vector3& eulerAnglesRadian)
    {
        AZ_Warning("TransformComponent", false, "SetRotation is deprecated, please use SetLocalRotation");

        AZ::Transform newWorldTransform = m_worldTM;
        newWorldTransform.SetRotationPartFromQuaternion(AZ::ConvertEulerRadiansToQuaternion(eulerAnglesRadian));
        SetWorldTM(newWorldTransform);
    }

    void TransformComponent::SetRotationQuaternion(const AZ::Quaternion& quaternion)
    {
        AZ_Warning("TransformComponent", false, "SetRotationQuaternion is deprecated, please use SetLocalRotationQuaternion");

        AZ::Transform newWorldTransform = m_worldTM;
        newWorldTransform.SetRotationPartFromQuaternion(quaternion);
        SetWorldTM(newWorldTransform);
    }

    void TransformComponent::SetRotationX(float eulerAngleRadian)
    {
        AZ_Warning("TransformComponent", false, "SetRotationX is deprecated, please use SetLocalRotation");

        AZ::Transform newWorldTransform = m_worldTM;
        newWorldTransform.SetRotationPartFromQuaternion(AZ::Quaternion::CreateRotationX(eulerAngleRadian));
        SetWorldTM(newWorldTransform);
    }

    void TransformComponent::SetRotationY(float eulerAngleRadian)
    {
        AZ_Warning("TransformComponent", false, "SetRotationY is deprecated, please use SetLocalRotation");

        AZ::Transform newWorldTransform = m_worldTM;
        newWorldTransform.SetRotationPartFromQuaternion(AZ::Quaternion::CreateRotationY(eulerAngleRadian));
        SetWorldTM(newWorldTransform);
    }

    void TransformComponent::SetRotationZ(float eulerAngleRadian)
    {
        AZ_Warning("TransformComponent", false, "SetRotationZ is deprecated, please use SetLocalRotation");

        AZ::Transform newWorldTransform = m_worldTM;
        newWorldTransform.SetRotationPartFromQuaternion(AZ::Quaternion::CreateRotationZ(eulerAngleRadian));
        SetWorldTM(newWorldTransform);
    }

    void TransformComponent::RotateByX(float eulerAngleRadian)
    {
        AZ_Warning("TransformComponent", false, "RotateByX is deprecated, please use RotateAroundLocalX");

        RotateAroundLocalX(eulerAngleRadian);
    }

    void TransformComponent::RotateByY(float eulerAngleRadian)
    {
        AZ_Warning("TransformComponent", false, "RotateByY is deprecated, please use RotateAroundLocalY");

        RotateAroundLocalY(eulerAngleRadian);
    }

    void TransformComponent::RotateByZ(float eulerAngleRadian)
    {
        AZ_Warning("TransformComponent", false, "RotateByZ is deprecated, please use RotateAroundLocalZ");

        RotateAroundLocalZ(eulerAngleRadian);
    }

    AZ::Vector3 TransformComponent::GetRotationEulerRadians()
    {
        AZ_Warning("TransformComponent", false, "GetRotationEulerRadians is deprecated, please use GetWorldRotation");

        return m_worldTM.GetEulerRadians();
    }

    AZ::Quaternion TransformComponent::GetRotationQuaternion()
    {
        AZ_Warning("TransformComponent", false, "GetRotationQuaternion is deprecated, please use GetWorldRotationQuaternion");

        return AZ::Quaternion::CreateFromTransform(m_worldTM);
    }

    float TransformComponent::GetRotationX()
    {
        AZ_Warning("TransformComponent", false, "GetRotationX is deprecated, please use GetWorldRotation");

        return GetRotationEulerRadians().GetX();
    }

    float TransformComponent::GetRotationY()
    {
        AZ_Warning("TransformComponent", false, "GetRotationY is deprecated, please use GetWorldRotation");

        return GetRotationEulerRadians().GetY();
    }

    float TransformComponent::GetRotationZ()
    {
        AZ_Warning("TransformComponent", false, "GetRotationZ is deprecated, please use GetWorldRotation");

        return GetRotationEulerRadians().GetZ();
    }
#endif
    AZ::Vector3 TransformComponent::GetWorldRotation()
    {
        return m_worldTM.GetRotation().GetEulerRadians();
    }

    AZ::Quaternion TransformComponent::GetWorldRotationQuaternion()
    {
        return m_worldTM.GetRotation();
    }

    void TransformComponent::SetLocalRotation(const AZ::Vector3& eulerRadianAngles)
    {
        AZ::Transform newLocalTM = m_localTM;
        newLocalTM.SetRotation(AZ::Quaternion::CreateFromEulerAnglesRadians(eulerRadianAngles));
        SetLocalTM(newLocalTM);
    }

    void TransformComponent::SetLocalRotationQuaternion(const AZ::Quaternion& quaternion)
    {
        AZ::Transform newLocalTM = m_localTM;
        newLocalTM.SetRotation(quaternion);
        SetLocalTM(newLocalTM);
    }

    static AZ::Transform RotateAroundLocalHelper(float eulerAngleRadian, const AZ::Transform& localTM, AZ::Vector3 axis)
    {
        // normalize the axis before creating rotation
        axis.Normalize();
        AZ::Quaternion rotate = AZ::Quaternion::CreateFromAxisAngle(axis, eulerAngleRadian);

        AZ::Transform newLocalTM = localTM;
        newLocalTM.SetRotation((rotate * localTM.GetRotation()).GetNormalized());
        return newLocalTM;
    }

    void TransformComponent::RotateAroundLocalX(float eulerAngleRadian)
    {
        AZ::Vector3 xAxis = m_localTM.GetBasisX();

        AZ::Transform newLocalTM = RotateAroundLocalHelper(eulerAngleRadian, m_localTM, xAxis);

        SetLocalTM(newLocalTM);
    }

    void TransformComponent::RotateAroundLocalY(float eulerAngleRadian)
    {
        AZ::Vector3 yAxis = m_localTM.GetBasisY();
        
        AZ::Transform newLocalTM = RotateAroundLocalHelper(eulerAngleRadian, m_localTM, yAxis);

        SetLocalTM(newLocalTM);
    }

    void TransformComponent::RotateAroundLocalZ(float eulerAngleRadian)
    {
        AZ::Vector3 zAxis = m_localTM.GetBasisZ();
        
        AZ::Transform newLocalTM = RotateAroundLocalHelper(eulerAngleRadian, m_localTM, zAxis);

        SetLocalTM(newLocalTM);
    }

    AZ::Vector3 TransformComponent::GetLocalRotation()
    {
        return m_localTM.GetRotation().GetEulerRadians();
    }

    AZ::Quaternion TransformComponent::GetLocalRotationQuaternion()
    {
        return m_localTM.GetRotation();
    }

#if 0
    void TransformComponent::SetScale(const AZ::Vector3& scale)
    {
        AZ_Warning("TransformComponent", false, "SetScale is deprecated, please use SetLocalScale");

        AZ::Transform newWorldTransform = m_worldTM;
        AZ::Vector3 prevScale = newWorldTransform.ExtractScale();
        if (!prevScale.IsClose(scale))
        {
            newWorldTransform.MultiplyByScale(scale);
            SetWorldTM(newWorldTransform);
        }
    }

    void TransformComponent::SetScaleX(float scaleX)
    {
        AZ_Warning("TransformComponent", false, "SetScaleX is deprecated, please use SetLocalScaleX");

        AZ::Transform newWorldTransform = m_worldTM;
        AZ::Vector3 scale = newWorldTransform.ExtractScale();
        scale.SetX(scaleX);
        newWorldTransform.MultiplyByScale(scale);
        SetWorldTM(newWorldTransform);
    }

    void TransformComponent::SetScaleY(float scaleY)
    {
        AZ_Warning("TransformComponent", false, "SetScaleY is deprecated, please use SetLocalScaleY");

        AZ::Transform newWorldTransform = m_worldTM;
        AZ::Vector3 scale = newWorldTransform.ExtractScale();
        scale.SetY(scaleY);
        newWorldTransform.MultiplyByScale(scale);
        SetWorldTM(newWorldTransform);
    }

    void TransformComponent::SetScaleZ(float scaleZ)
    {
        AZ_Warning("TransformComponent", false, "SetScaleZ is deprecated, please use SetLocalScaleZ");

        AZ::Transform newWorldTransform = m_worldTM;
        AZ::Vector3 scale = newWorldTransform.ExtractScale();
        scale.SetZ(scaleZ);
        newWorldTransform.MultiplyByScale(scale);
        SetWorldTM(newWorldTransform);
    }

    AZ::Vector3 TransformComponent::GetScale()
    {
        AZ_Warning("TransformComponent", false, "GetScale is deprecated, please use GetLocalScale");

        return m_worldTM.RetrieveScale();
    }

    float TransformComponent::GetScaleX()
    {
        AZ_Warning("TransformComponent", false, "GetScaleX is deprecated, please use GetLocalScale");

        AZ::Vector3 scale = m_worldTM.RetrieveScale();
        return scale.GetX();
    }

    float TransformComponent::GetScaleY()
    {
        AZ_Warning("TransformComponent", false, "GetScaleY is deprecated, please use GetLocalScale");

        AZ::Vector3 scale = m_worldTM.RetrieveScale();
        return scale.GetY();
    }

    float TransformComponent::GetScaleZ()
    {
        AZ_Warning("TransformComponent", false, "GetScaleZ is deprecated, please use GetLocalScale");

        AZ::Vector3 scale = m_worldTM.RetrieveScale();
        return scale.GetZ();
    }

    void TransformComponent::SetLocalScale(const AZ::Vector3& scale)
    {
        AZ::Transform newLocalTM = m_localTM;
        newLocalTM.ExtractScaleExact();
        newLocalTM.MultiplyByScale(scale);
        SetLocalTM(newLocalTM);
    }

    void TransformComponent::SetLocalScaleX(float scaleX)
    {
        AZ::Transform newLocalTM = m_localTM;
        AZ::Vector3 newScale = newLocalTM.ExtractScaleExact();
        newScale.SetX(scaleX);
        newLocalTM.MultiplyByScale(newScale);
        SetLocalTM(newLocalTM);
    }

    void TransformComponent::SetLocalScaleY(float scaleY)
    {
        AZ::Transform newLocalTM = m_localTM;
        AZ::Vector3 newScale = newLocalTM.ExtractScaleExact();
        newScale.SetY(scaleY);
        newLocalTM.MultiplyByScale(newScale);
        SetLocalTM(newLocalTM);
    }

    void TransformComponent::SetLocalScaleZ(float scaleZ)
    {
        AZ::Transform newLocalTM = m_localTM;
        AZ::Vector3 newScale = newLocalTM.ExtractScaleExact();
        newScale.SetZ(scaleZ);
        newLocalTM.MultiplyByScale(newScale);
        SetLocalTM(newLocalTM);
    }

    AZ::Vector3 TransformComponent::GetWorldScale()
    {
        AZ::Vector3 scale = m_worldTM.RetrieveScaleExact();
        return scale;
    }
#endif

    
    AZ::Vector3 TransformComponent::GetLocalScale()
    {
        AZ_WarningOnce("TransformComponent", false, "GetLocalScale is deprecated, please use GetLocalUniformScale instead");
        return AZ::Vector3(m_localTM.GetUniformScale());
    }

    void TransformComponent::SetLocalUniformScale(float scale)
    {
        AZ::Transform newLocalTM = m_localTM;
        newLocalTM.SetUniformScale(scale);
        SetLocalTM(newLocalTM);
    }

    float TransformComponent::GetLocalUniformScale()
    {
        return m_localTM.GetUniformScale();
    }

    float TransformComponent::GetWorldUniformScale()
    {
        return m_worldTM.GetUniformScale();
    }

    AZStd::vector<AZ::EntityId> TransformComponent::GetChildren()
    {
        AZStd::vector<AZ::EntityId> children;
        EBUS_EVENT_ID(GetEntityId(), AZ::TransformHierarchyInformationBus, GatherChildren, children);
        return children;
    }

    AZStd::vector<AZ::EntityId> TransformComponent::GetAllDescendants()
    {
        AZStd::vector<AZ::EntityId> descendants = GetChildren();
        for (size_t i = 0; i < descendants.size(); ++i)
        {
            EBUS_EVENT_ID(descendants[i], AZ::TransformHierarchyInformationBus, GatherChildren, descendants);
        }
        return descendants;
    }

    AZStd::vector<AZ::EntityId> TransformComponent::GetEntityAndAllDescendants()
    {
        AZStd::vector<AZ::EntityId> descendants = { GetEntityId() };
        for (size_t i = 0; i < descendants.size(); ++i)
        {
            EBUS_EVENT_ID(descendants[i], AZ::TransformHierarchyInformationBus, GatherChildren, descendants);
        }
        return descendants;
    }

    bool TransformComponent::IsStaticTransform()
    {
        return m_isStatic;
    }

    void TransformComponent::OnTransformChanged(const AZ::Transform& parentLocalTM, const AZ::Transform& parentWorldTM)
    {
        OnTransformChangedImpl(parentLocalTM, parentWorldTM);
    }

    void TransformComponent::GatherChildren(AZStd::vector<AZ::EntityId>& children)
    {
        children.push_back(GetEntityId());
    }

    void TransformComponent::OnEntityActivated(const AZ::EntityId& parentEntityId)
    {
        OnEntityActivatedImpl(parentEntityId);
        UpdateReplicaChunk();
    }

    void TransformComponent::OnEntityDeactivated(const AZ::EntityId& parentEntityId)
    {
        if (!IsNetworkControlled())
        {
            OnEntityDeactivateImpl(parentEntityId);
            UpdateReplicaChunk();
        }
        else
        {
            // If this transform is network controlled, then the localTM is updated by the network,
            // so update m_parentTM and compute worldTM instead.
            AZ_Assert(parentEntityId == m_parentId, "We expect to receive notifications only from the current parent!");
            m_parentTM = nullptr;
            m_parentActive = false;
            ComputeWorldTM();
        }
    }

    GridMate::ReplicaChunkPtr TransformComponent::GetNetworkBinding()
    {
        TransformReplicaChunk* replicaChunk = GridMate::CreateReplicaChunk<TransformReplicaChunk>();
        replicaChunk->SetHandler(this);
        m_replicaChunk = replicaChunk;

        UpdateReplicaChunk();

        return m_replicaChunk;
    }

    void TransformComponent::SetNetworkBinding(GridMate::ReplicaChunkPtr replicaChunk)
    {
        AZ_Assert(m_replicaChunk == nullptr, "Being bound to two ReplicaChunks");

        bool isTransformChunk = replicaChunk != nullptr;

        AZ_Assert(isTransformChunk, "Being bound to invalid chunk type");
        if (isTransformChunk)
        {
            replicaChunk->SetHandler(this);
            m_replicaChunk = replicaChunk;

            TransformReplicaChunk* transformReplicaChunk = static_cast<TransformReplicaChunk*>(m_replicaChunk.get());

            m_parentId = AZ::EntityId(transformReplicaChunk->m_parentId.Get());

            m_worldTM = transformReplicaChunk->m_initialWorldTM;
            m_localTM = transformReplicaChunk->GetLocalTransform();

            CreateSamples();

            m_netTargetTranslation->SetPreviousValue(
                transformReplicaChunk->m_localTranslation.Get(),
                transformReplicaChunk->m_localTranslation.GetLastUpdateTime());
            m_netTargetTranslation->SetNewTarget(
                transformReplicaChunk->m_localTranslation.Get(),
                transformReplicaChunk->m_localTranslation.GetLastUpdateTime());
            m_netTargetRotation->SetPreviousValue(
                transformReplicaChunk->m_localRotation.Get(),
                transformReplicaChunk->m_localRotation.GetLastUpdateTime());
            m_netTargetRotation->SetNewTarget(
                transformReplicaChunk->m_localRotation.Get(),
                transformReplicaChunk->m_localRotation.GetLastUpdateTime());
            m_netTargetScale = transformReplicaChunk->m_localScale.Get();

            if (HasAnyInterpolation())
            {
                // only connect if interpolation was selected for either position or rotation
                AZ::TickBus::Handler::BusConnect();
            }
        }

        m_onNewParentKeepWorldTM = false;
    }

    void TransformComponent::UnbindFromNetwork()
    {
        if (HasAnyInterpolation())
        {
            AZ::TickBus::Handler::BusDisconnect();
        }

        if (m_replicaChunk)
        {
            m_replicaChunk->SetHandler(nullptr);
            m_replicaChunk = nullptr;
        }
    }

    void TransformComponent::OnNewNetTransformData(const AZ::Transform& transform, const GridMate::TimeContext& /*tc*/)
    {
        SetLocalTMImpl(transform);
    }

    void TransformComponent::OnNewNetParentData(const AZ::u64& parentId, const GridMate::TimeContext& /*tc*/)
    {
        SetParentImpl(AZ::EntityId(parentId), false);
    }

    bool TransformComponent::IsNetworkControlled() const
    {
#if defined(CARBONATED)
        if (m_isClientSimulated)
            return false;
#endif
        return m_replicaChunk && m_replicaChunk->GetReplica() && !m_replicaChunk->IsMaster();
    }

#if defined(CARBONATED)
    //Ignore network updates... currently
    void TransformComponent::SetClientSimulated(bool clientSim)
    {
        m_isClientSimulated = clientSim;
    }
#endif

    bool TransformComponent::IsPositionInterpolated()
    {
        return m_interpolatePosition != AZ::InterpolationMode::NoInterpolation;
    }

    bool TransformComponent::IsRotationInterpolated()
    {
        return m_interpolateRotation != AZ::InterpolationMode::NoInterpolation;
    }

    bool TransformComponent::HasAnyInterpolation()
    {
        return IsPositionInterpolated() || IsRotationInterpolated();
    }

    void TransformComponent::OnTick(float /*deltaTime*/, AZ::ScriptTimePoint /*currentTime*/)
    {
        if (GetEntity() && GetEntity()->GetState() == AZ::Entity::State::Active)
        {
#if defined(CARBONATED)
            if (IsNetworkControlled())
#else
            if (m_replicaChunk && m_replicaChunk->IsProxy())
#endif
            {
                const unsigned int localTime = m_replicaChunk->GetReplicaManager()->GetTime().m_localTime;
                const AZ::Transform newXform = GetInterpolatedTransform(localTime);
                SetLocalTMImpl(newXform);
            }
        }
    }

    AZ::Transform TransformComponent::GetInterpolatedTransform(unsigned localTime)
    {
        const AZ::Vector3 newTranslation = m_netTargetTranslation->GetInterpolatedValue(localTime);
        const AZ::Quaternion newRotation = m_netTargetRotation->GetInterpolatedValue(localTime);
        AZ::Transform newXform = AZ::Transform::CreateFromQuaternionAndTranslation(newRotation, newTranslation);
        // Gruber patch begin // FIXME // AVK -- missed MultiplyByScale
        AZ_Assert(false, "Must be fixed");
#if 0
        newXform.MultiplyByScale(m_netTargetScale);
#endif
        // Gruber patch end // FIXME // AVK
#if defined(CARBONATED)
        AZ_Assert(newXform.IsFinite(), "Interpolated Transform is invalid");
#endif
        return newXform;
    }

    void TransformComponent::OnNewPositionData(const AZ::Vector3& translation, const GridMate::TimeContext& tc)
    {
#if defined(CARBONATED)
        AZ_Assert(translation.IsFinite(), "TransformComponent:OnNewPositionData translation is invalid");
#endif
        m_netTargetTranslation->SetNewTarget(translation, tc.m_realTime);
        if (!HasAnyInterpolation())
        {
            unsigned int localTime = m_replicaChunk->GetReplicaManager()->GetTime().m_localTime;
            AZ::Transform newXform = GetInterpolatedTransform(localTime);
            SetLocalTMImpl(newXform);
        }
    };

    void TransformComponent::OnNewRotationData(const AZ::Quaternion& rotation, const GridMate::TimeContext& tc)
    {
        m_netTargetRotation->SetNewTarget(rotation, tc.m_realTime);
        if (!HasAnyInterpolation())
        {
            unsigned int localTime = m_replicaChunk->GetReplicaManager()->GetTime().m_localTime;
            AZ::Transform newXform = GetInterpolatedTransform(localTime);
            SetLocalTMImpl(newXform);
        }
    }

    void TransformComponent::OnNewScaleData(const AZ::Vector3& scale, const GridMate::TimeContext& /*tc*/)
    {
        // no interpolation of scale by design, very unlikely somebody needs it
        m_netTargetScale = scale;
    }

    void TransformComponent::UpdateReplicaChunk()
    {
        if (!IsNetworkControlled() && m_replicaChunk)
        {
            TransformReplicaChunk* transformReplicaChunk = static_cast<TransformReplicaChunk*>(m_replicaChunk.get());
            transformReplicaChunk->SetInitialTM(GetWorldTM());
            transformReplicaChunk->SetLocalTM(GetLocalTM());
            transformReplicaChunk->m_parentId.Set(static_cast<AZ::u64>(GetParentId()));
        }
    }

    void TransformComponent::SetParentImpl(AZ::EntityId parentId, bool isKeepWorldTM)
    {
        if (parentId == GetEntityId())
        {
            AZ_Warning("TransformComponent", false, "An entity can not be set as its own parent.");
            return;
        }

        AZ::EntityId oldParent = m_parentId;
        if (m_parentId.IsValid())
        {
            AZ::TransformNotificationBus::Handler::BusDisconnect();
            AZ::TransformHierarchyInformationBus::Handler::BusDisconnect();
            AZ::EntityBus::Handler::BusDisconnect();
            m_parentActive = false;
        }

        m_parentId = parentId;
        if (m_parentId.IsValid())
        {
            AZ::Entity* parentEntity = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(parentEntity, &AZ::ComponentApplicationBus::Events::FindEntity, m_parentId);
            m_parentActive = parentEntity && (parentEntity->GetState() == AZ::Entity::State::Active);

            m_onNewParentKeepWorldTM = isKeepWorldTM;

            AZ::TransformNotificationBus::Handler::BusConnect(m_parentId);
            AZ::TransformHierarchyInformationBus::Handler::BusConnect(m_parentId);
            AZ::EntityBus::Handler::BusConnect(m_parentId);
        }
        else
        {
            m_parentTM = nullptr;

            if (isKeepWorldTM)
            {
            SetWorldTM(m_worldTM);
            }
            else
            {
                SetLocalTM(m_localTM);
            }

            if (oldParent.IsValid())
            {
                EBUS_EVENT_PTR(m_notificationBus, AZ::TransformNotificationBus, OnTransformChanged, m_localTM, m_worldTM);
            }
        }

        EBUS_EVENT_PTR(m_notificationBus, AZ::TransformNotificationBus, OnParentChanged, oldParent, parentId);

        if (oldParent != parentId) // Don't send removal notification while activating.
        {
            EBUS_EVENT_ID(oldParent, AZ::TransformNotificationBus, OnChildRemoved, GetEntityId());
        }

        EBUS_EVENT_ID(parentId, AZ::TransformNotificationBus, OnChildAdded, GetEntityId());
    }

    void TransformComponent::SetLocalTMImpl(const AZ::Transform& tm)
    {
        m_localTM = tm;
        ComputeWorldTM();  // We can user dirty flags and compute it later on demand
    }

    void TransformComponent::SetWorldTMImpl(const AZ::Transform& tm)
    {
        m_worldTM = tm;
        ComputeLocalTM(); // We can user dirty flags and compute it later on demand
    }

    void TransformComponent::OnTransformChangedImpl(const AZ::Transform& /*parentLocalTM*/, const AZ::Transform& parentWorldTM)
    {
        // Called when our parent transform changes
        // Ignore the event until we've already derived our local transform.
        if (m_parentTM)
        {
            m_worldTM = parentWorldTM * m_localTM;
            EBUS_EVENT_PTR(m_notificationBus, AZ::TransformNotificationBus, OnTransformChanged, m_localTM, m_worldTM);
        }
    }

    void TransformComponent::OnEntityActivatedImpl(const AZ::EntityId& parentEntityId)
    {
        AZ_Assert(parentEntityId == m_parentId, "We expect to receive notifications only from the current parent!");

        m_parentActive = true;

#ifndef _RELEASE
        AZ::EntityId parentId = m_parentId;

        while (parentId.IsValid())
        {
            if (parentId == GetEntityId())
            {
                AZ_Error("TransformComponent", false, "Trying to create a circular dependency of parenting. Aborting set parent call.");
                SetParent(AZ::EntityId());
                return;
            }

            auto handler = AZ::TransformBus::FindFirstHandler(parentId);

            if (handler == nullptr)
            {
                break;
            }

            parentId = handler->GetParentId();
        }
#endif

        AZ::Entity* parentEntity = nullptr;
        EBUS_EVENT_RESULT(parentEntity, AZ::ComponentApplicationBus, FindEntity, parentEntityId);
        AZ_Assert(parentEntity, "We expect to have a parent entity associated with the provided parent's entity Id.");
        if (parentEntity)
        {
            m_parentTM = parentEntity->GetTransform();

#if defined(CARBONATED)
           //Warning keeps going off for statics in layers... so its just noise.
#else
           AZ_Warning("TransformComponent", !m_isStatic || m_parentTM->IsStaticTransform(),
                "Entity '%s' %s has static transform, but parent has non-static transform. This may lead to unexpected movement.",
                GetEntity()->GetName().c_str(), GetEntityId().ToString().c_str());
#endif

            if (m_onNewParentKeepWorldTM)
            {
                ComputeLocalTM();
            }
            else
            {
                ComputeWorldTM();
            }
        }
    }

    void TransformComponent::OnEntityDeactivateImpl(const AZ::EntityId& parentEntityId)
    {
        (void)parentEntityId;
        AZ_Assert(parentEntityId == m_parentId, "We expect to receive notifications only from the current parent!");
        m_parentTM = nullptr;
        m_parentActive = false;
        ComputeLocalTM();
    }

    void TransformComponent::ComputeLocalTM()
    {
        if (m_parentTM)
        {
            m_localTM = m_parentTM->GetWorldTM().GetInverse() * m_worldTM;
        }
        else if (!m_parentActive)
        {
            m_localTM = m_worldTM;
        }

        AZ::TransformNotificationBus::Event(
            m_notificationBus, &AZ::TransformNotificationBus::Events::OnTransformChanged, m_localTM, m_worldTM);
        m_transformChangedEvent.Signal(m_localTM, m_worldTM);

        AzFramework::IEntityBoundsUnion* boundsUnion = AZ::Interface<AzFramework::IEntityBoundsUnion>::Get();
        if (boundsUnion != nullptr)
        {
            boundsUnion->OnTransformUpdated(GetEntity());
        }
    }

    void TransformComponent::ComputeWorldTM()
    {
        if (m_parentTM)
        {
            m_worldTM = m_parentTM->GetWorldTM() * m_localTM;
        }
        else if (!m_parentActive)
        {
            m_worldTM = m_localTM;
        }

        EBUS_EVENT_PTR(m_notificationBus, AZ::TransformNotificationBus, OnTransformChanged, m_localTM, m_worldTM);
    }

    bool TransformComponent::AreMoveRequestsAllowed() const
    {
        if (IsNetworkControlled())
        {
            return false;
        }

        // Don't allow static transform to be moved while entity is activated.
        // But do allow a static transform to be moved when the entity is deactivated.
        if (m_isStatic && m_entity && (m_entity->GetState() > AZ::Entity::State::Init))
        {
            return false;
        }

        return true;
    }

    void TransformComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("TransformService", 0x8ee22c50));
    }

    void TransformComponent::Reflect(AZ::ReflectContext* reflection)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
        if (serializeContext)
        {
            serializeContext->Class<TransformComponent, AZ::Component, NetBindable>()
                ->Version(4, &TransformComponentVersionConverter)
                ->Field("Parent", &TransformComponent::m_parentId)
                ->Field("Transform", &TransformComponent::m_worldTM)
                ->Field("LocalTransform", &TransformComponent::m_localTM)
                ->Field("ParentActivationTransformMode", &TransformComponent::m_parentActivationTransformMode)
                ->Field("IsStatic", &TransformComponent::m_isStatic)
                ->Field("InterpolatePosition", &TransformComponent::m_interpolatePosition)
                ->Field("InterpolateRotation", &TransformComponent::m_interpolateRotation)
                ;
        }

        AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(reflection);
        if(behaviorContext)
        {
            behaviorContext->EBus<AZ::TransformNotificationBus>("TransformNotificationBus")->
                Handler<AZ::BehaviorTransformNotificationBusHandler>();

            behaviorContext->Class<TransformComponent>()->RequestBus("TransformBus");

            behaviorContext->EBus<AZ::TransformBus>("TransformBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Module, "components")
                ->Event("GetLocalTM", &AZ::TransformBus::Events::GetLocalTM)
                ->Event("GetWorldTM", &AZ::TransformBus::Events::GetWorldTM)
                ->Event("GetParentId", &AZ::TransformBus::Events::GetParentId)
                ->Event("GetLocalAndWorld", &AZ::TransformBus::Events::GetLocalAndWorld)
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Event("SetLocalTM", &AZ::TransformBus::Events::SetLocalTM)
                ->Event("SetWorldTM", &AZ::TransformBus::Events::SetWorldTM)
                ->Event("SetParent", &AZ::TransformBus::Events::SetParent)
                ->Event("SetParentRelative", &AZ::TransformBus::Events::SetParentRelative)
                ->Event("SetWorldTranslation", &AZ::TransformBus::Events::SetWorldTranslation)
                ->Event("SetLocalTranslation", &AZ::TransformBus::Events::SetLocalTranslation)
                ->Event("GetWorldTranslation", &AZ::TransformBus::Events::GetWorldTranslation)
                ->Event("GetLocalTranslation", &AZ::TransformBus::Events::GetLocalTranslation)
                    ->Attribute("Position", AZ::Edit::Attributes::PropertyPosition)
                ->VirtualProperty("Position", "GetLocalTranslation", "SetLocalTranslation")
                ->Event("MoveEntity", &AZ::TransformBus::Events::MoveEntity)
                ->Event("SetWorldX", &AZ::TransformBus::Events::SetWorldX)
                ->Event("SetWorldY", &AZ::TransformBus::Events::SetWorldY)
                ->Event("SetWorldZ", &AZ::TransformBus::Events::SetWorldZ)
                ->Event("GetWorldX", &AZ::TransformBus::Events::GetWorldX)
                ->Event("GetWorldY", &AZ::TransformBus::Events::GetWorldY)
                ->Event("GetWorldZ", &AZ::TransformBus::Events::GetWorldZ)
                ->Event("SetLocalX", &AZ::TransformBus::Events::SetLocalX)
                ->Event("SetLocalY", &AZ::TransformBus::Events::SetLocalY)
                ->Event("SetLocalZ", &AZ::TransformBus::Events::SetLocalZ)
                ->Event("GetLocalX", &AZ::TransformBus::Events::GetLocalX)
                ->Event("GetLocalY", &AZ::TransformBus::Events::GetLocalY)
                ->Event("GetLocalZ", &AZ::TransformBus::Events::GetLocalZ)
#if 0
                ->Event("RotateByX", &AZ::TransformBus::Events::RotateByX)
                    ->Attribute(AZ::Script::Attributes::Deprecated, true)
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Event("RotateByY", &AZ::TransformBus::Events::RotateByY)
                    ->Attribute(AZ::Script::Attributes::Deprecated, true)
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Event("RotateByZ", &AZ::TransformBus::Events::RotateByZ)
                    ->Attribute(AZ::Script::Attributes::Deprecated, true)
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Event("SetEulerRotation", &AZ::TransformBus::Events::SetRotation)
                    ->Attribute(AZ::Script::Attributes::Deprecated, true)
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Event("SetRotationQuaternion", &AZ::TransformBus::Events::SetRotationQuaternion)
                    ->Attribute(AZ::Script::Attributes::Deprecated, true)
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Event("SetRotationX", &AZ::TransformBus::Events::SetRotationX)
                    ->Attribute(AZ::Script::Attributes::Deprecated, true)
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Event("SetRotationY", &AZ::TransformBus::Events::SetRotationY)
                    ->Attribute(AZ::Script::Attributes::Deprecated, true)
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Event("SetRotationZ", &AZ::TransformBus::Events::SetRotationZ)
                    ->Attribute(AZ::Script::Attributes::Deprecated, true)
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Event("GetEulerRotation", &AZ::TransformBus::Events::GetRotationEulerRadians)
                    ->Attribute(AZ::Script::Attributes::Deprecated, true)
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Event("GetRotationQuaternion", &AZ::TransformBus::Events::GetRotationQuaternion)
                    ->Attribute(AZ::Script::Attributes::Deprecated, true)
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Event("GetRotationX", &AZ::TransformBus::Events::GetRotationX)
                    ->Attribute(AZ::Script::Attributes::Deprecated, true)
                   ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Event("GetRotationY", &AZ::TransformBus::Events::GetRotationY)
                    ->Attribute(AZ::Script::Attributes::Deprecated, true)
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Event("GetRotationZ", &AZ::TransformBus::Events::GetRotationZ)
                    ->Attribute(AZ::Script::Attributes::Deprecated, true)
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
#endif
                ->Event("GetWorldRotation", &AZ::TransformBus::Events::GetWorldRotation)
                ->Event("GetWorldRotationQuaternion", &AZ::TransformBus::Events::GetWorldRotationQuaternion)
                ->Event("SetLocalRotation", &AZ::TransformBus::Events::SetLocalRotation)
                ->Event("SetLocalRotationQuaternion", &AZ::TransformBus::Events::SetLocalRotationQuaternion)
                ->Event("RotateAroundLocalX", &AZ::TransformBus::Events::RotateAroundLocalX)
                ->Event("RotateAroundLocalY", &AZ::TransformBus::Events::RotateAroundLocalY)
                ->Event("RotateAroundLocalZ", &AZ::TransformBus::Events::RotateAroundLocalZ)
                ->Event("GetLocalRotation", &AZ::TransformBus::Events::GetLocalRotation)
                ->Event("GetLocalRotationQuaternion", &AZ::TransformBus::Events::GetLocalRotationQuaternion)
                    ->Attribute("Rotation", AZ::Edit::Attributes::PropertyRotation)
                ->VirtualProperty("Rotation", "GetLocalRotationQuaternion", "SetLocalRotationQuaternion")
#if 0
                ->Event("SetScale", &AZ::TransformBus::Events::SetScale)
                    ->Attribute(AZ::Script::Attributes::Deprecated, true)
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Event("SetScaleX", &AZ::TransformBus::Events::SetScaleX)
                    ->Attribute(AZ::Script::Attributes::Deprecated, true)
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Event("SetScaleY", &AZ::TransformBus::Events::SetScaleY)
                    ->Attribute(AZ::Script::Attributes::Deprecated, true)
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Event("SetScaleZ", &AZ::TransformBus::Events::SetScaleZ)
                    ->Attribute(AZ::Script::Attributes::Deprecated, true)
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Event("GetScale", &AZ::TransformBus::Events::GetScale)
                    ->Attribute(AZ::Script::Attributes::Deprecated, true)
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Event("GetScaleX", &AZ::TransformBus::Events::GetScaleX)
                    ->Attribute(AZ::Script::Attributes::Deprecated, true)
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Event("GetScaleY", &AZ::TransformBus::Events::GetScaleY)
                    ->Attribute(AZ::Script::Attributes::Deprecated, true)
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Event("GetScaleZ", &AZ::TransformBus::Events::GetScaleZ)
                    ->Attribute(AZ::Script::Attributes::Deprecated, true)
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Event("SetLocalScale", &AZ::TransformBus::Events::SetLocalScale)
                ->Event("SetLocalScaleX", &AZ::TransformBus::Events::SetLocalScaleX)
                ->Event("SetLocalScaleY", &AZ::TransformBus::Events::SetLocalScaleY)
                ->Event("SetLocalScaleZ", &AZ::TransformBus::Events::SetLocalScaleZ)
#endif
                ->Event("GetLocalScale", &AZ::TransformBus::Events::GetLocalScale)
                    ->Attribute("Scale", AZ::Edit::Attributes::PropertyScale)
                ->VirtualProperty("Scale", "GetLocalScale", "SetLocalScale")
#if 0
                ->Event("GetWorldScale", &AZ::TransformBus::Events::GetWorldScale)
#endif
                ->Event("GetChildren", &AZ::TransformBus::Events::GetChildren)
                ->Event("GetAllDescendants", &AZ::TransformBus::Events::GetAllDescendants)
                ->Event("GetEntityAndAllDescendants", &AZ::TransformBus::Events::GetEntityAndAllDescendants)
                ->Event("IsStaticTransform", &AZ::TransformBus::Events::IsStaticTransform)
                ;

            behaviorContext->Constant("TransformComponentTypeId", BehaviorConstant(AZ::TransformComponentTypeId));
            behaviorContext->Constant("EditorTransformComponentTypeId", BehaviorConstant(AZ::EditorTransformComponentTypeId));

            behaviorContext->Class<AZ::TransformConfig>()
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Unused)
                ->Attribute(AZ::Script::Attributes::ConstructorOverride, &AZ::TransformConfigConstructor)
                ->Enum<(int)AZ::TransformConfig::ParentActivationTransformMode::MaintainOriginalRelativeTransform>("MaintainOriginalRelativeTransform")
                ->Enum<(int)AZ::TransformConfig::ParentActivationTransformMode::MaintainCurrentWorldTransform>("MaintainCurrentWorldTransform")
                ->Constructor()
                ->Constructor<const AZ::Transform&>()
                ->Method("SetTransform", &AZ::TransformConfig::SetTransform)
                    ->Attribute(AZ::Script::Attributes::Deprecated, true)
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Method("SetLocalAndWorldTransform", &AZ::TransformConfig::SetLocalAndWorldTransform)
                    ->Attribute(AZ::Script::Attributes::Deprecated, true)
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Property("worldTransform", BehaviorValueProperty(&AZ::TransformConfig::m_worldTransform))
                ->Property("localTransform", BehaviorValueProperty(&AZ::TransformConfig::m_localTransform))
                ->Property("parentId", BehaviorValueProperty(&AZ::TransformConfig::m_parentId))
                ->Property("parentActivationTransformMode",
                    [](AZ::TransformConfig* config) { return (int&)(config->m_parentActivationTransformMode); },
                    [](AZ::TransformConfig* config, const int& i) { config->m_parentActivationTransformMode = (AZ::TransformConfig::ParentActivationTransformMode)i; })
                ->Property("netSyncEnabled", BehaviorValueProperty(&AZ::TransformConfig::m_netSyncEnabled))
                ->Property("interpolatePosition",
                    [](AZ::TransformConfig* config) { return (int&)(config->m_interpolatePosition); },
                    [](AZ::TransformConfig* config, const int& i) { config->m_interpolatePosition = (AZ::InterpolationMode)i; })
                ->Property("interpolateRotation",
                    [](AZ::TransformConfig* config) { return (int&)(config->m_interpolateRotation); },
                    [](AZ::TransformConfig* config, const int& i) { config->m_interpolateRotation = (AZ::InterpolationMode)i; })
                ->Property("isStatic", BehaviorValueProperty(&AZ::TransformConfig::m_isStatic))
                ;
        }

        NetworkContext* netContext = azrtti_cast<NetworkContext*>(reflection);
        if (netContext)
        {
            netContext->Class<TransformComponent>()
                ->Chunk<TransformReplicaChunk, TransformReplicaChunk::Descriptor>()
                    ->Field("ParentId", &TransformReplicaChunk::m_parentId)
                    ->Field("LocalTranslationData", &TransformReplicaChunk::m_localTranslation)
                    ->Field("LocalRotationData", &TransformReplicaChunk::m_localRotation)
                    ->Field("LocalScaleData", &TransformReplicaChunk::m_localScale);
        }
    }
} // namespace AZ

#else
#include <AzFramework/Components/TransformComponent.h>
#include <AzFramework/Visibility/EntityBoundsUnionBus.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Quaternion.h>

namespace AZ
{
    class BehaviorTransformNotificationBusHandler
        : public TransformNotificationBus::Handler
        , public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER
        (
            BehaviorTransformNotificationBusHandler,
            "{9CEF4DAB-F359-4A3E-9856-7780281E0DAA}",
            AZ::SystemAllocator, 
            OnTransformChanged, 
            OnParentChanged, 
            OnChildAdded, 
            OnChildRemoved
        );

        void OnTransformChanged(const Transform& localTM, const Transform& worldTM) override
        {
            Call(FN_OnTransformChanged, localTM, worldTM);
        }

        void OnParentChanged(EntityId oldParent, EntityId newParent) override
        {
            Call(FN_OnParentChanged, oldParent, newParent);
        }

        void OnChildAdded(EntityId child) override
        {
            Call(FN_OnChildAdded, child);
        }

        void OnChildRemoved(EntityId child) override
        {
            Call(FN_OnChildRemoved, child);
        }
    };

    void TransformConfigConstructor(TransformConfig* self, ScriptDataContext& dc)
    {
        const int numArgs = dc.GetNumArguments();
        if (numArgs == 0)
        {
            new(self) TransformConfig();
        }
        else if (numArgs == 1 && dc.IsClass<Transform>(0))
        {
            Transform transform;
            dc.ReadArg(0, transform);

            new(self) TransformConfig(transform);
        }
        else
        {
            dc.GetScriptContext()->Error(AZ::ScriptContext::ErrorType::Error, true, "Invalid constructor call. Valid overrides are: TransformConfig(), TransfomConfig(Transform)");

            new(self) TransformConfig();
        }
    }
} // namespace AZ

namespace AzFramework
{
    bool TransformComponentVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
    {
        if (classElement.GetVersion() < 3)
        {
            // IsStatic field added at v3.
            // It should be false for old versions of TransformComponent.
            classElement.AddElementWithData(context, "IsStatic", false);
        }

        // note the == on the following line.  Do not add to this block.  If you add an "InterpolateScale" back in, then
        // consider erasing this block.
        // The version was bumped from 3 to 4 to ensure this code runs.
        // if you add the field back in, then increment the version number again.
        if (classElement.GetVersion() == 3)
        {
            // a field was temporarily added to this specific version, then was removed.
            // However, some data may have been exported with this field present, so 
            // remove it if its found, but only in this version which the change was present in, so that
            // future re-additions of it won't remove it (as long as they bump the version number.)
            classElement.RemoveElementByName(AZ_CRC("InterpolateScale", 0x9d00b831));
        }

        return true;
    }

    TransformComponent::TransformComponent(const TransformComponent& copy)
        : m_localTM(copy.m_localTM)
        , m_worldTM(copy.m_worldTM)
        , m_parentId(copy.m_parentId)
        , m_parentTM(copy.m_parentTM)
        , m_parentActive(copy.m_parentActive)
        , m_notificationBus(nullptr)
        , m_onNewParentKeepWorldTM(copy.m_onNewParentKeepWorldTM)
        , m_parentActivationTransformMode(copy.m_parentActivationTransformMode)
        , m_isStatic(copy.m_isStatic)
    {
        ;
    }

    bool TransformComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (auto config = azrtti_cast<const AZ::TransformConfig*>(baseConfig))
        {
            m_localTM = config->m_localTransform;
            m_worldTM = config->m_worldTransform;
            m_parentId = config->m_parentId;
            m_parentActivationTransformMode = config->m_parentActivationTransformMode;
            m_isStatic = config->m_isStatic;
            return true;
        }
        return false;
    }

    bool TransformComponent::WriteOutConfig(AZ::ComponentConfig* baseConfig) const
    {
        if (auto config = azrtti_cast<AZ::TransformConfig*>(baseConfig))
        {
            config->m_localTransform = m_localTM;
            config->m_worldTransform = m_worldTM;
            config->m_parentId = m_parentId;
            config->m_parentActivationTransformMode = m_parentActivationTransformMode;
            config->m_isStatic = m_isStatic;
            return true;
        }
        return false;
    }

    void TransformComponent::Activate()
    {
        AZ::TransformBus::Handler::BusConnect(m_entity->GetId());
        AZ::TransformNotificationBus::Bind(m_notificationBus, m_entity->GetId());

        const bool keepWorldTm = (m_parentActivationTransformMode == ParentActivationTransformMode::MaintainCurrentWorldTransform || !m_parentId.IsValid());
        SetParentImpl(m_parentId, keepWorldTm);
    }

    void TransformComponent::Deactivate()
    {
        AZ::TransformNotificationBus::Event(m_parentId, &AZ::TransformNotificationBus::Events::OnChildRemoved, GetEntityId());
        auto parentTransform = AZ::TransformBus::FindFirstHandler(m_parentId);
        if (parentTransform)
        {
            parentTransform->NotifyChildChangedEvent(AZ::ChildChangeType::Removed, GetEntityId());
        }

        m_notificationBus = nullptr;
        if (m_parentId.IsValid())
        {
            AZ::TransformNotificationBus::Handler::BusDisconnect();
            AZ::TransformHierarchyInformationBus::Handler::BusDisconnect();
            AZ::EntityBus::Handler::BusDisconnect();
        }
        AZ::TransformBus::Handler::BusDisconnect();
    }

    void TransformComponent::BindTransformChangedEventHandler(AZ::TransformChangedEvent::Handler& handler)
    {
        handler.Connect(m_transformChangedEvent);
    }

    void TransformComponent::BindParentChangedEventHandler(AZ::ParentChangedEvent::Handler& handler)
    {
        handler.Connect(m_parentChangedEvent);
    }

    void TransformComponent::BindChildChangedEventHandler(AZ::ChildChangedEvent::Handler& handler)
    {
        handler.Connect(m_childChangedEvent);
    }

    void TransformComponent::NotifyChildChangedEvent(AZ::ChildChangeType changeType, AZ::EntityId entityId)
    {
        m_childChangedEvent.Signal(changeType, entityId);
    }

    void TransformComponent::SetLocalTM(const AZ::Transform& tm)
    {
        if (AreMoveRequestsAllowed())
        {
            SetLocalTMImpl(tm);
        }
    }

    void TransformComponent::SetWorldTM(const AZ::Transform& tm)
    {
        if (AreMoveRequestsAllowed())
        {
            SetWorldTMImpl(tm);
        }
    }

    void TransformComponent::SetParent(AZ::EntityId id)
    {
        SetParentImpl(id, true);
    }

    void TransformComponent::SetParentRelative(AZ::EntityId id)
    {
        SetParentImpl(id, m_isStatic);
    }

    void TransformComponent::SetWorldTranslation(const AZ::Vector3& newPosition)
    {
        AZ::Transform newWorldTransform = m_worldTM;
        newWorldTransform.SetTranslation(newPosition);
        SetWorldTM(newWorldTransform);
    }

    void TransformComponent::SetLocalTranslation(const AZ::Vector3& newPosition)
    {
        AZ::Transform newLocalTransform = m_localTM;
        newLocalTransform.SetTranslation(newPosition);
        SetLocalTM(newLocalTransform);
    }

    AZ::Vector3 TransformComponent::GetWorldTranslation()
    {
        return m_worldTM.GetTranslation();
    }

    AZ::Vector3 TransformComponent::GetLocalTranslation()
    {
        return m_localTM.GetTranslation();
    }

    void TransformComponent::MoveEntity(const AZ::Vector3& offset)
    {
        const AZ::Vector3& worldPosition = m_worldTM.GetTranslation();
        SetWorldTranslation(worldPosition + offset);
    }

    void TransformComponent::SetWorldX(float x)
    {
        const AZ::Vector3& worldPosition = m_worldTM.GetTranslation();
        SetWorldTranslation(AZ::Vector3(x, worldPosition.GetY(), worldPosition.GetZ()));
    }

    void TransformComponent::SetWorldY(float y)
    {
        const AZ::Vector3& worldPosition = m_worldTM.GetTranslation();
        SetWorldTranslation(AZ::Vector3(worldPosition.GetX(), y, worldPosition.GetZ()));
    }

    void TransformComponent::SetWorldZ(float z)
    {
        const AZ::Vector3& worldPosition = m_worldTM.GetTranslation();
        SetWorldTranslation(AZ::Vector3(worldPosition.GetX(), worldPosition.GetY(), z));
    }

    float TransformComponent::GetWorldX()
    {
        return GetWorldTranslation().GetX();
    }

    float TransformComponent::GetWorldY()
    {
        return GetWorldTranslation().GetY();
    }

    float TransformComponent::GetWorldZ()
    {
        return GetWorldTranslation().GetZ();
    }

    void TransformComponent::SetLocalX(float x)
    {
        AZ::Vector3 newLocalTranslation = m_localTM.GetTranslation();
        newLocalTranslation.SetX(x);
        SetLocalTranslation(newLocalTranslation);
    }

    void TransformComponent::SetLocalY(float y)
    {
        AZ::Vector3 newLocalTranslation = m_localTM.GetTranslation();
        newLocalTranslation.SetY(y);
        SetLocalTranslation(newLocalTranslation);
    }

    void TransformComponent::SetLocalZ(float z)
    {
        AZ::Vector3 newLocalTranslation = m_localTM.GetTranslation();
        newLocalTranslation.SetZ(z);
        SetLocalTranslation(newLocalTranslation);
    }

    float TransformComponent::GetLocalX()
    {
        float localX = m_localTM.GetTranslation().GetX();
        return localX;
    }

    float TransformComponent::GetLocalY()
    {
        float localY = m_localTM.GetTranslation().GetY();
        return localY;
    }

    float TransformComponent::GetLocalZ()
    {
        float localZ = m_localTM.GetTranslation().GetZ();
        return localZ;
    }

    void TransformComponent::SetWorldRotation(const AZ::Vector3& eulerAnglesRadian)
    {
        AZ::Transform newWorldTransform = m_worldTM;
        newWorldTransform.SetRotation(AZ::Quaternion::CreateFromEulerAnglesRadians(eulerAnglesRadian));
        SetWorldTM(newWorldTransform);
    }

    void TransformComponent::SetWorldRotationQuaternion(const AZ::Quaternion& quaternion)
    {
        AZ::Transform newWorldTransform = m_worldTM;
        newWorldTransform.SetRotation(quaternion);
        SetWorldTM(newWorldTransform);
    }

    AZ::Vector3 TransformComponent::GetWorldRotation()
    {
        return m_worldTM.GetRotation().GetEulerRadians();
    }

    AZ::Quaternion TransformComponent::GetWorldRotationQuaternion()
    {
        return m_worldTM.GetRotation();
    }

    void TransformComponent::SetLocalRotation(const AZ::Vector3& eulerRadianAngles)
    {
        AZ::Transform newLocalTM = m_localTM;
        newLocalTM.SetRotation(AZ::Quaternion::CreateFromEulerAnglesRadians(eulerRadianAngles));
        SetLocalTM(newLocalTM);
    }

    void TransformComponent::SetLocalRotationQuaternion(const AZ::Quaternion& quaternion)
    {
        AZ::Transform newLocalTM = m_localTM;
        newLocalTM.SetRotation(quaternion);
        SetLocalTM(newLocalTM);
    }

    static AZ::Transform RotateAroundLocalHelper(float eulerAngleRadian, const AZ::Transform& localTM, AZ::Vector3 axis)
    {
        //normalize the axis before creating rotation
        axis.Normalize();
        AZ::Quaternion rotate = AZ::Quaternion::CreateFromAxisAngle(axis, eulerAngleRadian);

        AZ::Transform newLocalTM = localTM;
        newLocalTM.SetRotation((rotate * localTM.GetRotation()).GetNormalized());
        return newLocalTM;
    }

    void TransformComponent::RotateAroundLocalX(float eulerAngleRadian)
    {
        AZ::Vector3 xAxis = m_localTM.GetBasisX();
        
        AZ::Transform newLocalTM = RotateAroundLocalHelper(eulerAngleRadian, m_localTM, xAxis);
        
        SetLocalTM(newLocalTM);
    }

    void TransformComponent::RotateAroundLocalY(float eulerAngleRadian)
    {
        AZ::Vector3 yAxis = m_localTM.GetBasisY();
        
        AZ::Transform newLocalTM = RotateAroundLocalHelper(eulerAngleRadian, m_localTM, yAxis);

        SetLocalTM(newLocalTM);
    }

    void TransformComponent::RotateAroundLocalZ(float eulerAngleRadian)
    {
        AZ::Vector3 zAxis = m_localTM.GetBasisZ();
        
        AZ::Transform newLocalTM = RotateAroundLocalHelper(eulerAngleRadian, m_localTM, zAxis);

        SetLocalTM(newLocalTM);
    }

    AZ::Vector3 TransformComponent::GetLocalRotation()
    {
        return m_localTM.GetRotation().GetEulerRadians();
    }

    AZ::Quaternion TransformComponent::GetLocalRotationQuaternion()
    {
        return m_localTM.GetRotation();
    }

    AZ::Vector3 TransformComponent::GetLocalScale()
    {
        AZ_WarningOnce("TransformComponent", false, "GetLocalScale is deprecated, please use GetLocalUniformScale instead");
        return AZ::Vector3(m_localTM.GetUniformScale());
    }

    void TransformComponent::SetLocalUniformScale(float scale)
    {
        AZ::Transform newLocalTM = m_localTM;
        newLocalTM.SetUniformScale(scale);
        SetLocalTM(newLocalTM);
    }

    float TransformComponent::GetLocalUniformScale()
    {
        return m_localTM.GetUniformScale();
    }

    float TransformComponent::GetWorldUniformScale()
    {
        return m_worldTM.GetUniformScale();
    }

    AZStd::vector<AZ::EntityId> TransformComponent::GetChildren()
    {
        AZStd::vector<AZ::EntityId> children;
        AZ::TransformHierarchyInformationBus::Event(GetEntityId(), &AZ::TransformHierarchyInformationBus::Events::GatherChildren, children);
        return children;
    }

    AZStd::vector<AZ::EntityId> TransformComponent::GetAllDescendants()
    {
        AZStd::vector<AZ::EntityId> descendants = GetChildren();
        for (size_t i = 0; i < descendants.size(); ++i)
        {
            AZ::TransformHierarchyInformationBus::Event(
                descendants[i], &AZ::TransformHierarchyInformationBus::Events::GatherChildren, descendants);
        }
        return descendants;
    }

    AZStd::vector<AZ::EntityId> TransformComponent::GetEntityAndAllDescendants()
    {
        AZStd::vector<AZ::EntityId> descendants = { GetEntityId() };
        for (size_t i = 0; i < descendants.size(); ++i)
        {
            AZ::TransformHierarchyInformationBus::Event(
                descendants[i], &AZ::TransformHierarchyInformationBus::Events::GatherChildren, descendants);
        }
        return descendants;
    }

    bool TransformComponent::IsStaticTransform()
    {
        return m_isStatic;
    }

    AZ::OnParentChangedBehavior TransformComponent::GetOnParentChangedBehavior()
    {
        return m_onParentChangedBehavior;
    }

    void TransformComponent::SetOnParentChangedBehavior(AZ::OnParentChangedBehavior onParentChangedBehavior)
    {
        m_onParentChangedBehavior = onParentChangedBehavior;
    }

    void TransformComponent::OnTransformChanged(const AZ::Transform& parentLocalTM, const AZ::Transform& parentWorldTM)
    {
        OnTransformChangedImpl(parentLocalTM, parentWorldTM);
    }

    void TransformComponent::GatherChildren(AZStd::vector<AZ::EntityId>& children)
    {
        children.push_back(GetEntityId());
    }

    void TransformComponent::OnEntityActivated(const AZ::EntityId& parentEntityId)
    {
        AZ_Assert(parentEntityId == m_parentId, "We expect to receive notifications only from the current parent!");

        m_parentActive = true;

#ifndef _RELEASE
        AZ::EntityId parentId = m_parentId;

        while (parentId.IsValid())
        {
            if (parentId == GetEntityId())
            {
                AZ_Error("TransformComponent", false, "Trying to create a circular dependency of parenting. Aborting set parent call.");
                SetParent(AZ::EntityId());
                return;
            }

            auto handler = AZ::TransformBus::FindFirstHandler(parentId);

            if (handler == nullptr)
            {
                break;
            }

            parentId = handler->GetParentId();
        }
#endif
        AZ::ComponentApplicationRequests* componentApplication = AZ::Interface<AZ::ComponentApplicationRequests>::Get();
        AZ::Entity* parentEntity = (componentApplication != nullptr) ? componentApplication->FindEntity(parentEntityId) : nullptr;
        AZ_Assert(parentEntity, "We expect to have a parent entity associated with the provided parent's entity Id.");
        if (parentEntity)
        {
            m_parentTM = parentEntity->GetTransform();

            AZ_Warning("TransformComponent", !m_isStatic || m_parentTM->IsStaticTransform(),
                "Entity '%s' %s has static transform, but parent has non-static transform. This may lead to unexpected movement.",
                GetEntity()->GetName().c_str(), GetEntityId().ToString().c_str());

            if (m_onNewParentKeepWorldTM)
            {
                ComputeLocalTM();
            }
            else
            {
                ComputeWorldTM();
            }
        }
    }

    void TransformComponent::OnEntityDeactivated([[maybe_unused]] const AZ::EntityId& parentEntityId)
    {
        AZ_Assert(parentEntityId == m_parentId, "We expect to receive notifications only from the current parent!");
        m_parentTM = nullptr;
        m_parentActive = false;
        ComputeLocalTM();
    }

    void TransformComponent::SetParentImpl(AZ::EntityId parentId, bool isKeepWorldTM)
    {
        if (GetEntity() && parentId == GetEntityId())
        {
            AZ_Warning("TransformComponent", false, "An entity can not be set as its own parent.");
            return;
        }

        AZ::EntityId oldParent = m_parentId;
        if (m_parentId.IsValid())
        {
            AZ::TransformNotificationBus::Handler::BusDisconnect();
            AZ::TransformHierarchyInformationBus::Handler::BusDisconnect();
            AZ::EntityBus::Handler::BusDisconnect();
            m_parentActive = false;
        }

        m_parentId = parentId;
        if (m_parentId.IsValid())
        {
            AZ::ComponentApplicationRequests* componentApplication = AZ::Interface<AZ::ComponentApplicationRequests>::Get();
            AZ::Entity* parentEntity = (componentApplication != nullptr) ? componentApplication->FindEntity(m_parentId) : nullptr;
            m_parentActive = parentEntity && (parentEntity->GetState() == AZ::Entity::State::Active);

            m_onNewParentKeepWorldTM = isKeepWorldTM;

            AZ::TransformNotificationBus::Handler::BusConnect(m_parentId);
            AZ::TransformHierarchyInformationBus::Handler::BusConnect(m_parentId);
            AZ::EntityBus::Handler::BusConnect(m_parentId);
        }
        else
        {
            m_parentTM = nullptr;

            if (isKeepWorldTM)
            {
                SetWorldTM(m_worldTM);
            }
            else
            {
                SetLocalTM(m_localTM);
            }

            if (oldParent.IsValid())
            {
                AZ::TransformNotificationBus::Event(
                    m_notificationBus, &AZ::TransformNotificationBus::Events::OnTransformChanged, m_localTM, m_worldTM);
                m_transformChangedEvent.Signal(m_localTM, m_worldTM);
            }
        }

        AZ::TransformNotificationBus::Event(m_notificationBus, &AZ::TransformNotificationBus::Events::OnParentChanged, oldParent, parentId);
        m_parentChangedEvent.Signal(oldParent, parentId);

        if (oldParent.IsValid() && oldParent != parentId) // Don't send removal notification while activating.
        {
            AZ::TransformNotificationBus::Event(oldParent, &AZ::TransformNotificationBus::Events::OnChildRemoved, GetEntityId());
            auto oldParentTransform = AZ::TransformBus::FindFirstHandler(oldParent);
            if (oldParentTransform)
            {
                oldParentTransform->NotifyChildChangedEvent(AZ::ChildChangeType::Removed, GetEntityId());
            }
        }

        AZ::TransformNotificationBus::Event(parentId, &AZ::TransformNotificationBus::Events::OnChildAdded, GetEntityId());
        auto newParentTransform = AZ::TransformBus::FindFirstHandler(parentId);
        if (newParentTransform)
        {
            newParentTransform->NotifyChildChangedEvent(AZ::ChildChangeType::Added, GetEntityId());
        }
    }

    void TransformComponent::SetLocalTMImpl(const AZ::Transform& tm)
    {
        m_localTM = tm;
        ComputeWorldTM();  // We can user dirty flags and compute it later on demand
    }

    void TransformComponent::SetWorldTMImpl(const AZ::Transform& tm)
    {
        m_worldTM = tm;
        ComputeLocalTM(); // We can user dirty flags and compute it later on demand
    }

    void TransformComponent::OnTransformChangedImpl(const AZ::Transform& /*parentLocalTM*/, const AZ::Transform& parentWorldTM)
    {
        // Called when our parent transform changes
        // Ignore the event until we've already derived our local transform.
        if (m_parentTM)
        {
            if (m_onParentChangedBehavior == AZ::OnParentChangedBehavior::Update)
            {
                m_worldTM = parentWorldTM * m_localTM;
                AZ::TransformNotificationBus::Event(
                    m_notificationBus, &AZ::TransformNotificationBus::Events::OnTransformChanged, m_localTM, m_worldTM);
                m_transformChangedEvent.Signal(m_localTM, m_worldTM);
            }
            else
            {
                // Update the local transform to make sure it is consistent with the parent's new transform
                // but keeps our world transform unchanged. Do not send any notifications here, because our world
                // transform has not changed, and with this OnParentChangedBehavior setting the expectation is that
                // another system will update our transform, and the notification will be triggered then.
                m_localTM = parentWorldTM.GetInverse() * m_worldTM;
            }
        }
    }

    void TransformComponent::ComputeLocalTM()
    {
        if (m_parentTM)
        {
            m_localTM = m_parentTM->GetWorldTM().GetInverse() * m_worldTM;
        }
        else if (!m_parentActive)
        {
            m_localTM = m_worldTM;
        }

        AZ::TransformNotificationBus::Event(
            m_notificationBus, &AZ::TransformNotificationBus::Events::OnTransformChanged, m_localTM, m_worldTM);
        m_transformChangedEvent.Signal(m_localTM, m_worldTM);

        AzFramework::IEntityBoundsUnion* boundsUnion = AZ::Interface<AzFramework::IEntityBoundsUnion>::Get();
        if (boundsUnion != nullptr)
        {
            boundsUnion->OnTransformUpdated(GetEntity());
        }
    }

    void TransformComponent::ComputeWorldTM()
    {
        if (m_parentTM)
        {
            m_worldTM = m_parentTM->GetWorldTM() * m_localTM;
        }
        else if (!m_parentActive)
        {
            m_worldTM = m_localTM;
        }

        AZ::TransformNotificationBus::Event(
            m_notificationBus, &AZ::TransformNotificationBus::Events::OnTransformChanged, m_localTM, m_worldTM);
        m_transformChangedEvent.Signal(m_localTM, m_worldTM);
    }

    bool TransformComponent::AreMoveRequestsAllowed() const
    {
        // Don't allow static transform to be moved while entity is activated.
        // But do allow a static transform to be moved when the entity is deactivated.
        if (m_isStatic && m_entity && (m_entity->GetState() > AZ::Entity::State::Init))
        {
            return false;
        }

        return true;
    }

    void TransformComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("TransformService", 0x8ee22c50));
    }

    void TransformComponent::Reflect(AZ::ReflectContext* reflection)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
        if (serializeContext)
        {
            serializeContext->ClassDeprecate("NetBindable", AZ::Uuid("{80206665-D429-4703-B42E-94434F82F381}"));

            serializeContext->Class<TransformComponent, AZ::Component>()
                ->Version(5, &TransformComponentVersionConverter)
                ->Field("Parent", &TransformComponent::m_parentId)
                ->Field("Transform", &TransformComponent::m_worldTM)
                ->Field("LocalTransform", &TransformComponent::m_localTM)
                ->Field("ParentActivationTransformMode", &TransformComponent::m_parentActivationTransformMode)
                ->Field("IsStatic", &TransformComponent::m_isStatic)
                ;
        }

        AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(reflection);
        if (behaviorContext)
        {
            behaviorContext->EBus<AZ::TransformNotificationBus>("TransformNotificationBus")->
                Handler<AZ::BehaviorTransformNotificationBusHandler>();

            behaviorContext->Class<TransformComponent>()->RequestBus("TransformBus");

            behaviorContext->EBus<AZ::TransformBus>("TransformBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Module, "components")
                ->Event("GetLocalTM", &AZ::TransformBus::Events::GetLocalTM)
                ->Event("GetWorldTM", &AZ::TransformBus::Events::GetWorldTM)
                ->Event("GetParentId", &AZ::TransformBus::Events::GetParentId)
                ->Event("GetLocalAndWorld", &AZ::TransformBus::Events::GetLocalAndWorld)
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Event("SetLocalTM", &AZ::TransformBus::Events::SetLocalTM)
                ->Event("SetWorldTM", &AZ::TransformBus::Events::SetWorldTM)
                ->Event("SetParent", &AZ::TransformBus::Events::SetParent)
                ->Event("SetParentRelative", &AZ::TransformBus::Events::SetParentRelative)
                ->Event("SetWorldTranslation", &AZ::TransformBus::Events::SetWorldTranslation)
                ->Event("SetLocalTranslation", &AZ::TransformBus::Events::SetLocalTranslation)
                ->Event("GetWorldTranslation", &AZ::TransformBus::Events::GetWorldTranslation)
                ->Event("GetLocalTranslation", &AZ::TransformBus::Events::GetLocalTranslation)
                    ->Attribute("Position", AZ::Edit::Attributes::PropertyPosition)
                ->VirtualProperty("Position", "GetLocalTranslation", "SetLocalTranslation")
                ->Event("MoveEntity", &AZ::TransformBus::Events::MoveEntity)
                ->Event("SetWorldX", &AZ::TransformBus::Events::SetWorldX)
                ->Event("SetWorldY", &AZ::TransformBus::Events::SetWorldY)
                ->Event("SetWorldZ", &AZ::TransformBus::Events::SetWorldZ)
                ->Event("GetWorldX", &AZ::TransformBus::Events::GetWorldX)
                ->Event("GetWorldY", &AZ::TransformBus::Events::GetWorldY)
                ->Event("GetWorldZ", &AZ::TransformBus::Events::GetWorldZ)
                ->Event("SetLocalX", &AZ::TransformBus::Events::SetLocalX)
                ->Event("SetLocalY", &AZ::TransformBus::Events::SetLocalY)
                ->Event("SetLocalZ", &AZ::TransformBus::Events::SetLocalZ)
                ->Event("GetLocalX", &AZ::TransformBus::Events::GetLocalX)
                ->Event("GetLocalY", &AZ::TransformBus::Events::GetLocalY)
                ->Event("GetLocalZ", &AZ::TransformBus::Events::GetLocalZ)
                ->Event("SetWorldRotation", &AZ::TransformBus::Events::SetWorldRotation)
                ->Event("SetWorldRotationQuaternion", &AZ::TransformBus::Events::SetWorldRotationQuaternion)
                ->Event("GetWorldRotation", &AZ::TransformBus::Events::GetWorldRotation)
                ->Event("GetWorldRotationQuaternion", &AZ::TransformBus::Events::GetWorldRotationQuaternion)
                ->Event("SetLocalRotation", &AZ::TransformBus::Events::SetLocalRotation)
                ->Event("SetLocalRotationQuaternion", &AZ::TransformBus::Events::SetLocalRotationQuaternion)
                ->Event("RotateAroundLocalX", &AZ::TransformBus::Events::RotateAroundLocalX)
                ->Event("RotateAroundLocalY", &AZ::TransformBus::Events::RotateAroundLocalY)
                ->Event("RotateAroundLocalZ", &AZ::TransformBus::Events::RotateAroundLocalZ)
                ->Event("GetLocalRotation", &AZ::TransformBus::Events::GetLocalRotation)
                ->Event("GetLocalRotationQuaternion", &AZ::TransformBus::Events::GetLocalRotationQuaternion)
                    ->Attribute("Rotation", AZ::Edit::Attributes::PropertyRotation)
                ->VirtualProperty("Rotation", "GetLocalRotationQuaternion", "SetLocalRotationQuaternion")
                ->Event("GetLocalScale", &AZ::TransformBus::Events::GetLocalScale)
                    ->Attribute("Scale", AZ::Edit::Attributes::PropertyScale)
                ->Event("SetLocalUniformScale", &AZ::TransformBus::Events::SetLocalUniformScale)
                ->Event("GetLocalUniformScale", &AZ::TransformBus::Events::GetLocalUniformScale)
                ->VirtualProperty("Uniform Scale", "GetLocalUniformScale", "SetLocalUniformScale")
                ->Event("GetChildren", &AZ::TransformBus::Events::GetChildren)
                ->Event("GetAllDescendants", &AZ::TransformBus::Events::GetAllDescendants)
                ->Event("GetEntityAndAllDescendants", &AZ::TransformBus::Events::GetEntityAndAllDescendants)
                ->Event("IsStaticTransform", &AZ::TransformBus::Events::IsStaticTransform)
                ->Event("SetOnParentChangedBehavior", &AZ::TransformBus::Events::SetOnParentChangedBehavior)
                ->Event("GetWorldUniformScale", &AZ::TransformBus::Events::GetWorldUniformScale)
                ;

            behaviorContext->Constant("TransformComponentTypeId", BehaviorConstant(AZ::TransformComponentTypeId));
            behaviorContext->ConstantProperty("EditorTransformComponentTypeId", BehaviorConstant(AZ::EditorTransformComponentTypeId))
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ;

            behaviorContext->Class<AZ::TransformConfig>()
                ->Attribute(AZ::Script::Attributes::ConstructorOverride, &AZ::TransformConfigConstructor)
                ->Enum<(int)AZ::TransformConfig::ParentActivationTransformMode::MaintainOriginalRelativeTransform>("MaintainOriginalRelativeTransform")
                ->Enum<(int)AZ::TransformConfig::ParentActivationTransformMode::MaintainCurrentWorldTransform>("MaintainCurrentWorldTransform")
                ->Constructor()
                ->Constructor<const AZ::Transform&>()
                ->Method("SetTransform", &AZ::TransformConfig::SetTransform)
                    ->Attribute(AZ::Script::Attributes::Deprecated, true)
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Method("SetLocalAndWorldTransform", &AZ::TransformConfig::SetLocalAndWorldTransform)
                    ->Attribute(AZ::Script::Attributes::Deprecated, true)
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Property("worldTransform", BehaviorValueProperty(&AZ::TransformConfig::m_worldTransform))
                ->Property("localTransform", BehaviorValueProperty(&AZ::TransformConfig::m_localTransform))
                ->Property("parentId", BehaviorValueProperty(&AZ::TransformConfig::m_parentId))
                ->Property("parentActivationTransformMode",
                    [](AZ::TransformConfig* config) { return (int&)(config->m_parentActivationTransformMode); },
                    [](AZ::TransformConfig* config, const int& i) { config->m_parentActivationTransformMode = (AZ::TransformConfig::ParentActivationTransformMode)i; })
                ->Property("isStatic", BehaviorValueProperty(&AZ::TransformConfig::m_isStatic))
                ;
        }
    }
} // namespace AZ

#endif
