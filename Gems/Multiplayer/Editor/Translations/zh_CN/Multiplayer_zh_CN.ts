<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="zh_CN">
<context>
    <name>Multiplayer</name>
    <message>
        <location filename="../../../Code/Source/Components/NetBindComponent.cpp" line="46"/>
        <source>Network Binding</source>
        <translation>网络绑定</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Components/NetBindComponent.cpp" line="47"/>
        <source>The Network Binding component marks an entity as able to be replicated across the network</source>
        <translation>网络绑定组件将实体标记为可通过网络进行复制</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Components/NetworkDebugPlayerIdComponent.cpp" line="37"/>
        <source>Network Debug Player ID</source>
        <translation>网络调试玩家ID</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Components/NetworkDebugPlayerIdComponent.cpp" line="38"/>
        <source>Renders the player id as debug text over network players.</source>
        <translation>在网络玩家上方以调试文本形式渲染玩家ID。</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Components/NetworkDebugPlayerIdComponent.cpp" line="43"/>
        <source>Translation Offset</source>
        <translation>平移偏移</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Components/NetworkDebugPlayerIdComponent.cpp" line="44"/>
        <source>The world-space offset from the player position to render the debug text.</source>
        <translation>从玩家位置到渲染调试文本位置的世界空间偏移量。</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Components/NetworkDebugPlayerIdComponent.cpp" line="46"/>
        <source>Font Scale</source>
        <translation>字体缩放</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Components/NetworkDebugPlayerIdComponent.cpp" line="47"/>
        <source>Apply a scale to the default debug font rendering size.</source>
        <translation>对默认调试字体渲染大小应用缩放。</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Components/NetworkDebugPlayerIdComponent.cpp" line="49"/>
        <source>Color</source>
        <translation>颜色</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Components/NetworkDebugPlayerIdComponent.cpp" line="50"/>
        <source>Debug text color.</source>
        <translation>调试文本颜色。</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Components/NetworkHierarchyChildComponent.cpp" line="31"/>
        <source>Network Hierarchy Child</source>
        <translation>网络层级子节点</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Components/NetworkHierarchyChildComponent.cpp" line="32"/>
        <source>Declares a network dependency on the root of this hierarchy.</source>
        <translation>声明对此层级根节点的网络依赖。</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Components/NetworkHierarchyRootComponent.cpp" line="39"/>
        <source>Network Hierarchy Root</source>
        <translation>网络层级根节点</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Components/NetworkHierarchyRootComponent.cpp" line="40"/>
        <source>Marks the entity as the root of an entity hierarchy.</source>
        <translation>将实体标记为实体层级的根节点。</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Components/SimplePlayerSpawnerComponent.cpp" line="45"/>
        <source>Simple Network Player Spawner</source>
        <translation>简单网络玩家生成器</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Components/SimplePlayerSpawnerComponent.cpp" line="46"/>
        <source>A simple player spawner that comes included with the Multiplayer gem. Attach this component to any level&apos;s root entity which needs to spawn a network player.If no spawn points are provided the network players will be spawned at the world-space origin.</source>
        <translation>多人游戏Gem中附带的简单玩家生成器。将此组件附加到任何需要生成网络玩家的关卡根实体上。如果未提供生成点，网络玩家将在世界空间原点处生成。</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Components/SimplePlayerSpawnerComponent.cpp" line="56"/>
        <source>Player Spawnable Asset</source>
        <translation>玩家可生成资产</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Components/SimplePlayerSpawnerComponent.cpp" line="57"/>
        <source>The network player spawnable asset which will be spawned for each player that joins.</source>
        <translation>将为每个加入的玩家生成的网络玩家可生成资产。</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Components/SimplePlayerSpawnerComponent.cpp" line="61"/>
        <source>Spawn Points</source>
        <translation>生成点</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Components/SimplePlayerSpawnerComponent.cpp" line="62"/>
        <source>Networked players will spawn at the spawn point locations in order. If there are more players than spawn points, the new players will round-robin back starting with the first spawn point.</source>
        <translation>网络玩家将按顺序在生成点位置生成。如果玩家数量超过生成点数量，新玩家将从第一个生成点开始循环使用。</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Session/MatchmakingRequests.cpp" line="30"/>
        <source>AcceptMatchRequest</source>
        <translation>接受匹配请求</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Session/MatchmakingRequests.cpp" line="31"/>
        <source>The container for AcceptMatch request parameters</source>
        <translation>接受匹配请求参数的容器</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Session/MatchmakingRequests.cpp" line="35"/>
        <source>AcceptMatch</source>
        <translation>接受匹配</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Session/MatchmakingRequests.cpp" line="36"/>
        <source>Player response to accept or reject match</source>
        <translation>玩家接受或拒绝匹配的响应</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Session/MatchmakingRequests.cpp" line="38"/>
        <source>PlayerIds</source>
        <translation>玩家ID列表</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Session/MatchmakingRequests.cpp" line="39"/>
        <source>A list of unique identifiers for players delivering the response</source>
        <translation>提交响应的玩家的唯一标识符列表</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Session/MatchmakingRequests.cpp" line="41"/>
        <location filename="../../../Code/Source/Session/MatchmakingRequests.cpp" line="63"/>
        <location filename="../../../Code/Source/Session/MatchmakingRequests.cpp" line="85"/>
        <source>TicketId</source>
        <translation>票据ID</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Session/MatchmakingRequests.cpp" line="42"/>
        <location filename="../../../Code/Source/Session/MatchmakingRequests.cpp" line="64"/>
        <location filename="../../../Code/Source/Session/MatchmakingRequests.cpp" line="86"/>
        <source>A unique identifier for a matchmaking ticket</source>
        <translation>匹配票据的唯一标识符</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Session/MatchmakingRequests.cpp" line="58"/>
        <source>StartMatchmakingRequest</source>
        <translation>开始匹配请求</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Session/MatchmakingRequests.cpp" line="59"/>
        <source>The container for StartMatchmaking request parameters</source>
        <translation>开始匹配请求参数的容器</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Session/MatchmakingRequests.cpp" line="80"/>
        <source>StopMatchmakingRequest</source>
        <translation>停止匹配请求</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Session/MatchmakingRequests.cpp" line="81"/>
        <source>The container for StopMatchmaking request parameters</source>
        <translation>停止匹配请求参数的容器</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Session/SessionConfig.cpp" line="41"/>
        <source>SessionConfig</source>
        <translation>会话配置</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Session/SessionConfig.cpp" line="42"/>
        <source>Properties describing a session</source>
        <translation>描述会话的属性</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Session/SessionConfig.cpp" line="46"/>
        <source>CreationTime</source>
        <translation>创建时间</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Session/SessionConfig.cpp" line="47"/>
        <source>A time stamp indicating when this session was created. Format is a number expressed in Unix time as milliseconds.</source>
        <translation>表示会话创建时间的时间戳。格式为以毫秒表示的Unix时间数值。</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Session/SessionConfig.cpp" line="49"/>
        <source>TerminationTime</source>
        <translation>终止时间</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Session/SessionConfig.cpp" line="50"/>
        <source>A time stamp indicating when this data object was terminated. Same format as creation time.</source>
        <translation>表示此数据对象终止时间的时间戳。格式与创建时间相同。</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Session/SessionConfig.cpp" line="52"/>
        <location filename="../../../Code/Source/Session/SessionRequests.cpp" line="38"/>
        <source>CreatorId</source>
        <translation>创建者ID</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Session/SessionConfig.cpp" line="53"/>
        <source>A unique identifier for a player or entity creating the session.</source>
        <translation>创建会话的玩家或实体的唯一标识符。</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Session/SessionConfig.cpp" line="55"/>
        <location filename="../../../Code/Source/Session/SessionRequests.cpp" line="41"/>
        <source>SessionProperties</source>
        <translation>会话属性</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Session/SessionConfig.cpp" line="56"/>
        <source>A collection of custom properties for a session.</source>
        <translation>会话的自定义属性集合。</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Session/SessionConfig.cpp" line="58"/>
        <source>MatchmakingData</source>
        <translation>匹配数据</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Session/SessionConfig.cpp" line="59"/>
        <source>The matchmaking process information that was used to create the session.</source>
        <translation>用于创建会话的匹配过程信息。</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Session/SessionConfig.cpp" line="61"/>
        <location filename="../../../Code/Source/Session/SessionRequests.cpp" line="137"/>
        <source>SessionId</source>
        <translation>会话ID</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Session/SessionConfig.cpp" line="62"/>
        <source>A unique identifier for the session.</source>
        <translation>会话的唯一标识符。</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Session/SessionConfig.cpp" line="64"/>
        <location filename="../../../Code/Source/Session/SessionRequests.cpp" line="44"/>
        <source>SessionName</source>
        <translation>会话名称</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Session/SessionConfig.cpp" line="65"/>
        <source>A descriptive label that is associated with a session.</source>
        <translation>与会话关联的描述性标签。</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Session/SessionConfig.cpp" line="67"/>
        <source>DnsName</source>
        <translation>DNS名称</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Session/SessionConfig.cpp" line="68"/>
        <source>The DNS identifier assigned to the instance that is running the session.</source>
        <translation>分配给运行会话的实例的DNS标识符。</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Session/SessionConfig.cpp" line="70"/>
        <source>IpAddress</source>
        <translation>IP地址</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Session/SessionConfig.cpp" line="71"/>
        <source>The IP address of the session.</source>
        <translation>会话的IP地址。</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Session/SessionConfig.cpp" line="73"/>
        <source>Port</source>
        <translation>端口</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Session/SessionConfig.cpp" line="74"/>
        <source>The port number for the session.</source>
        <translation>会话的端口号。</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Session/SessionConfig.cpp" line="76"/>
        <location filename="../../../Code/Source/Session/SessionRequests.cpp" line="47"/>
        <source>MaxPlayer</source>
        <translation>最大玩家数</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Session/SessionConfig.cpp" line="77"/>
        <source>The maximum number of players that can be connected simultaneously to the session.</source>
        <translation>可同时连接到会话的最大玩家数量。</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Session/SessionConfig.cpp" line="79"/>
        <source>CurrentPlayer</source>
        <translation>当前玩家数</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Session/SessionConfig.cpp" line="80"/>
        <source>Number of players currently in the session.</source>
        <translation>当前会话中的玩家数量。</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Session/SessionConfig.cpp" line="82"/>
        <source>Status</source>
        <translation>状态</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Session/SessionConfig.cpp" line="83"/>
        <source>Current status of the session.</source>
        <translation>会话的当前状态。</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Session/SessionConfig.cpp" line="85"/>
        <source>StatusReason</source>
        <translation>状态原因</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Session/SessionConfig.cpp" line="86"/>
        <source>Provides additional information about session status.</source>
        <translation>提供有关会话状态的附加信息。</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Session/SessionRequests.cpp" line="33"/>
        <source>CreateSessionRequest</source>
        <translation>创建会话请求</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Session/SessionRequests.cpp" line="34"/>
        <source>The container for CreateSession request parameters</source>
        <translation>创建会话请求参数的容器</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Session/SessionRequests.cpp" line="39"/>
        <source>A unique identifier for a player or entity creating the session</source>
        <translation>创建会话的玩家或实体的唯一标识符</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Session/SessionRequests.cpp" line="42"/>
        <source>A collection of custom properties for a session</source>
        <translation>会话的自定义属性集合</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Session/SessionRequests.cpp" line="45"/>
        <source>A descriptive label that is associated with a session</source>
        <translation>与会话关联的描述性标签</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Session/SessionRequests.cpp" line="48"/>
        <source>The maximum number of players that can be connected simultaneously to the session</source>
        <translation>可同时连接到会话的最大玩家数量</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Session/SessionRequests.cpp" line="69"/>
        <source>SearchSessionsRequest</source>
        <translation>搜索会话请求</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Session/SessionRequests.cpp" line="70"/>
        <source>The container for SearchSessions request parameters</source>
        <translation>搜索会话请求参数的容器</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Session/SessionRequests.cpp" line="74"/>
        <source>FilterExpression</source>
        <translation>过滤表达式</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Session/SessionRequests.cpp" line="75"/>
        <source>String containing the search criteria for the session search</source>
        <translation>包含会话搜索条件的字符串</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Session/SessionRequests.cpp" line="77"/>
        <source>SortExpression</source>
        <translation>排序表达式</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Session/SessionRequests.cpp" line="78"/>
        <source>Instructions on how to sort the search results</source>
        <translation>搜索结果排序方式的说明</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Session/SessionRequests.cpp" line="80"/>
        <source>MaxResult</source>
        <translation>最大结果数</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Session/SessionRequests.cpp" line="81"/>
        <source>The maximum number of results to return</source>
        <translation>返回的最大结果数量</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Session/SessionRequests.cpp" line="83"/>
        <location filename="../../../Code/Source/Session/SessionRequests.cpp" line="111"/>
        <source>NextToken</source>
        <translation>下一页令牌</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Session/SessionRequests.cpp" line="84"/>
        <location filename="../../../Code/Source/Session/SessionRequests.cpp" line="112"/>
        <source>A token that indicates the start of the next sequential page of results</source>
        <translation>表示下一页连续结果起始位置的令牌</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Session/SessionRequests.cpp" line="103"/>
        <source>SearchSessionsResponse</source>
        <translation>搜索会话响应</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Session/SessionRequests.cpp" line="104"/>
        <source>The container for SearchSession request results</source>
        <translation>搜索会话请求结果的容器</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Session/SessionRequests.cpp" line="108"/>
        <source>SessionConfigs</source>
        <translation>会话配置列表</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Session/SessionRequests.cpp" line="109"/>
        <source>A collection of sessions that match the search criteria and sorted in specific order</source>
        <translation>符合搜索条件并按特定顺序排序的会话集合</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Session/SessionRequests.cpp" line="132"/>
        <source>JoinSessionRequest</source>
        <translation>加入会话请求</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Session/SessionRequests.cpp" line="133"/>
        <source>The container for JoinSession request parameters</source>
        <translation>加入会话请求参数的容器</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Session/SessionRequests.cpp" line="138"/>
        <source>A unique identifier for the session</source>
        <translation>会话的唯一标识符</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Session/SessionRequests.cpp" line="140"/>
        <source>PlayerId</source>
        <translation>玩家ID</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Session/SessionRequests.cpp" line="141"/>
        <source>A unique identifier for a player. Player IDs are developer-defined</source>
        <translation>玩家的唯一标识符。玩家ID由开发者定义</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Session/SessionRequests.cpp" line="143"/>
        <source>PlayerData</source>
        <translation>玩家数据</translation>
    </message>
    <message>
        <location filename="../../../Code/Source/Session/SessionRequests.cpp" line="144"/>
        <source>Developer-defined information related to a player</source>
        <translation>与玩家相关的开发者自定义信息</translation>
    </message>
</context>
</TS>
