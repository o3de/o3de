#pragma once

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
