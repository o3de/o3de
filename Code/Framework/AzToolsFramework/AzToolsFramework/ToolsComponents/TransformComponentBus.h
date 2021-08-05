/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef TRANSFORMCOMPONENTBUS_H_
#define TRANSFORMCOMPONENTBUS_H_

#include <AzCore/base.h>
#include <AzCore/Math/Transform.h>

#pragma once

namespace AzToolsFramework
{
    namespace Components
    {
        //////////////////////////////////////////////////////////////////////////
        // Manages transform data as separate vector fields for editing purposes.
        struct EditorTransform
        {
            AZ_TYPE_INFO(EditorTransform, "{B02B7063-D238-4F40-A724-405F7A6D68CB}")

            EditorTransform()
            {
                m_translate = AZ::Vector3::CreateZero();
                m_legacyScale = AZ::Vector3::CreateOne();
                m_uniformScale = 1.0f;
                m_rotate = AZ::Vector3::CreateZero();
                m_locked = false;
            }

            static EditorTransform Identity()
            {
                return EditorTransform();
            }

            AZ::Vector3 m_translate; //!< Translation in engine units (meters)
            AZ::Vector3 m_legacyScale; //!< Legacy vector scale value, retained only for migration.
            float m_uniformScale; //!< Single scale value applied uniformly.
            AZ::Vector3 m_rotate; //!< Rotation in degrees
            bool m_locked;
        };

        //////////////////////////////////////////////////////////////////////////
        // messages controlling or polling the hierarchy
        //////////////////////////////////////////////////////////////////////////
        class TransformComponentMessages
            : public AZ::ComponentBus
        {
        public:

            using Bus = AZ::EBus<TransformComponentMessages>;

            //////////////////////////////////////////////////////////////////////////
            // Bus configuration
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple; // every listener registers unordered on this bus

            virtual const EditorTransform& GetLocalEditorTransform() = 0;
            virtual void SetLocalEditorTransform(const EditorTransform& dest) = 0;

            virtual void TranslateBy(const AZ::Vector3&) = 0;
            virtual void RotateBy(const AZ::Vector3&) = 0;

            virtual bool IsTransformLocked() = 0;
        };
    } // namespace Components
} // namespace AzToolsFramework

#endif // TRANSFORMCOMPONENTBUS_H_
