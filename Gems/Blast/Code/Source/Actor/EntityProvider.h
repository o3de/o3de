/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
