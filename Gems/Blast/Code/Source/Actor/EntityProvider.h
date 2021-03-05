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
#pragma once

#include <AzCore/Component/Entity.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>

namespace Blast
{
    //! Abstracts away creation of an entity with components for convenience.
    class EntityProvider
    {
    public:
        static AZStd::shared_ptr<EntityProvider> Create();

        virtual ~EntityProvider() = default;

        //! Returns an entity with the specified components.
        //! @note If any of the components failed to be created, returns a nullptr instead.
        virtual AZStd::shared_ptr<AZ::Entity> CreateEntity(const AZStd::vector<AZ::Uuid>& componentIds) = 0;
    };

    class EntityProviderImpl : public EntityProvider
    {
    public:
        ~EntityProviderImpl() = default;

        AZStd::shared_ptr<AZ::Entity> CreateEntity(const AZStd::vector<AZ::Uuid>& componentIds) override;
    };
} // namespace Blast
