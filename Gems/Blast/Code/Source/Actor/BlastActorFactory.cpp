/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Actor/BlastActorFactory.h>

#include <Actor/BlastActorImpl.h>
#include <AzFramework/Components/TransformComponent.h>
#include <Family/BlastFamily.h>
#include <NvBlastExtPxAsset.h>
#include <NvBlastTkActor.h>
#include <NvBlastTkAsset.h>
#include <PhysX/ComponentTypeIds.h>

namespace Blast
{
    BlastActor* BlastActorFactoryImpl::CreateActor(const BlastActorDesc& desc)
    {
        auto actor = aznew BlastActorImpl(desc);
        actor->Spawn();
        return actor;
    }

    void BlastActorFactoryImpl::DestroyActor(BlastActor* actor)
    {
        delete actor;
    }

    AZStd::vector<uint32_t> BlastActorFactoryImpl::CalculateVisibleChunks(
        const BlastFamily& blastFamily, const Nv::Blast::TkActor& tkActor) const
    {
        const Nv::Blast::TkAsset* tkAsset = tkActor.getAsset();
        AZ_Assert(tkAsset, "Invalid TkAsset on TkActor.");
        if (!tkAsset)
        {
            return {};
        }

        const Nv::Blast::ExtPxChunk* pxChunks = blastFamily.GetPxAsset().getChunks();
        const NvBlastChunk* chunks = tkAsset->getChunks();
        const uint32_t pxChunkCount = blastFamily.GetPxAsset().getChunkCount();

        AZ_Assert(pxChunks, "ExtPxAsset asset has a null chunk array.");
        AZ_Assert(chunks, "TkActor's asset has a null chunk array.");

        if (!pxChunks || !chunks)
        {
            return {};
        }

        AZStd::vector<uint32_t> chunkIndicesScratch;
        chunkIndicesScratch.resize_no_construct(tkActor.getVisibleChunkCount());
        tkActor.getVisibleChunkIndices(chunkIndicesScratch.data(), static_cast<uint32_t>(chunkIndicesScratch.size()));

        AZStd::vector<uint32_t> chunkIndices;
        // Filter the visible chunks to ensure they have subchunks
        chunkIndices.reserve(chunkIndicesScratch.size());
        for (uint32_t chunkIndex : chunkIndicesScratch)
        {
            AZ_Assert(chunkIndex < pxChunkCount, "Out of bounds access to the ExtPxAsset's PxChunks.");
            if (chunkIndex < pxChunkCount)
            {
                const Nv::Blast::ExtPxChunk& chunk = pxChunks[chunkIndex];
                if (chunk.subchunkCount > 0)
                {
                    chunkIndices.push_back(chunkIndex);
                }
            }
        }
        return chunkIndices;
    }

    bool BlastActorFactoryImpl::CalculateIsLeafChunk(
        const Nv::Blast::TkActor& tkActor, const AZStd::vector<uint32_t>& chunkIndices) const
    {
        const Nv::Blast::TkAsset* tkAsset = tkActor.getAsset();
        AZ_Assert(tkAsset, "Invalid TkAsset on TkActor.");
        if (!tkAsset)
        {
            return false;
        }

        const uint32_t nodeCount = tkActor.getGraphNodeCount();
        const uint32_t chunkCount = tkAsset->getChunkCount();
        const NvBlastChunk* chunks = tkAsset->getChunks();

        AZStd::vector<uint32_t> chunkIndicesScratch;
        chunkIndicesScratch.resize_no_construct(tkActor.getVisibleChunkCount());
        tkActor.getVisibleChunkIndices(chunkIndicesScratch.data(), static_cast<uint32_t>(chunkIndicesScratch.size()));

        bool isLeafChunk = false;
        // Single lower-support chunk actors might be leaf actors - if so, set our leaf chunk flag for use in filter
        // data
        if (nodeCount <= 1 && !chunkIndicesScratch.empty() && !chunkIndices.empty())
        {
            const uint32_t chunkIndex = chunkIndices[0];
            AZ_Assert(chunkIndex < chunkCount, "Out of bounds access to the ExtPxAsset's PxChunks.");
            if (chunkIndex < chunkCount)
            {
                const NvBlastChunk& chunk = chunks[chunkIndex];
                // Mark as a leaf chunk if it has no children
                isLeafChunk = chunk.firstChildIndex == chunk.childIndexStop;
            }
        }
        return isLeafChunk;
    }

    bool BlastActorFactoryImpl::CalculateIsStatic(
        const BlastFamily& blastFamily, const Nv::Blast::TkActor& tkActor,
        const AZStd::vector<uint32_t>& chunkIndices) const
    {
        return tkActor.isBoundToWorld() || SupportGraphHasStaticActor(blastFamily, tkActor) ||
            VisibleChunksHasStaticActor(blastFamily, chunkIndices);
    }

    AZStd::vector<AZ::Uuid> BlastActorFactoryImpl::CalculateComponents(bool isStatic) const
    {
        if (isStatic)
        {
            return {AZ::TransformComponentTypeId, PhysX::StaticRigidBodyComponentTypeId};
        }
        return
            {AZ::TransformComponentTypeId,
             /* RigidBodyComponent */ AZ::TypeId("{D4E52A70-BDE1-4819-BD3C-93AB3F4F3BE3}")};
    }

    bool BlastActorFactoryImpl::SupportGraphHasStaticActor(
        const BlastFamily& blastFamily, const Nv::Blast::TkActor& tkActor) const
    {
        const uint32_t nodeCount = tkActor.getGraphNodeCount();
        const Nv::Blast::ExtPxChunk* pxChunks = blastFamily.GetPxAsset().getChunks();
        const Nv::Blast::TkAsset* tkAsset = tkActor.getAsset();

        if (nodeCount == 0 || !pxChunks || !tkAsset)
        {
            AZ_Assert(pxChunks, "BlastFamily's asset has a null chunk array.");
            AZ_Assert(tkAsset, "Invalid TkAsset on TkActor.");
            return false;
        }

        bool staticFound = false;

        AZStd::vector<uint32_t> graphNodeIndices;
        graphNodeIndices.resize_no_construct(nodeCount);
        tkActor.getGraphNodeIndices(graphNodeIndices.data(), static_cast<uint32_t>(graphNodeIndices.size()));
        const NvBlastSupportGraph graph = tkAsset->getGraph();
        const uint32_t chunkCount = blastFamily.GetPxAsset().getChunkCount();

        for (uint32_t graphNodeIndex : graphNodeIndices)
        {
            if (graphNodeIndex >= graph.nodeCount)
            {
                AZ_Assert(false, "Out of bounds access to NvBlastSupportGraph.");
                continue;
            }

            const uint32_t chunkIndex = graph.chunkIndices[graphNodeIndex];

            if (chunkIndex >= chunkCount)
            {
                AZ_Assert(false, "Out of bounds access to BlastFamily asset's ExtPxChunks.");
                continue;
            }

            const Nv::Blast::ExtPxChunk& chunk = pxChunks[chunkIndex];

            if (chunk.isStatic)
            {
                staticFound = true;
                break;
            }
        }

        return staticFound;
    }

    bool BlastActorFactoryImpl::VisibleChunksHasStaticActor(
        const BlastFamily& blastFamily, const AZStd::vector<uint32_t>& chunkIndices) const
    {
        const Nv::Blast::ExtPxChunk* pxChunks = blastFamily.GetPxAsset().getChunks();
        if (!pxChunks)
        {
            AZ_Assert(false, "ExtPxAsset has a null chunk array.");
            return false;
        }

        bool staticFound = false;
        for (const uint32_t chunkIndex : chunkIndices)
        {
            const Nv::Blast::ExtPxChunk& chunk = pxChunks[chunkIndex];

            if (chunk.isStatic)
            {
                staticFound = true;
                break;
            }
        }

        return staticFound;
    }
} // namespace Blast
