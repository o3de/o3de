/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Outcome/Outcome.h>

#include "TranslationUtilities.h"
#include "GraphToX.h"

namespace ScriptCanvas
{
    class Graph;

    namespace Grammar
    {
        class AbstractCodeModel;
    }

    namespace Translation
    {
        class GraphToCPlusPlus
            : public GraphToX
        {
        public:
            static AZ::Outcome<void, AZStd::pair<AZStd::string, AZStd::string>> Translate(const Grammar::AbstractCodeModel& model, AZStd::string& dotH, AZStd::string& dotCPP);
            
            AZ_INLINE bool IsSuccessfull() const { return false;  }
        private:         
            // cpp only
            Writer m_dotH;
            Writer m_dotCPP;
            
            GraphToCPlusPlus(const Grammar::AbstractCodeModel& model);

            void TranslateClassClose();
            void TranslateClassOpen();
            void TranslateConstruction();
            void TranslateDependencies();
            void TranslateDependenciesDotH();
            void TranslateDependenciesDotCPP();
            void TranslateDestruction();
            void TranslateFunctions();
            void TranslateHandlers();
            void TranslateNamespaceOpen();
            void TranslateNamespaceClose();
            void TranslateStartNode();
            void TranslateVariables();
            void WriteHeader(); // Write, not translate, because this should be less dependent on the contents of the graph
            void WriteHeaderDotH(); // Write, not translate, because this should be less dependent on the contents of the graph
            void WriteHeaderDotCPP(); // Write, not translate, because this should be less dependent on the contents of the graph
        };
    } 

}
