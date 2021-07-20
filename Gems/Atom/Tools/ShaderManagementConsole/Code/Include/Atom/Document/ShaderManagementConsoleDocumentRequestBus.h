/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/std/any.h>
#include <AzCore/std/string/string_view.h>

#include <Atom/RPI.Reflect/Shader/ShaderOptionGroupLayout.h>
#include <Atom/RPI.Edit/Shader/ShaderSourceData.h>
#include <Atom/RPI.Edit/Shader/ShaderVariantListSourceData.h>

namespace ShaderManagementConsole
{
    using ShaderManagementConsoleDocumentResult = AZ::Outcome<AZStd::string, AZStd::string>;

    class ShaderManagementConsoleDocumentRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        typedef AZ::Uuid BusIdType;

        //! Get absolute path of document
        virtual AZStd::string_view GetAbsolutePath() const = 0;

        //! Get relative path of document
        virtual AZStd::string_view GetRelativePath() const = 0;

        //! Get the number of options
        virtual size_t GetShaderOptionCount() const = 0;

        //! Get the descriptor for the shader option at the specified index
        virtual const AZ::RPI::ShaderOptionDescriptor& GetShaderOptionDescriptor(size_t index) const = 0;

        //! Get the number of shader variants
        virtual size_t GetShaderVariantCount() const = 0;

        //! Get the information for the shader variant at the specified index
        virtual const AZ::RPI::ShaderVariantListSourceData::VariantInfo& GetShaderVariantInfo(size_t index) const = 0;

        //! Load document and related data
        //! @param loadPath Absolute path of document to load
        virtual ShaderManagementConsoleDocumentResult Open(AZStd::string_view loadPath) = 0;

        //! Save document to file
        virtual ShaderManagementConsoleDocumentResult Save() = 0;

        //! Save document copy
        //! @param savePath Absolute path where document is saved
        virtual ShaderManagementConsoleDocumentResult SaveAsCopy(AZStd::string_view savePath) = 0;

        //! Close document and reset its data
        virtual ShaderManagementConsoleDocumentResult Close() = 0;

        //! document is loaded
        virtual bool IsOpen() const = 0;

        //! document has changes pending
        virtual bool IsModified() const = 0;

        //! Can the document be saved
        virtual bool IsSavable() const = 0;

        //! Returns true if there are reversible modifications to the document
        virtual bool CanUndo() const = 0;

        //! Returns true if there are changes that were reversed and can be re-applied to the document
        virtual bool CanRedo() const = 0;

        //! Restores the previous state of the document
        virtual bool Undo() = 0;

        //! Restores the next state of the document
        virtual bool Redo() = 0;

        //! Signal that editing is about to begin, like beginning to drag a slider control
        virtual bool BeginEdit() = 0;

        //! Signal that editing has completed, like after releasing the mouse button after continuously dragging a slider control
        virtual bool EndEdit() = 0;
    };

    using ShaderManagementConsoleDocumentRequestBus = AZ::EBus<ShaderManagementConsoleDocumentRequests>;
} // namespace ShaderManagementConsole
