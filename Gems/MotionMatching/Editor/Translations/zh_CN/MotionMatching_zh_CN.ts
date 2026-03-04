<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="zh_CN">
<context>
    <name>MotionMatching</name>
    <message>
        <location filename="../../../Code/Source/BlendTreeMotionMatchNode.cpp" line="411"/>
        <source>Motion Matching Node</source>
        <translation>动作匹配节点</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/BlendTreeMotionMatchNode.cpp" line="412"/>
        <source>Motion Matching Attributes</source>
        <translation>动作匹配属性</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/BlendTreeMotionMatchNode.cpp" line="417"/>
        <source>Search frequency</source>
        <translation>搜索频率</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/BlendTreeMotionMatchNode.cpp" line="418"/>
        <source>How often per second we apply the motion matching search and find the lowest cost / best matching frame, and start to blend towards it.</source>
        <translation>每秒执行动作匹配搜索并找到最低代价/最佳匹配帧、开始混合过渡的频率。</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/BlendTreeMotionMatchNode.cpp" line="423"/>
        <source>Feature sample rate</source>
        <translation>特征采样率</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/BlendTreeMotionMatchNode.cpp" line="424"/>
        <source>The sample rate (in Hz) used for extracting the features from the animations. The higher the sample rate, the more data will be used and the more options the motion matching search has available for the best matching frame.</source>
        <translation>从动画中提取特征时使用的采样率（单位：Hz）。采样率越高，使用的数据越多，动作匹配搜索可用于寻找最佳匹配帧的选项也越多。</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/BlendTreeMotionMatchNode.cpp" line="429"/>
        <source>Trajectory Prediction</source>
        <translation>轨迹预测</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/BlendTreeMotionMatchNode.cpp" line="430"/>
        <source>Desired future trajectory generation mode.</source>
        <translation>期望的未来轨迹生成模式。</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/BlendTreeMotionMatchNode.cpp" line="433"/>
        <source>Target-driven</source>
        <translation>目标驱动</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/BlendTreeMotionMatchNode.cpp" line="435"/>
        <source>Automatic (Demo)</source>
        <translation>自动（演示）</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/BlendTreeMotionMatchNode.cpp" line="437"/>
        <source>Path radius</source>
        <translation>路径半径</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/BlendTreeMotionMatchNode.cpp" line="443"/>
        <source>Path speed</source>
        <translation>路径速度</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/BlendTreeMotionMatchNode.cpp" line="449"/>
        <source>Data Normalization</source>
        <translation>数据归一化</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/BlendTreeMotionMatchNode.cpp" line="452"/>
        <source>Normalize Data</source>
        <translation>归一化数据</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/BlendTreeMotionMatchNode.cpp" line="453"/>
        <source>Normalize feature data for more intuitive control over weighting the cost factors.</source>
        <translation>归一化特征数据，以便更直观地控制代价因子的权重。</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/BlendTreeMotionMatchNode.cpp" line="457"/>
        <source>Type</source>
        <translation>类型</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/BlendTreeMotionMatchNode.cpp" line="458"/>
        <source>Feature scaler type to be used to normalize the data.</source>
        <translation>用于归一化数据的特征缩放器类型。</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/BlendTreeMotionMatchNode.cpp" line="463"/>
        <source>Standard Scaler</source>
        <translation>标准缩放器</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/BlendTreeMotionMatchNode.cpp" line="465"/>
        <source>Min-max Scaler</source>
        <translation>最小-最大缩放器</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/BlendTreeMotionMatchNode.cpp" line="467"/>
        <source>Feature Minimum</source>
        <translation>特征最小值</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/BlendTreeMotionMatchNode.cpp" line="468"/>
        <source>Minimum value after data transformation.</source>
        <translation>数据变换后的最小值。</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/BlendTreeMotionMatchNode.cpp" line="472"/>
        <source>Feature Maximum</source>
        <translation>特征最大值</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/BlendTreeMotionMatchNode.cpp" line="473"/>
        <source>Maximum value after data transformation.</source>
        <translation>数据变换后的最大值。</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/BlendTreeMotionMatchNode.cpp" line="477"/>
        <source>Clip Features</source>
        <translation>裁剪特征</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/BlendTreeMotionMatchNode.cpp" line="478"/>
        <source>Clip feature values for outliers to the above range.</source>
        <translation>将异常值的特征值裁剪到上述范围内。</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/BlendTreeMotionMatchNode.cpp" line="482"/>
        <source>Acceleration Structure</source>
        <translation>加速结构</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/BlendTreeMotionMatchNode.cpp" line="485"/>
        <source>Max kd-tree depth</source>
        <translation>最大 KD 树深度</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/BlendTreeMotionMatchNode.cpp" line="486"/>
        <source>The maximum number of hierarchy levels in the kdTree.</source>
        <translation>KD 树中的最大层级数。</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/BlendTreeMotionMatchNode.cpp" line="491"/>
        <source>Min kd-tree node size</source>
        <translation>最小 KD 树节点大小</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/BlendTreeMotionMatchNode.cpp" line="492"/>
        <source>The minimum number of frames to store per kdTree node.</source>
        <translation>每个 KD 树节点存储的最小帧数。</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/BlendTreeMotionMatchNode.cpp" line="498"/>
        <location filename="../../../Code/Source/FeatureSchema.cpp" line="135"/>
        <source>FeatureSchema</source>
        <translation>特征方案</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/BlendTreeMotionMatchNode.cpp" line="504"/>
        <source>Motions</source>
        <translation>动作</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/EventData.cpp" line="45"/>
        <source>[Motion Matching] Discard Frame</source>
        <translation>[动作匹配] 丢弃帧</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/EventData.cpp" line="46"/>
        <source>Event used for discarding ranges of the animation..</source>
        <translation>用于丢弃动画范围的事件。</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/EventData.cpp" line="88"/>
        <source>[Motion Matching] Tag</source>
        <translation>[动作匹配] 标签</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/EventData.cpp" line="94"/>
        <source>Tag</source>
        <translation>标签</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/EventData.cpp" line="95"/>
        <source>The tag that should be active.</source>
        <translation>应激活的标签。</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Feature.cpp" line="173"/>
        <source>Feature</source>
        <translation>特征</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Feature.cpp" line="174"/>
        <source>Base class for a feature</source>
        <translation>特征的基类</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Feature.cpp" line="178"/>
        <source>Name</source>
        <translation>名称</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Feature.cpp" line="179"/>
        <source>Custom name of the feature used for identification and debug visualizations.</source>
        <translation>用于识别和调试可视化的特征自定义名称。</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Feature.cpp" line="181"/>
        <source>Joint</source>
        <translation>关节</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Feature.cpp" line="182"/>
        <source>The joint to extract the data from.</source>
        <translation>要从中提取数据的关节。</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Feature.cpp" line="184"/>
        <source>Relative To Joint</source>
        <translation>相对于关节</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Feature.cpp" line="185"/>
        <source>When extracting feature data, convert it to relative-space to the given joint.</source>
        <translation>提取特征数据时，将其转换为相对于给定关节的相对空间。</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Feature.cpp" line="187"/>
        <source>Debug Draw</source>
        <translation>调试绘制</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Feature.cpp" line="188"/>
        <source>Are debug visualizations enabled for this feature?</source>
        <translation>是否为此特征启用调试可视化？</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Feature.cpp" line="190"/>
        <source>Debug Draw Color</source>
        <translation>调试绘制颜色</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Feature.cpp" line="191"/>
        <source>Color used for debug visualizations to identify the feature.</source>
        <translation>用于调试可视化以识别特征的颜色。</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Feature.cpp" line="193"/>
        <source>Cost Factor</source>
        <translation>代价因子</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Feature.cpp" line="194"/>
        <source>The cost factor for the feature is multiplied with the actual and can be used to change a feature&apos;s influence in the motion matching search.</source>
        <translation>特征的代价因子会与实际值相乘，可用于改变特征在动作匹配搜索中的影响力。</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Feature.cpp" line="200"/>
        <source>Residual</source>
        <translation>残差</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Feature.cpp" line="201"/>
        <source>Use &apos;Squared&apos; in case minimal differences should be ignored and larger differences should overweight others. Use &apos;Absolute&apos; for linear differences and don&apos;t want the mentioned effect.</source>
        <translation>如果希望忽略微小差异并让较大差异占更高权重，请使用&quot;平方&quot;。如果需要线性差异且不希望出现上述效果，请使用&quot;绝对值&quot;。</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Feature.cpp" line="203"/>
        <source>Absolute</source>
        <translation>绝对值</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Feature.cpp" line="205"/>
        <source>Squared</source>
        <translation>平方</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/FeatureAngularVelocity.cpp" line="120"/>
        <source>FeatureAngularVelocity</source>
        <translation>特征角速度</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/FeatureAngularVelocity.cpp" line="121"/>
        <source>Matches joint angular velocities.</source>
        <translation>匹配关节角速度。</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/FeaturePosition.cpp" line="94"/>
        <source>FeaturePosition</source>
        <translation>特征位置</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/FeaturePosition.cpp" line="95"/>
        <source>Matches joint positions.</source>
        <translation>匹配关节位置。</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/FeatureSchema.cpp" line="140"/>
        <source>Features</source>
        <translation>特征</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/FeatureTrajectory.cpp" line="437"/>
        <source>FeatureTrajectory</source>
        <translation>特征轨迹</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/FeatureTrajectory.cpp" line="438"/>
        <source>Matches the joint past and future trajectory.</source>
        <translation>匹配关节的过去和未来轨迹。</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/FeatureTrajectory.cpp" line="443"/>
        <source>Past Samples</source>
        <translation>历史采样数</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/FeatureTrajectory.cpp" line="444"/>
        <source>The number of samples stored per frame for the past trajectory. [Default = 4 samples to represent the trajectory history]</source>
        <translation>每帧存储的历史轨迹采样数量。[默认 = 4 个采样来表示轨迹历史]</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/FeatureTrajectory.cpp" line="450"/>
        <source>Past Time Range</source>
        <translation>历史时间范围</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/FeatureTrajectory.cpp" line="451"/>
        <source>The time window the samples are distributed along for the trajectory history. [Default = 0.7 seconds]</source>
        <translation>采样分布在轨迹历史上的时间窗口。[默认 = 0.7 秒]</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/FeatureTrajectory.cpp" line="457"/>
        <source>Past Cost Factor</source>
        <translation>历史代价因子</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/FeatureTrajectory.cpp" line="458"/>
        <source>The cost factor is multiplied with the cost from the trajectory history and can be used to change the influence of the trajectory history match in the motion matching search.</source>
        <translation>代价因子会与轨迹历史的代价相乘，可用于改变轨迹历史匹配在动作匹配搜索中的影响力。</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/FeatureTrajectory.cpp" line="465"/>
        <source>Future Samples</source>
        <translation>未来采样数</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/FeatureTrajectory.cpp" line="466"/>
        <source>The number of samples stored per frame for the future trajectory. [Default = 6 samples to represent the future trajectory]</source>
        <translation>每帧存储的未来轨迹采样数量。[默认 = 6 个采样来表示未来轨迹]</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/FeatureTrajectory.cpp" line="473"/>
        <source>Future Time Range</source>
        <translation>未来时间范围</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/FeatureTrajectory.cpp" line="474"/>
        <source>The time window the samples are distributed along for the future trajectory. [Default = 1.2 seconds]</source>
        <translation>采样分布在未来轨迹上的时间窗口。[默认 = 1.2 秒]</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/FeatureTrajectory.cpp" line="480"/>
        <source>Future Cost Factor</source>
        <translation>未来代价因子</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/FeatureTrajectory.cpp" line="481"/>
        <source>The cost factor is multiplied with the cost from the future trajectory and can be used to change the influence of the future trajectory match in the motion matching search.</source>
        <translation>代价因子会与未来轨迹的代价相乘，可用于改变未来轨迹匹配在动作匹配搜索中的影响力。</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/FeatureTrajectory.cpp" line="488"/>
        <source>Facing Axis</source>
        <translation>朝向轴</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/FeatureTrajectory.cpp" line="489"/>
        <source>The facing direction of the character. Which axis of the joint transform is facing forward? [Default = Looking into Y-axis direction]</source>
        <translation>角色的朝向方向。关节变换的哪个轴朝前？[默认 = Y 轴方向]</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/FeatureVelocity.cpp" line="123"/>
        <source>FeatureVelocity</source>
        <translation>特征速度</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/FeatureVelocity.cpp" line="124"/>
        <source>Matches joint velocities.</source>
        <translation>匹配关节速度。</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/MotionMatchingSystemComponent.cpp" line="63"/>
        <source>MotionMatching</source>
        <translation>动作匹配</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/MotionMatchingSystemComponent.cpp" line="64"/>
        <source>[Description of functionality provided by this System Component]</source>
        <translation>[此系统组件提供的功能描述]</translation>
    </message>
</context>
</TS>
