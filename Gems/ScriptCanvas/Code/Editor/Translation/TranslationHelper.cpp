/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "TranslationHelper.h"

namespace ScriptCanvasEditor::TranslationHelper
{
    AZStd::string SanitizeText(const AZStd::string& text)
    {
        AZStd::string result = text;
        AZ::StringFunc::Replace(result, "+", "");
        AZ::StringFunc::Replace(result, "-", "");
        AZ::StringFunc::Replace(result, "*", "");
        AZ::StringFunc::Replace(result, "/", "");
        AZ::StringFunc::Replace(result, "(", "");
        AZ::StringFunc::Replace(result, ")", "");
        AZ::StringFunc::Replace(result, "{", "");
        AZ::StringFunc::Replace(result, "}", "");
        AZ::StringFunc::Replace(result, ":", "");
        AZ::StringFunc::Replace(result, "<", "");
        AZ::StringFunc::Replace(result, ">", "");
        AZ::StringFunc::Replace(result, ",", "");
        AZ::StringFunc::Replace(result, ".", "");
        AZ::StringFunc::Replace(result, "=", "");
        AZ::StringFunc::Replace(result, "!", "");
        AZ::StringFunc::Strip(result, " ");
        return result;
    }

    AZStd::string SanitizeCustomNodeFileName(const AZStd::string& nodeName, const AZ::Uuid& nodeUuid)
    {
        AZStd::string sanitizedNodeName = SanitizeText(nodeName);
        AZ::Uuid::FixedString nodeUuidName = nodeUuid.ToFixedString(false);

        AZStd::string result = AZStd::string::format("%s_%s", sanitizedNodeName.c_str(), nodeUuidName.c_str());
        AZ::StringFunc::Path::Normalize(result);
        return result;
    }

    AZStd::string GetSafeTypeName(ScriptCanvas::Data::Type dataType)
    {
        if (!dataType.IsValid())
        {
            return "";
        }

        AZStd::string azType = dataType.GetAZType().ToString<AZStd::string>();

        GraphCanvas::TranslationKey key;
        key << "BehaviorType" << azType << "details";

        GraphCanvas::TranslationRequests::Details details;

        details.m_name = ScriptCanvas::Data::GetName(dataType);

        GraphCanvas::TranslationRequestBus::BroadcastResult(details, &GraphCanvas::TranslationRequests::GetDetails, key, details);

        return details.m_name;
    }
} // namespace ScriptCanvasEditor::TranslationHelper
