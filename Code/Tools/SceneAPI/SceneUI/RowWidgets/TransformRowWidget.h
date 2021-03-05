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

#pragma once

#if !defined(Q_MOC_RUN)
#include <QString>
#include <QLineEdit>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <SceneAPI/SceneUI/SceneUIConfiguration.h>
#endif

namespace AzQtComponents
{
    class VectorInput;
}

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneUI
        {
            class SCENE_UI_API ExpandedTransform
            {
            public:
                AZ_CLASS_ALLOCATOR_DECL;

                ExpandedTransform();
                explicit ExpandedTransform(const Transform& transform);
                
                void GetTransform(AZ::Transform& transform) const;
                void SetTransform(const AZ::Transform& transform);
                                
                const AZ::Vector3& GetTranslation() const;
                void SetTranslation(const AZ::Vector3& translation);

                const AZ::Vector3& GetRotation() const;
                void SetRotation(const AZ::Vector3& translation);

                const AZ::Vector3& GetScale() const;
                void SetScale(const AZ::Vector3& scale);

            private:
                AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
                AZ::Vector3 m_translation;
                AZ::Vector3 m_rotation;
                AZ::Vector3 m_scale;
                AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
            };

            class TransformRowWidget : public QWidget
            {
                Q_OBJECT
            public:
                AZ_CLASS_ALLOCATOR_DECL;

                explicit TransformRowWidget(QWidget* parent = nullptr);

                void SetEnableEdit(bool enableEdit);

                void SetTransform(const AZ::Transform& transform);
                void GetTransform(AZ::Transform& transform) const;
                const ExpandedTransform& GetExpandedTransform() const;

                AzQtComponents::VectorInput* GetTranslationWidget();
                AzQtComponents::VectorInput* GetRotationWidget();
                AzQtComponents::VectorInput* GetScaleWidget();

            protected:
                ExpandedTransform m_transform;

                bool m_expanded = true;

                AzQtComponents::VectorInput* m_translationWidget;
                AzQtComponents::VectorInput* m_rotationWidget;
                AzQtComponents::VectorInput* m_scaleWidget;
            };
        } // namespace SceneUI
    } // namespace SceneAPI
} // namespace AZ
