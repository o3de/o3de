<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="zh_CN">
<context>
    <name>MaterialCanvas::MaterialCanvasMainWindow</name>
    <message>
        <location filename="../../../Code/Source/Window/MaterialCanvasMainWindow.cpp" line="29"/>
        <source>Material</source>
        <translation>材质</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Window/MaterialCanvasMainWindow.cpp" line="30"/>
        <source>Material Graph</source>
        <translation>材质图</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Window/MaterialCanvasMainWindow.cpp" line="31"/>
        <source>Material Graph Node</source>
        <translation>材质图节点</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Window/MaterialCanvasMainWindow.cpp" line="32"/>
        <source>Material Type</source>
        <translation>材质类型</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Window/MaterialCanvasMainWindow.cpp" line="33"/>
        <source>Material Pipeline</source>
        <translation>材质管线</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Window/MaterialCanvasMainWindow.cpp" line="34"/>
        <source>Shader</source>
        <translation>着色器</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Window/MaterialCanvasMainWindow.cpp" line="35"/>
        <source>Shader Template</source>
        <translation>着色器模板</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Window/MaterialCanvasMainWindow.cpp" line="36"/>
        <source>Shader Variant List</source>
        <translation>着色器变体列表</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Window/MaterialCanvasMainWindow.cpp" line="37"/>
        <source>Image</source>
        <translation>图像</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Window/MaterialCanvasMainWindow.cpp" line="38"/>
        <source>Lua</source>
        <translation>Lua</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Window/MaterialCanvasMainWindow.cpp" line="39"/>
        <source>AZSL</source>
        <translation>AZSL</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Window/MaterialCanvasMainWindow.cpp" line="44"/>
        <source>Inspector</source>
        <translation>检查器</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Window/MaterialCanvasMainWindow.cpp" line="77"/>
        <source>Viewport</source>
        <translation>视口</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Window/MaterialCanvasMainWindow.cpp" line="80"/>
        <location filename="../../../Code/Source/Window/MaterialCanvasMainWindow.cpp" line="81"/>
        <source>Viewport Settings</source>
        <translation>视口设置</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Window/MaterialCanvasMainWindow.cpp" line="84"/>
        <location filename="../../../Code/Source/Window/MaterialCanvasMainWindow.cpp" line="85"/>
        <source>Bookmarks</source>
        <translation>书签</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Window/MaterialCanvasMainWindow.cpp" line="87"/>
        <location filename="../../../Code/Source/Window/MaterialCanvasMainWindow.cpp" line="88"/>
        <source>MiniMap</source>
        <translation>小地图</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Window/MaterialCanvasMainWindow.cpp" line="97"/>
        <location filename="../../../Code/Source/Window/MaterialCanvasMainWindow.cpp" line="98"/>
        <source>Node Palette</source>
        <translation>节点面板</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Window/MaterialCanvasMainWindow.cpp" line="162"/>
        <location filename="../../../Code/Source/Window/MaterialCanvasMainWindow.cpp" line="163"/>
        <source>Material Canvas Settings</source>
        <translation>材质画布设置</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Window/MaterialCanvasMainWindow.cpp" line="166"/>
        <source>Enable Faster Shader Builds</source>
        <translation>启用更快的着色器构建</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Window/MaterialCanvasMainWindow.cpp" line="167"/>
        <source>By default, some platforms perform an exhaustive compilation of shaders for multiple RHI. For example, the default Windows shader builder settings automatically compiles shaders for DX12, Vulkan, and the Null renderer.

This option overrides those registry settings and makes compilation and preview times much faster by only compiling shaders for the currently active platform and RHI.

This also disables automatic shader variant generation.

Changing this setting requires restarting Material Canvas and the Asset Processor.

Changing the active RHI with this setting enabled may require clearing the cache to regenerate shaders for the new RHI.

The settings files containing the overrides will be placed in the user/Registry folder for the current project.</source>
        <translation>默认情况下，某些平台会对多个 RHI 的着色器执行全面编译。例如，默认的 Windows 着色器构建器设置会自动为 DX12、Vulkan 和空渲染器编译着色器。

此选项将覆盖这些注册表设置，仅为当前活动的平台和 RHI 编译着色器，从而大大加快编译和预览速度。

此选项还会禁用自动着色器变体生成。

更改此设置需要重新启动材质画布和资产处理器。

在启用此设置的情况下更改活动 RHI，可能需要清除缓存以便为新的 RHI 重新生成着色器。

包含覆盖设置的文件将放置在当前项目的 user/Registry 文件夹中。</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Window/MaterialCanvasMainWindow.cpp" line="177"/>
        <source>Delete Files On Compile</source>
        <translation>编译时删除文件</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Window/MaterialCanvasMainWindow.cpp" line="178"/>
        <source>This option forces files previously generated from the current graph to be deleted before creating new ones.</source>
        <translation>此选项会在创建新文件之前，强制删除当前图之前生成的文件。</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Window/MaterialCanvasMainWindow.cpp" line="182"/>
        <source>Clear Asset Fingerprints On Compile</source>
        <translation>编译时清除资产指纹</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Window/MaterialCanvasMainWindow.cpp" line="183"/>
        <source>This option forces the AP to reprocess generated files even if no differences were detected since last generated. This guarantees that notifications are sent for assets like materials that may not be changed even if their dependent material types or shaders are. This setting is most useful to ensure that other systems or applications are able to recognize and not reload yeah materials after shaders are modified. Enabling this setting may affect the time it takes for the viewport to reflect shader and material changes.</source>
        <translation>此选项会强制资产处理器重新处理已生成的文件，即使自上次生成以来未检测到差异。这可确保即使材质类型或着色器等依赖项已更改，也能为可能未更改的材质等资产发送通知。此设置对于确保其他系统或应用程序能够在着色器修改后识别并重新加载材质非常有用。启用此设置可能会影响视口反映着色器和材质更改所需的时间。</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Window/MaterialCanvasMainWindow.cpp" line="191"/>
        <source>Enable Compile On Open</source>
        <translation>启用打开时编译</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Window/MaterialCanvasMainWindow.cpp" line="192"/>
        <source>If enabled, shaders and materials will automatically be generated whenever a material graph is opened.</source>
        <translation>如果启用，每当打开材质图时将自动生成着色器和材质。</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Window/MaterialCanvasMainWindow.cpp" line="196"/>
        <source>Enable Compile On Save</source>
        <translation>启用保存时编译</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Window/MaterialCanvasMainWindow.cpp" line="197"/>
        <source>If enabled, shaders and materials will automatically be generated whenever a material graph is saved.</source>
        <translation>如果启用，每当保存材质图时将自动生成着色器和材质。</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Window/MaterialCanvasMainWindow.cpp" line="201"/>
        <source>Enable Compile On Edit</source>
        <translation>启用编辑时编译</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Window/MaterialCanvasMainWindow.cpp" line="202"/>
        <source>If enabled, shaders and materials will automatically be generated whenever a material graph is edited.</source>
        <translation>如果启用，每当编辑材质图时将自动生成着色器和材质。</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Window/MaterialCanvasMainWindow.cpp" line="206"/>
        <source>Clear Viewport Material When Compiling Starts</source>
        <translation>编译开始时清除视口材质</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Window/MaterialCanvasMainWindow.cpp" line="207"/>
        <source>Clear the viewport model&apos;s material whenever compiling shaders and materials starts.</source>
        <translation>每当开始编译着色器和材质时，清除视口模型的材质。</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Window/MaterialCanvasMainWindow.cpp" line="211"/>
        <source>Clear Viewport Material When Compiling Fails</source>
        <translation>编译失败时清除视口材质</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Window/MaterialCanvasMainWindow.cpp" line="212"/>
        <source>Clear the viewport model&apos;s material whenever compiling shaders and materials fails.</source>
        <translation>每当编译着色器和材质失败时，清除视口模型的材质。</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Window/MaterialCanvasMainWindow.cpp" line="216"/>
        <source>Enable Compiler Logging</source>
        <translation>启用编译器日志</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Window/MaterialCanvasMainWindow.cpp" line="217"/>
        <source>Toggle verbose logging for material graph generation.</source>
        <translation>切换材质图生成的详细日志记录。</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Window/MaterialCanvasMainWindow.cpp" line="221"/>
        <source>Enable Property Editing On Nodes</source>
        <translation>启用节点上的属性编辑</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Window/MaterialCanvasMainWindow.cpp" line="222"/>
        <source>Toggle settings to display properties and allow them to be added directly on graph nodes.</source>
        <translation>切换设置以显示属性并允许直接在图节点上添加属性。</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Window/MaterialCanvasMainWindow.cpp" line="226"/>
        <source>Create Untitled Graph Document On Start</source>
        <translation>启动时创建无标题图文档</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Window/MaterialCanvasMainWindow.cpp" line="227"/>
        <source>Create a default, untitled graph document when Material Canvas starts.</source>
        <translation>在材质画布启动时创建一个默认的无标题图文档。</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Window/MaterialCanvasMainWindow.cpp" line="231"/>
        <source>Queue Graph Compile Interval Ms</source>
        <translation>图编译队列间隔（毫秒）</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Window/MaterialCanvasMainWindow.cpp" line="232"/>
        <source>The delay (in milliseconds) before the graph is recompiled after changes.</source>
        <translation>更改后重新编译图之前的延迟时间（以毫秒为单位）。</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Window/MaterialCanvasMainWindow.cpp" line="247"/>
        <location filename="../../../Code/Source/Window/MaterialCanvasMainWindow.cpp" line="248"/>
        <source>Graph View Settings</source>
        <translation>图视图设置</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Window/MaterialCanvasMainWindow.cpp" line="249"/>
        <source>Configuration settings for the graph view interaction, animation, and other behavior.</source>
        <translation>图视图交互、动画和其他行为的配置设置。</translation>
    </message>
</context>
<context>
    <name>MaterialCanvasApplication</name>
    <message>
        <location filename="../../../Code/Source/MaterialCanvasApplication.cpp" line="67"/>
        <source>O3DE Material Canvas</source>
        <translation>O3DE 材质画布</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/MaterialCanvasApplication.cpp" line="259"/>
        <source>Template File</source>
        <translation>模板文件</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/MaterialCanvasApplication.cpp" line="266"/>
        <source>Include File</source>
        <translation>包含文件</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/MaterialCanvasApplication.cpp" line="354"/>
        <source>Material Graph Node Config properties can be edited in the inspector.</source>
        <translation>材质图节点配置属性可在检查器中编辑。</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/MaterialCanvasApplication.cpp" line="376"/>
        <source>Shader Source Data properties can be edited in the inspector.</source>
        <translation>着色器源数据属性可在检查器中编辑。</translation>
    </message>
</context>
</TS>
