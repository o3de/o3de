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

namespace ScriptCanvas
{
    namespace SlotExecution
    {
        class Map;
    }

    namespace Nodes
    {
        namespace Core
        {
            class FunctionCallNode;

            struct FunctionCallNodeCompareConfig
            {
                bool ignoreInNameChanges = true;
                bool ignoreInputDefaultValueChanges = false;
                bool ignoreInputNameChanges = false;
                bool ignoreInputSourceIdChanges = false;
                bool ignoreInSourceIdChanges = false;
                
                bool ignoreOutNameChanges = true;
                bool ignoreOutputNameChanges = false;
                bool ignoreOutputSourceIdChanges = false;
                bool ignoreOutSourceIdChanges = false;

                bool ignorePurityChanges = true;
            };

            struct IsFunctionCallOutOfDateConfig
            {
                const FunctionCallNodeCompareConfig& compare;
                const FunctionCallNode& node;
                const SlotExecution::Map& slotMap;
                const Grammar::FunctionSourceId& sourceId;
                const Grammar::SubgraphInterface& originalInterface;
                const Grammar::SubgraphInterface& latestInterface;
            };

            bool IsFunctionCallNodeOutOfDate(const IsFunctionCallOutOfDateConfig& config);

            bool IsFunctionCallNodeOutOfDateLatents(const IsFunctionCallOutOfDateConfig& config);

            bool IsFunctionCallNodeOutOfDateNodeable(const IsFunctionCallOutOfDateConfig& config);

            bool IsFunctionCallNodeOutOfDatePure(const IsFunctionCallOutOfDateConfig& config);

            bool IsOutOfDate(const FunctionCallNodeCompareConfig& config, const Grammar::In& inOld, const Grammar::In& inNew);

            bool IsOutOfDate(const FunctionCallNodeCompareConfig& config, const Grammar::Input& inputOld, const Grammar::Input& inputNew);

            bool IsOutOfDate(const FunctionCallNodeCompareConfig& config, const Grammar::Out& outOld, const Grammar::Out& outNew);

            bool IsOutOfDate(const FunctionCallNodeCompareConfig& config, const Grammar::Output& outputOld, const Grammar::Output& outputNew);
        }
    }
}
