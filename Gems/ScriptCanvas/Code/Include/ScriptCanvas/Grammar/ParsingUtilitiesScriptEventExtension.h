#include "Primitives.h"
#include "PrimitivesExecution.h"

namespace ScriptCanvas
{
    class Node;

    namespace Nodes::Core
    {
        class FunctionDefinitionNode;
    }

    namespace ScriptEventGrammar
    {
        struct FunctionNodeToScriptEventResult
        {
            bool m_isScriptEvent = false;
            const Node* m_node = nullptr;
            AZStd::vector<AZStd::string> m_parseErrors;
            ScriptEvents::Method m_method;
        };

        struct GraphToScriptEventsResult
        {
            bool m_isScriptEvents = false;
            SourceHandle m_graph;
            AZStd::vector<AZStd::string> m_parseErrors;
            AZStd::vector<FunctionNodeToScriptEventResult> m_nodeResults;
            ScriptEvents::ScriptEvent m_event;
        };

        GraphToScriptEventsResult ParseMinimumScriptEventArtifacts(Graph& graph);

        FunctionNodeToScriptEventResult ParseScriptEvent(const Node& node);

        GraphToScriptEventsResult ParseScriptEventsDefinition(Graph& graph);
    }
}

namespace ScriptEvents
{
    void AddScriptEventHelpers(ScriptCanvas::Graph& graph);
}
