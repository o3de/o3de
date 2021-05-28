README_PhysXDebug

Introduction:

The PhysX Debug Gem will visualize PhysX primitives and other Debug functionalities.

Dependencies:
  - LY minimum v 1.14
  - PhysX Gem
  - ImGui Gem

Setup:
  - Enable Gem and enable via ImGui or Console Commands.
  
Console Commands:
  - The gem is configured via ImGui PhysX Debug menu in game mode and console commands, possible commands:
  

physx_Debug <preference>

Using the following integer preference.
1 - Debug visualizations (by default will enable the basic visualization of: physx::eCOLLISION_SHAPES, physx::eCOLLISION_EDGES). [1]
2 - Enable all configuration options (enabling all of the following: physx::PxVisualizationParameter.*). [1]
0 - Disable debug visualisations.

physx_CullingBox <preference>
Turn on/off a visual culling box frame.

physx_CullingBoxSize <100>
Adjust culling box size to 100. Use 0 to disable culling.

physx_PvdConnect <100>
Connect to the PhysX Visual Debugger.

physx_PvdDisconnect
Disconnect from the PhysX Visual Debugger.

[1] https://docs.nvidia.com/gameworks/content/gameworkslibrary/physx/guide/Manual/VisualDebugger.html#physxvisualdebugger
