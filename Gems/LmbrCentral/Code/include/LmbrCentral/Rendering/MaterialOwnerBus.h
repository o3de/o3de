/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Math/Color.h>
#include <IMaterial.h>
#include <LmbrCentral/Rendering/MaterialHandle.h>

namespace LmbrCentral
{
    /*!
     * Messages serviced by components that support materials (e.g. Mesh, Decal).
     * We specifically chose the name "MaterialOwnerRequestBus" rather than just "MaterialRequestBus" to communicate
     * the fact that the requests are not for a Material directly, but for an Entity/Component that uses a Material.
     */
    class MaterialOwnerRequests
        : public AZ::ComponentBus
    {
    public:

        //! Sets the component's current material.
        virtual void SetMaterial(_smart_ptr<IMaterial>) = 0;
        //! Gets the component's current material.
        virtual _smart_ptr<IMaterial> GetMaterial() = 0;

        //! Indicates whether the Material Owner is fully initialized, and MaterialOwnerRequestBus can be used on the Material.
        virtual bool IsMaterialOwnerReady() { return true; }

        //! Sets the component's current material. This MaterialHandle version provides support for BehaviorContext reflection.
        //! \param materialHandle       New material handle
        virtual void SetMaterialHandle(const MaterialHandle& /*materialHandle*/) {};
        //! Gets the component's current material. This MaterialHandle version provides support for BehaviorContext reflection.
        virtual MaterialHandle GetMaterialHandle() { return MaterialHandle(); }

        //! Sets a Material property for the bus Entity. The Material will be cloned once before any changes are applied, so other instances are not affected.
        //! \param name        Name of the material param to set. May be a custom defined param in the shader, or one of the standard lighting params (diffuse, specular, emissive_color, emissive_intensity, shininess, opacity, alpha).
        //! \param value       New value for the param
        //! \param materialId  ID of the desired material slot. The first slot is Material ID 1.
        virtual void SetMaterialParamVector4(const AZStd::string& /*name*/, const AZ::Vector4& /*value*/, int /*materialId = 1*/) {};

        //! Sets a Material property for the bus Entity. The Material will be cloned once before any changes are applied, so other instances are not affected.
        //! \param name        Name of the material param to set. May be a custom defined param in the shader, or one of the standard lighting params (diffuse, specular, emissive_color, emissive_intensity, shininess, opacity, alpha).
        //! \param value       New value for the param
        //! \param materialId  ID of the desired material slot. The first slot is Material ID 1.
        virtual void SetMaterialParamVector3(const AZStd::string& /*name*/, const AZ::Vector3& /*value*/, int /*materialId = 1*/) {};

        //! Sets a Material property for the bus Entity. The Material will be cloned once before any changes are applied, so other instances are not affected.
        //! \param name        Name of the material param to set. May be a custom defined param in the shader, or one of the standard lighting params (diffuse, specular, emissive_color, emissive_intensity, shininess, opacity, alpha).
        //! \param value       New value for the param
        //! \param materialId  ID of the desired material slot. The first slot is Material ID 1.
        virtual void SetMaterialParamColor(const AZStd::string& /*name*/, const AZ::Color& /*value*/, int /*materialId = 1*/) {};

        //! Sets a Material property for the bus Entity. The Material will be cloned once before any changes are applied, so other instances are not affected.
        //! \param name        Name of the material param to set. May be a custom defined param in the shader, or one of the standard lighting params (diffuse, specular, emissive_color, emissive_intensity, shininess, opacity, alpha).
        //! \param value       New value for the param
        //! \param materialId  ID of the desired material slot. The first slot is Material ID 1.
        virtual void SetMaterialParamFloat(const AZStd::string& /*name*/, float /*value*/, int /*materialId = 1*/) {};

        //! Returns a Material property value for the bus Entity.
        //! \param name        Name of the material param to get. May be a custom defined param in the shader, or one of the standard lighting params (diffuse, specular, emissive_color, emissive_intensity, shininess, opacity, alpha).
        //! \param materialId  ID of the desired material slot. The first slot is Material ID 1.
        //! \return            The value of the param, or 0's if the param could not be found.
        virtual AZ::Vector4 GetMaterialParamVector4(const AZStd::string& /*name*/, int /*materialId = 1*/) { return AZ::Vector4::CreateZero(); };

        //! Returns a Material property value for the bus Entity.
        //! \param name        Name of the material param to get. May be a custom defined param in the shader, or one of the standard lighting params (diffuse, specular, emissive_color, emissive_intensity, shininess, opacity, alpha).
        //! \param materialId  ID of the desired material slot. The first slot is Material ID 1.
        //! \return            The value of the param, or 0's if the param could not be found.
        virtual AZ::Vector3 GetMaterialParamVector3(const AZStd::string& /*name*/, int /*materialId = 1*/) { return AZ::Vector3::CreateZero(); };

        //! Returns a Material property value for the bus Entity.
        //! \param name        Name of the material param to get. May be a custom defined param in the shader, or one of the standard lighting params (diffuse, specular, emissive_color, emissive_intensity, shininess, opacity, alpha).
        //! \param materialId  ID of the desired material slot. The first slot is Material ID 1.
        //! \return            The value of the param, or 0's if the param could not be found.
        virtual AZ::Color GetMaterialParamColor(const AZStd::string& /*name*/, int /*materialId = 1*/) { return AZ::Color::CreateZero(); };

        //! Returns a Material property value for the bus Entity.
        //! \param name        Name of the material param to get. May be a custom defined param in the shader, or one of the standard lighting params (diffuse, specular, emissive_color, emissive_intensity, shininess, opacity, alpha).
        //! \param materialId  ID of the desired material slot. The first slot is Material ID 1.
        //! \return            The value of the param, or 0 if the param could not be found.
        virtual float     GetMaterialParamFloat(const AZStd::string& /*name*/, int /*materialId = 1*/) { return 0.0f; };
    };

    using MaterialOwnerRequestBus = AZ::EBus<MaterialOwnerRequests>;

    /*!
     * Messages sent by components that support materials (e.g. Mesh, Decal).
     * We specifically chose the name "MaterialOwnerNotificationBus" rather than just "MaterialNotificationBus" to communicate
     * the fact that the requests are not for a Material directly, but for an Entity/Component that uses a Material.
     */
    class MaterialOwnerNotifications
        : public AZ::ComponentBus
    {
    public:

        //! Sent when the material owner is fully initialized, and MaterialOwnerRequestBus can be used on the Material.
        //! Before this event, MaterialOwnerRequestBus functions probably won't do anything, because the Material likely
        //! doesn't exist yet.
        virtual void OnMaterialOwnerReady() = 0;

        /**
        * When connecting to this bus, if the material owner is ready you will immediately get an OnMaterialOwnerReady event
        **/
        template<class Bus>
        struct ConnectionPolicy
            : public AZ::EBusConnectionPolicy<Bus>
        {
            static void Connect(typename Bus::BusPtr& busPtr, typename Bus::Context& context, typename Bus::HandlerNode& handler, typename Bus::Context::ConnectLockGuard& connectLock, const typename Bus::BusIdType& id = 0)
            {
                AZ::EBusConnectionPolicy<Bus>::Connect(busPtr, context, handler, connectLock, id);

                bool readyResult = false;
                LmbrCentral::MaterialOwnerRequestBus::EventResult(
                    readyResult,
                    id,
                    &LmbrCentral::MaterialOwnerRequestBus::Events::IsMaterialOwnerReady);

                if (readyResult)
                {
                    handler->OnMaterialOwnerReady();
                }
            }
        };
    };

    using MaterialOwnerNotificationBus = AZ::EBus<MaterialOwnerNotifications>;

} // namespace LmbrCentral
