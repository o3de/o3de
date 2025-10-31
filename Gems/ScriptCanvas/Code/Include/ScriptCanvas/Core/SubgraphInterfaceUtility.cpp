/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "SubgraphInterfaceUtility.h"

#include <AzCore/RTTI/BehaviorContext.h>

namespace SubgraphInterfaceUtilityCpp
{
    const constexpr size_t k_signatureIndex = 1;
    const constexpr AZ::u64 k_defaultOutIdSignature = 0x3ACF20E73ACF20E7ull;

    const constexpr AZ::u64 k_functionSourceIdObjectSignature = 0xADC636A91EA5433Aull;
    const constexpr AZ::u64 k_functionSourceIdNodeableSignature = 0xAD71FC30CA2E468Cull;

    using namespace ScriptCanvas;
    using namespace ScriptCanvas::Grammar;

    bool IsSignatureId(size_t index, AZ::u64 signature, const FunctionSourceId& id)
    {
        const AZ::u64* idData = reinterpret_cast<const AZ::u64*>(AZStd::ranges::data(id));
        return idData[index] == signature;
    }

    FunctionSourceId MakeSignatureId(size_t index, AZ::u64 signature, const FunctionSourceId& id)
    {
        FunctionSourceId signatureId = id;
        AZ::u64* idData = reinterpret_cast<AZ::u64*>(AZStd::ranges::data(signatureId));
        idData[index] = signature;
        return signatureId;
    }
}

namespace ScriptCanvas
{
    namespace Grammar
    {
        using namespace SubgraphInterfaceUtilityCpp;

        bool IsDefaultOutId(const FunctionSourceId& id)
        {
            return IsSignatureId(k_signatureIndex, k_defaultOutIdSignature, id);
        }

        bool IsFunctionSourceIdObject(const FunctionSourceId& id)
        {
            return IsSignatureId(k_signatureIndex, k_functionSourceIdObjectSignature, id);
        }

        bool IsFunctionSourceIdNodeable(const FunctionSourceId& id)
        {
            return IsSignatureId(k_signatureIndex, k_functionSourceIdNodeableSignature, id);
        }

        bool IsReservedId(const FunctionSourceId& id)
        {
            return IsDefaultOutId(id) || IsFunctionSourceIdNodeable(id) || IsFunctionSourceIdObject(id);
        }

        FunctionSourceId MakeDefaultOutId(const FunctionSourceId& id)
        {
            return MakeSignatureId(k_signatureIndex, k_defaultOutIdSignature, id);
        }

        FunctionSourceId MakeFunctionSourceIdObject()
        {
            return MakeSignatureId(k_signatureIndex, k_functionSourceIdObjectSignature, FunctionSourceId());
        }

        FunctionSourceId MakeFunctionSourceIdNodeable()
        {
            return MakeSignatureId(k_signatureIndex, k_functionSourceIdNodeableSignature, FunctionSourceId());
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
