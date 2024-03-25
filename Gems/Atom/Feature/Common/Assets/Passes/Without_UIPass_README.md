### About Without_UIPass_MainRenderPipeline.azasset and Without_UIPass_MainPipeline.pass
#### TL;DR  

Do NOT use `Without_UIPass_MainRenderPipeline.azasset` and do NOT use `Without_UIPass_MainPipeline.pass`, unless you want to prove that the Editor doesn't crash when the `UIPass` doesn't exist in the active render pipeline.

#### Long Story
The CVAR `r_renderPipelinePath` can be used to change the active render pipeline for the Editor Viewport. By default the active render pipeline is `Passes/MainRenderPipeline.azasset`, which in turn loads `passes/MainPipeline.pass`. This default pipeline includes a pass called `UIPass`.
  
It was found that when the active render pipeline does not include a pass called `UIPass`, the Editor crashed in the class `DynamicDrawContext.cpp`.  
The functions that were crashing were modified with `AZ_WarningOnce` instead of `AZ_Assert`. And to further prove that the Editor should not crash anymore, the files `Without_UIPass_MainRenderPipeline.azasset` and `Without_UIPass_MainPipeline.pass` were added for debugging purposes.

In the command line arguments you can add:  
`-r_renderPipelinePath=passes/without_uipass_mainrenderpipeline.azasset`  
and it will change the default render pipeline to an almost identical pipeline except for the fact that the `UIPass` won't exist and hence you won't be able to
see overlayed data like `Frames Per Second`, nor you'll be able to see Game UI from the LyShine Gem.
