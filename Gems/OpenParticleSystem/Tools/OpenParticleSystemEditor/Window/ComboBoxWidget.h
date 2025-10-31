/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/std/string/string.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzToolsFramework/UI/PropertyEditor/ReflectedPropertyEditor.hxx>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI_Internals.h>
#include <QComboBox>
#include <Serializer/ParticleSourceData.h>
#include <ParticleCommonData.h>
#include <QVBoxLayout>
#include <QCoreApplication>
#endif

namespace OpenParticleSystemEditor
{
    constexpr AZStd::string_view MATERIAL_EXTENTION = "material";
    constexpr AZStd::string_view MODEL_EXTENTION = "azmodel";

    const QString MATERIAL_DESCRIPTION = QCoreApplication::translate("ComboBoxWidget", "Material");
    const QString MESH_DESCRIPTION = QCoreApplication::translate("ComboBoxWidget", "Mesh");
    const QString SHAPE_LABEL = QCoreApplication::translate("ComboBoxWidget", "Shape");
    const QString RENDERER_LABEL = QCoreApplication::translate("ComboBoxWidget", "Renderer");

    class AssetWidget;

    using AssetChangeCB = AZStd::function<void(AZ::Data::AssetId)>;
    //! ComboBoxWidget represents a widget that has a drop down combo at the top which lets you select a class of objects
    //! and depending on what you pick, changes the rest of the UI.
    //! It consists of a ReflectedPropertyEditor control (showing a list of properties that you can edit) and below that,
    //! a series of asset picker controls.  Depending on the selected class, it changes what properties, and which asset
    //! controls are visible.
    //! For example, it shows up when you select an emitter's shape class, and lets ou pick from "Point", "Box", "Sphere",
    //! "Torus", "Cylinder", or "Mesh".
    //! If you pick "mesh", then the "mesh" asset control appears for you to pick a mesh.  Otherwise, that asset control
    //! is hidden.
    class ComboBoxWidget
        : public QWidget
    {
        Q_OBJECT;

    public:
        ComboBoxWidget(
            const AZStd::string& className,
            OpenParticle::ParticleSourceData::DetailInfo* detail,
            AZ::SerializeContext* serializeContext,
            AzToolsFramework::IPropertyEditorNotify* pnotify,
            QWidget* parent = nullptr);
        ~ComboBoxWidget() override {};

        void Clear();
        void Refresh();
        void Refresh(AZStd::any* instance);

        // connects the asset widgets to the given asset pointers.
        // Note that these asset pointers are in the "detail" object that was passed in the constructor,
        // and the actual asset<T> lives on the particle source data object.
        void SetAssetWidget(
            AZ::Data::Asset<AZ::RPI::MaterialAsset>* materialAsset,
            AZ::Data::Asset<AZ::RPI::ModelAsset>* modelAsset,
            AZ::Data::Asset<AZ::RPI::ModelAsset>* skeletonModelAsset);

    private slots:
        void OnIndexChanged(const QString& curString);

    Q_SIGNALS:
        // signal that the assets have been changed in the editor.
        void OnMaterialChanged();
        void OnModelChanged();
        void OnSkeletonModelChanged();

    private:
        void SetAssetWidgetVisible();
        void InitComboBox();
        void SetUI();

        AZStd::string m_className;
        AZStd::string m_moduleName;
        AZStd::string m_lastModuleName;
        AzToolsFramework::ReflectedPropertyEditor* m_propertyEditor;
        QLabel* m_nameLabel;
        QComboBox* m_comboBox;
        QVBoxLayout* m_layout;
        QWidget* m_parent;
        AssetWidget* m_materialAssetWidget = nullptr;
        AssetWidget* m_modelAssetWidget = nullptr;
        AssetWidget* m_skeletonModelAssetWidget = nullptr;
        AZStd::vector<AZStd::pair<AZStd::string, QString>> m_locationClasses;
        AZStd::vector<AZStd::pair<AZStd::string, QString>> m_rendererClasses;
    };
} // namespace OpenParticleSystemEditor
