
#include "EditorWhiteBoxComponentModeCommon.h"
#include "EditorWhiteBoxTransformMode.h"
#include "Viewport/WhiteBoxModifierUtil.h"
#include "Viewport/WhiteBoxViewportConstants.h"

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/std/sort.h>
#include <AzToolsFramework/ComponentMode/EditorComponentModeBus.h>
#include <AzToolsFramework/Maths/TransformUtils.h>
#include <AzToolsFramework/Viewport/ViewportTypes.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>
#include <WhiteBox/EditorWhiteBoxComponentBus.h>

namespace WhiteBox
{
        AZ_CLASS_ALLOCATOR_IMPL(TransformMode, AZ::SystemAllocator, 0)

        void TransformMode::Refresh() {

        }

        AZStd::vector<AzToolsFramework::ActionOverride> TransformMode::PopulateActions(
            const AZ::EntityComponentIdPair& entityComponentIdPair) {
            return {};
        }
        void TransformMode::Display(
            const AZ::EntityComponentIdPair& entityComponentIdPair, const AZ::Transform& worldFromLocal,
            const IntersectionAndRenderData& renderData, const AzFramework::ViewportInfo& viewportInfo,
            AzFramework::DebugDisplayRequests& debugDisplay) {
            
            }
        bool TransformMode::HandleMouseInteraction(
            const AzToolsFramework::ViewportInteraction::MouseInteractionEvent& mouseInteraction,
            const AZ::EntityComponentIdPair& entityComponentIdPair,
            const AZStd::optional<EdgeIntersection>& edgeIntersection,
            const AZStd::optional<PolygonIntersection>& polygonIntersection,
            const AZStd::optional<VertexIntersection>& vertexIntersection) {
                return false;
            }
} // namespace Whitebox