/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <QPointer>
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

namespace AzToolsFramework
{
    class PropertyDoubleSpinCtrl;
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

                const float GetScale() const;
                void SetScale(const float scale);

            private:
                AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
                AZ::Vector3 m_translation;
                AZ::Vector3 m_rotation;
                float m_scale;
                AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
            };

            class TransformRowWidget : public QWidget
            {
                Q_OBJECT
            public:
                AZ_CLASS_ALLOCATOR_DECL;

                explicit TransformRowWidget(QWidget* parent = nullptr);
                ~TransformRowWidget();

                void SetEnableEdit(bool enableEdit);

                void SetTransform(const AZ::Transform& transform);
                void GetTransform(AZ::Transform& transform) const;
                const ExpandedTransform& GetExpandedTransform() const;

                AzQtComponents::VectorInput* GetTranslationWidget();
                AzQtComponents::VectorInput* GetRotationWidget();
                AzToolsFramework::PropertyDoubleSpinCtrl* GetScaleWidget();

            protected:
                ExpandedTransform m_transform;

                bool m_expanded = true;

                QPointer<QWidget> m_containerWidget;
                AzQtComponents::VectorInput* m_translationWidget;
                AzQtComponents::VectorInput* m_rotationWidget;
                AzToolsFramework::PropertyDoubleSpinCtrl* m_scaleWidget;
            };
        } // namespace SceneUI
    } // namespace SceneAPI
} // namespace AZ
