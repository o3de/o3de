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
#ifndef EDITOR_ASSET_REFERENCE_H
#define EDITOR_ASSET_REFERENCE_H

#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/RTTI/RTTI.h>

namespace AZ
{
    struct ClassDataReflection;
}

namespace AzToolsFramework
{
    // the base class of an editor asset reference - this is what you would derive a model reference from, for example
    // these guys show up in the editor as a live drag-and-droppable field.
    class AssetReferenceBase
    {
    public:
        AZ_RTTI(AssetReferenceBase, "{C30974B6-5831-443D-BFB2-CDF12600164D}");
        AssetReferenceBase() {}
        virtual ~AssetReferenceBase() {}
        virtual AZ::Data::AssetType GetAssetType() const = 0;
        const AZ::Data::AssetId& GetCurrentID() const { return m_currentID; }
        void SetCurrentID(const AZ::Data::AssetId& value) { m_currentID = value; }

        static void Reflect(AZ::ReflectContext* context);

    protected:
        AZ::Data::AssetId m_currentID;
    };
}

#endif
