/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
                m_scale = AZ::Vector3::CreateOne();
                m_rotate = AZ::Vector3::CreateZero();
                m_locked = false;
            }

            static EditorTransform Identity()
            {
                return EditorTransform();
            }

            AZ::Vector3 m_translate; //! Translation in engine units (meters)
            AZ::Vector3 m_scale;
            AZ::Vector3 m_rotate; //! Rotation in degrees
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
            virtual void ScaleBy(const AZ::Vector3&) = 0;

            virtual bool IsTransformLocked() = 0;
        };
    } // namespace Components
} // namespace AzToolsFramework

#endif // TRANSFORMCOMPONENTBUS_H_
