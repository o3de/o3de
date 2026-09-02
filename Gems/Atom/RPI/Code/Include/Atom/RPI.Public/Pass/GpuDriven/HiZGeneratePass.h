/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/array.h>

#include <Atom/RPI.Public/Configuration.h>
#include <Atom/RPI.Public/Image/AttachmentImage.h>
#include <Atom/RPI.Public/Pass/ParentPass.h>
#include <Atom/RPI.Public/Shader/ShaderReloadNotificationBus.h>
#include <Atom/RPI.Reflect/Asset/AssetReference.h>

namespace AZ
{
    namespace RPI
    {
        //! Generates a Hi-Z (hierarchical depth) pyramid from the depth prepass output.
        //! Creates child ComputePass instances that perform cascading 2x2 min-depth
        //! reductions across the mip chain of the output Hi-Z image.
        //!
        //! The first child reads the depth buffer and writes Hi-Z mip 0 (half resolution).
        //! Subsequent children read Hi-Z mip N and write Hi-Z mip N+1.
        class ATOM_RPI_PUBLIC_API HiZGeneratePass
            : public ParentPass
            , private ShaderReloadNotificationBus::Handler
        {
            AZ_RPI_PASS(HiZGeneratePass);

        public:
            AZ_RTTI(HiZGeneratePass, "{F1A2B3C4-D5E6-7890-ABCD-EF1234567891}", ParentPass);
            AZ_CLASS_ALLOCATOR(HiZGeneratePass, SystemAllocator);

            static Ptr<HiZGeneratePass> Create(const PassDescriptor& descriptor);
            virtual ~HiZGeneratePass();

            // -- Phase 7: persistent, double-buffered Hi-Z pyramid --
            // Only meaningful when this instance was built from a template that declares an owned
            // image attachment named "HiZPyramidPersistent" (see SetupPersistentPyramid / the
            // HiZGeneratePersistentTemplate .pass file) -- the stock HiZGenerateTemplate has no such
            // attachment, so IsPersistent() is false for it and it is completely unaffected.
            bool IsPersistent() const { return m_isPersistent; }

            //! True once both ping-pong slots have been written at least once since this instance was
            //! (re)built (e.g. after a resize). False content in an unpopulated slot would otherwise
            //! be undefined driver memory, not a "nothing is occluding" value -- callers (e.g. a
            //! future Meshlets cluster-cull binding) must check this before sampling
            //! GetLastCompletedPyramid().
            bool IsPersistentPyramidPopulated() const { return m_isPersistent && m_framesSincePersistentActivation >= PersistentPyramidCount; }

            //! The pyramid slot NOT being written this frame, i.e. the last fully-completed one --
            //! safe for an external, earlier-in-the-frame consumer to sample without racing this
            //! instance's own mip-chain dispatch. Null unless IsPersistent().
            Data::Instance<AttachmentImage> GetLastCompletedPyramid() const;

            //! The pyramid slot being WRITTEN this frame (the one bound to "HiZOutput").
            //! Safe ONLY for consumers whose scope the frame graph orders AFTER this
            //! pass's mip-chain dispatch AND who get their transition through a declared
            //! read of the HiZOutput attachment (e.g. a pipeline pass connected to it) --
            //! never sample it from an earlier-in-frame scope. Null unless IsPersistent().
            Data::Instance<AttachmentImage> GetCurrentPyramid() const
            {
                return m_isPersistent ? m_persistentPyramidImages[m_persistentPyramidIndex] : nullptr;
            }

        protected:
            explicit HiZGeneratePass(const PassDescriptor& descriptor);

            // Pass behavior overrides
            void ResetInternal() override;
            void BuildInternal() override;
            void FrameBeginInternal(FramePrepareParams params) override;

            // ShaderReloadNotificationBus::Handler overrides
            void OnShaderReinitialized(const Shader& shader) override;
            void OnShaderAssetReinitialized(const Data::Asset<ShaderAsset>& shaderAsset) override;
            void OnShaderVariantReinitialized(const ShaderVariant& shaderVariant) override;

        private:
            void GetAttachmentInfo();
            void BuildChildPasses();
            void UpdateChildren();

            // Creates (or, on a rebuild e.g. resize, recreates) the two persistent pyramid images and
            // manually wires one of them into the "HiZOutput" slot, if -- and only if -- this instance
            // owns a "HiZPyramidPersistent" attachment. A no-op (m_isPersistent stays false) for the
            // stock template. Deliberately NOT wired via the template's "Connections" array: an
            // Imported attachment's m_importedResource must exist before anything calls SetAttachment
            // on its binding or PassAttachmentBinding::SetAttachment asserts -- see
            // ReflectionScreenSpaceFilterPass's identical persistent-image pattern.
            void SetupPersistentPyramid();

            // Ping-pongs which of the two persistent images is bound to "HiZOutput" this frame. Called
            // every FrameBeginInternal (not just on rebuild) so consumers reading the OTHER slot never
            // race this instance's own mip-chain dispatch. No-op unless m_isPersistent.
            void SwapPersistentPyramid();

            AssetReference m_shaderReference;

            uint32_t m_depthWidth = 0;
            uint32_t m_depthHeight = 0;
            uint16_t m_hiZMipLevels = 0;

            bool m_needToRebuildChildren = true;
            bool m_needToUpdateChildren = true;

            static constexpr uint32_t PersistentPyramidCount = 2;
            bool m_isPersistent = false;
            uint32_t m_persistentPyramidIndex = 0;
            uint32_t m_framesSincePersistentActivation = 0;
            AZStd::array<Data::Instance<AttachmentImage>, PersistentPyramidCount> m_persistentPyramidImages;
        };
    } // namespace RPI
} // namespace AZ
