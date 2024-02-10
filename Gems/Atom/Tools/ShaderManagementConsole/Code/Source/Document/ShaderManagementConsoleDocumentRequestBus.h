/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Edit/Shader/ShaderSourceData.h>
#include <Atom/RPI.Edit/Shader/ShaderVariantListSourceData.h>
#include <Atom/RPI.Reflect/Shader/ShaderOptionGroupLayout.h>
#include <AzCore/Asset/AssetCommon.h>

namespace ShaderManagementConsole
{
    struct DocumentVerificationResult
    {
        bool AllGood() const { return !m_hasRedundantVariants && !m_hasRootLike && !m_hasStableIdJump; }

        bool m_hasRedundantVariants = false;
        bool m_hasRootLike = false;
        AZ::u32 m_rootLikeStableId{};
        bool m_hasStableIdJump = false;
        AZ::u32 faultyId{};
    };

    class ShaderManagementConsoleDocumentRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        typedef AZ::Uuid BusIdType;

        //! Add a new shader variant with a unique stable ID to the variant list
        virtual void AddOneVariantRow() = 0;

        //! Add a batch of variants
        //! The variants don't have to be fully enumerated, only some options may participate
        //! `optionHeaders` are like a csv file first line, they name the columns.
        //! example:      o_fog  |  o_shadow  |  o_brdfModel
        //!              --------|------------|--------------
        //!                 0    |     1      |
        //!                 1    |     0      |
        //! In that case optionHeaders is ["o_fog", "o_shadow"]
        //! and         matrixOfValues is [ 0,1, 1,0 ]    # flattened values-subrect matrix
        virtual void AppendSparseVariantSet(AZStd::vector<AZ::Name> optionHeaders, AZStd::vector<AZ::Name> matrixOfValues) = 0;

        //! Mix-expand a batch of variants
        //! Like the function above the options, the argument and matrix layout work in the same fashion.
        //! The difference is that instead of append, this will "multiply" the current variants in the document,
        //! with the given new variants (matrix rows).
        //! So if you have 10 current variants in your document, and pass a matrix of 4 options x 2 rows,
        //! The final document will have 20 variants.
        //! If the matrixOfValues specifies option values that are already set by the current variants,
        //! the current option values will be overwritten and lost.
        //! The matrixOfValues is expected to be a full enumeration in the current usage client: the ExpandOptionsFullCOmbinatorials.py script
        //! Therefore losing previous values is not a problem since a full enumeration will necessarily cover the previous values as well.
        virtual void MultiplySparseVariantSet(AZStd::vector<AZ::Name> optionHeaders, AZStd::vector<AZ::Name> matrixOfValues) = 0;

        //! Uniquifies and recompacts stableID space
        virtual void DefragmentVariantList() = 0;

        //! Set the shader variant list source data on the document.
        //! This function can be used to edit and update the data contained within the document.
        //! Functions can be added to this bus for more fine grained editing of shader variant list data.
        virtual void SetShaderVariantListSourceData(const AZ::RPI::ShaderVariantListSourceData& shaderVariantListSourceData) = 0;

        //! Get the shader variant list source data from the document.
        virtual const AZ::RPI::ShaderVariantListSourceData& GetShaderVariantListSourceData() const = 0;

        //! Get the number of shader options stored in the shader asset.
        //! Note that the shader asset can contain more descriptors than are stored in the shader variant list source data.
        virtual size_t GetShaderOptionDescriptorCount() const = 0;

        //! Get the shader option descriptor from the shader asset.
        //! Note that the shader asset can contain more descriptors than are stored in the shader variant list source data.
        virtual const AZ::RPI::ShaderOptionDescriptor& GetShaderOptionDescriptor(size_t index) const = 0;

        //! Verify before save that some guarantees are respected (like contiguous stableids)
        virtual DocumentVerificationResult Verify() const = 0;
    };

    using ShaderManagementConsoleDocumentRequestBus = AZ::EBus<ShaderManagementConsoleDocumentRequests>;
} // namespace ShaderManagementConsole
