/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/Memory.h>
#include <AzCore/std/string/string.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/ICommentRule.h>

namespace AZ
{
    class ReflectContext;

    namespace SceneAPI
    {
        namespace Containers
        {
            class Scene;
        }
        namespace SceneData
        {
            class CommentRule
                : public DataTypes::ICommentRule
            {
            public:
                AZ_RTTI(CommentRule, "{9A20AC53-04B3-4A2F-A43F-338456974874}", DataTypes::ICommentRule);
                AZ_CLASS_ALLOCATOR_DECL

                ~CommentRule() override = default;

                const AZStd::string& GetComment() const override;

                static void Reflect(ReflectContext* context);

            protected:
                AZStd::string m_comment;
            };
        } // SceneData
    } // SceneAPI
} // AZ
