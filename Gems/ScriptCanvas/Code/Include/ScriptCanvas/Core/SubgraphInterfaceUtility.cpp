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

#include "SubgraphInterfaceUtility.h"

#include <AzCore/RTTI/BehaviorContext.h>
#include <ScriptCanvas/Libraries/Core/MethodUtility.h>

namespace SubgraphInterfaceUtilityCpp
{
    const constexpr size_t k_defaultOutIndex = 1;
    const constexpr size_t k_uniqueOutIndex = 0;
    const constexpr AZ::u64 k_defaultOutIdSignature = 0x3ACF20E73ACF20E7ull;
}

namespace ScriptCanvas
{
    namespace Grammar
    {
        using namespace SubgraphInterfaceUtilityCpp;

        bool IsDefaultOutId(const FunctionSourceId& id)
        {
            const AZ::u64* idData = reinterpret_cast<const AZ::u64*>(id.data);
            return idData[k_defaultOutIndex] == k_defaultOutIdSignature;
        }

        FunctionSourceId MakeDefaultOutId(const FunctionSourceId& id)
        {
            FunctionSourceId defaultOut = id;
            AZ::u64* idData = reinterpret_cast<AZ::u64*>(defaultOut.data);
            idData[k_defaultOutIndex] = k_defaultOutIdSignature;
            AZ_Assert(idData[k_uniqueOutIndex] != k_defaultOutIdSignature, "the default out must also be unique");
            return defaultOut;
        }

        bool OutIdIsEqual(const FunctionSourceId& lhs, const FunctionSourceId& rhs)
        {
            return lhs == rhs || (IsDefaultOutId(lhs) && IsDefaultOutId(rhs));
        }

        Ins CreateInsFromBehaviorContextMethods(const AZStd::string& /*className*/, const AZ::BehaviorContext& /*behaviorContext*/, const AZStd::unordered_set<AZStd::string>& /*branchingMethods*/)
        {
            Ins ins;
            return ins;
        }
               
        Outs CreateOutsFromBehaviorContextMethod(const AZ::BehaviorMethod& /*method*/)
        {
            Outs outs;
            return outs;
        }

        In* FindInByName(const AZStd::string& inName, Ins& ins)
        {
            if (auto in = FindInByNameNoError(inName, ins))
            {
                return in;
            }
            else
            {
                AZ_Error("ScriptCanvas", false, "No in named: %s", inName.data());
                return nullptr;
            }
        }

        In* FindInByNameNoError(const AZStd::string& inName, Ins& ins)
        {
            auto iter = AZStd::find_if(ins.begin(), ins.end(), [&inName](const In& in) { return in.displayName == inName; });
            return iter != ins.end() ? iter : nullptr;
        }

    } 

} 
