/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <Atom/RPI.Reflect/Material/MaterialPropertyDescriptor.h>
#include <Atom/RPI.Public/Material/Material.h>

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/std/any.h>

#include <AtomToolsFramework/DynamicProperty/DynamicProperty.h>
#include <AtomToolsFramework/DynamicProperty/DynamicPropertyGroup.h>

namespace AZ
{
    namespace RPI
    {
        class MaterialSourceData;
        class MaterialTypeSourceData;
    } // namespace RPI
} // namespace AZ

namespace MaterialEditor
{
    //! UVs are processed in a property group but will be handled differently.
    static constexpr const char UvGroupName[] = "uvSets";

    class MaterialDocumentRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        typedef AZ::Uuid BusIdType;

        //! Get absolute path of material source file
        virtual AZStd::string_view GetAbsolutePath() const = 0;

        //! Get relative path of material source file
        virtual AZStd::string_view GetRelativePath() const = 0;

        //! Get material asset created by MaterialDocument
        virtual AZ::Data::Asset<AZ::RPI::MaterialAsset> GetAsset() const = 0;

        //! Get material instance created from asset loaded by MaterialDocument
        virtual AZ::Data::Instance<AZ::RPI::Material> GetInstance() const = 0;

        //! Get the internal material source data
        virtual const AZ::RPI::MaterialSourceData* GetMaterialSourceData() const = 0;

        //! Get the internal material type source data
        virtual const AZ::RPI::MaterialTypeSourceData* GetMaterialTypeSourceData() const = 0;

        //! Return property value
        //! If the document is not open or the id can't be found, an invalid value is returned instead.
        virtual const AZStd::any& GetPropertyValue(const AZ::Name& propertyFullName) const = 0;

        //! Returns a property object
        //! If the document is not open or the id can't be found, an invalid property is returned.
        virtual const AtomToolsFramework::DynamicProperty& GetProperty(const AZ::Name& propertyFullName) const = 0;
        
        //! Returns whether a property group is visible
        //! If the document is not open or the id can't be found, returns false.
        virtual bool IsPropertyGroupVisible(const AZ::Name& propertyGroupFullName) const = 0;

        //! Modify material property value
        virtual void SetPropertyValue(const AZ::Name& propertyFullName, const AZStd::any& value) = 0;

        //! Load source material and related data
        //! @param loadPath Absolute path of material to load
        virtual bool Open(AZStd::string_view loadPath) = 0;

        //! Reload document preserving edits
        virtual bool Rebuild() = 0;

        //! Save material to source file
        virtual bool Save() = 0;

        //! Save material to a new source file
        //! @param savePath Absolute path where material is saved
        virtual bool SaveAsCopy(AZStd::string_view savePath) = 0;

        //! Save material to a new source file as a child of the open material
        //! @param savePath Absolute path where material is saved
        virtual bool SaveAsChild(AZStd::string_view savePath) = 0;

        //! Close material document and reset its data
        virtual bool Close() = 0;

        //! Material is loaded
        virtual bool IsOpen() const = 0;

        //! Material has changes pending
        virtual bool IsModified() const = 0;

        //! Can the document be saved
        virtual bool IsSavable() const = 0;

        //! Returns true if there are reversible modifications to the material document
        virtual bool CanUndo() const = 0;

        //! Returns true if there are changes that were reversed and can be re-applied to the material document
        virtual bool CanRedo() const = 0;

        //! Restores the previous state of the material document
        virtual bool Undo() = 0;

        //! Restores the next state of the material document
        virtual bool Redo() = 0;

        //! Signal that property editing is about to begin, like beginning to drag a slider control
        virtual bool BeginEdit() = 0;

        //! Signal that property editing has completed, like after releasing the mouse button after continuously dragging a slider control
        virtual bool EndEdit() = 0;
    };

    using MaterialDocumentRequestBus = AZ::EBus<MaterialDocumentRequests>;
} // namespace MaterialEditor
