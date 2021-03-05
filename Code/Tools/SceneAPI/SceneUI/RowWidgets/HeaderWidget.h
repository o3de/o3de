#pragma once

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

#if !defined(Q_MOC_RUN)
#include <QWidget>
#include <AzCore/std/string/string.h>
#include <AzCore/Memory/SystemAllocator.h>
#endif

namespace AZ
{
    namespace SceneAPI
    {
        namespace Containers
        {
            class SceneManifest;
        }
        namespace DataTypes
        {
            class IManifestObject;
        }
        namespace UI
        {
            // QT space
            namespace Ui
            {
                class HeaderWidget;
            }
            class HeaderWidget : public QWidget
            {
                Q_OBJECT
            public:
                AZ_CLASS_ALLOCATOR_DECL

                enum NameStack
                {
                    Label,
                    EditField
                };

                explicit HeaderWidget(QWidget* parent);

                void SetManifestObject(const DataTypes::IManifestObject* target);
                const DataTypes::IManifestObject* GetManifestObject() const;

            protected:
                bool InitSceneManifest();

                virtual void DeleteObject();
                virtual void UpdateDeletable();
                
                virtual const char* GetSerializedName(const DataTypes::IManifestObject* target) const;
                virtual void SetIcon(const DataTypes::IManifestObject* target);

                AZStd::string m_objectName;
                QScopedPointer<Ui::HeaderWidget> ui;
                Containers::SceneManifest* m_sceneManifest; // Reference only, does not point to a local instance.
                const DataTypes::IManifestObject* m_target; // Reference only, does not point to a local instance.
                bool m_nameIsEditable;
            };
        } // namespace UI
    } // namespace SceneAPI
} // namespace AZ
