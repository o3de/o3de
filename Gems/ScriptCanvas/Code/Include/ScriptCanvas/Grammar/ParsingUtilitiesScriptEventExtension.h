#include "Primitives.h"
#include "PrimitivesExecution.h"

namespace ScriptCanvas
{
    namespace Nodes::Core
    {
        class FunctionDefinitionNode;
    }

    namespace ScriptEventGrammar
    {
        struct FunctionNodeToScriptEventResult
        {
            bool m_isScriptEvent = false;
            AZStd::vector<AZStd::string> m_reasons;
        };

        FunctionNodeToScriptEventResult IsScriptEvent(const Nodes::Core::FunctionDefinitionNode& node);
    }
}
