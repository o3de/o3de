#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

                bool ModifyTooltip(QString& toolTipString);

            protected:
                bool InitSceneManifest();

                virtual void DeleteObject();
                virtual void UpdateDeletable();
                
                virtual const char* GetSerializedName(const DataTypes::IManifestObject* target) const;
                virtual void UpdateUIForManifestObject(const DataTypes::IManifestObject* target);

                AZStd::string m_objectName;
                QScopedPointer<Ui::HeaderWidget> ui;
                Containers::SceneManifest* m_sceneManifest; // Reference only, does not point to a local instance.
                const DataTypes::IManifestObject* m_target; // Reference only, does not point to a local instance.
            };
        } // namespace UI
    } // namespace SceneAPI
} // namespace AZ
