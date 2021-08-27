/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>

struct aiNode;

namespace AZ
{
    namespace SDKNode
    {
        class NodeWrapper
        {
        public:
            AZ_RTTI(NodeWrapper, "{5EB0897B-9728-44B7-B056-BA34AAF14715}");

            virtual ~NodeWrapper() = default;

            enum CurveNodeComponent
            {
                Component_X,
                Component_Y,
                Component_Z
            };

            virtual const char* GetName() const;
            virtual AZ::u64 GetUniqueId() const;
            virtual int GetMaterialCount() const;

            virtual int GetChildCount()const;
            virtual const std::shared_ptr<NodeWrapper> GetChild(int childIndex) const;
        };
    } //namespace Node
} //namespace AZ
