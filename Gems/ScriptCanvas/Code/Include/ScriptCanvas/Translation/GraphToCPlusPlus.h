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

#include <AzCore/Outcome/Outcome.h>

#include "TranslationUtilities.h"
#include "GraphToX.h"

namespace ScriptCanvas
{
    class Graph;

    namespace Grammar
    {
        class AbstractCodeModel;
        struct Function;
    }

    namespace Translation
    {
        class GraphToCPlusPlus
            : public GraphToX
        {
        public:
            static AZ::Outcome<void, AZStd::string> Translate(const Grammar::AbstractCodeModel& model, AZStd::string& dotH, AZStd::string& dotCPP);
            
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
            void WriterHeader(); // Write, not translate, because this should be less dependent on the contents of the graph
            void WriteHeaderDotH(); // Write, not translate, because this should be less dependent on the contents of the graph
            void WriteHeaderDotCPP(); // Write, not translate, because this should be less dependent on the contents of the graph
        };
    } // namespace Translation

} // namespace ScriptCanvas