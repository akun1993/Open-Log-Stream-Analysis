基于 Qt 实现的动态流程图画板框架设计与实现
@[toc]

## 效果图

## 概述

- 本项目的主要功能包括：
  - 对通信数据的解析，提取节点信息和连接信息。
  - 动态生成流程图，根据节点信息和连接信息，自动布局节点和绘制连接线。
  - 支持节点的状态变化，根据通信数据，动态更新节点的状态。
  - 支持连接线的避让，避免连接线与节点重叠或交叉。
  - 支持流程图的导出，用户可以将生成的流程图导出为图片或PDF格式。
- 注：本项目只是对我数据进行可视化流程展示，实际应用中可能需要根据具体需求进行修改和扩展。

## 框架整体架构

该系统基于Qt的`Graphics View`框架设计，采用"场景-视图-元素"三层架构，实现了数据与展示的分离：

- **场景层（FlowChartScene）**：继承自`QGraphicsScene`，负责流程图数据的管理、布局计算和逻辑处理，是整个框架的核心
- **视图层（FlowChartView）**：继承自`QGraphicsView`，负责场景的显示、缩放和平移等交互操作
- **元素层**：包括节点（FlowNode）和边（FlowEdge），继承自`QGraphicsItem`，负责自身的绘制和状态管理
- **容器层**：MainWindow和SystemImageWidget提供界面容器，处理数据接收与转发

## 核心组件实现细节

### 1. 场景管理（FlowChartScene）

场景是流程图的"数据中枢"，主要负责：

#### 数据加载与解析

通过`loadFlowChart`方法接收JSON格式的流程图数据，完成三步核心操作：

- 解析JSON数据，提取节点状态（运行中、等待中、已完成等）和连接关系
- 清理现有场景元素（节点、边、图例）
- 重建节点和边，并恢复或重置激活状态

```cpp
void FlowChartScene::loadFlowChart(const QJsonObject &json) {
    // 解析节点状态变化
    parseJson(json);
    // 清空场景
    clearScene();
    // 创建节点和边
    createNodes();
    createEdges();
    // 布局与渲染
    arrangeLayout();
}
```

#### 自动布局算法

流程图的美观性很大程度依赖布局算法，这里实现了基于层级的布局策略：

1. **层级计算**：通过BFS（广度优先搜索）从"开始"节点出发，计算每个节点的层级，确保流程流向符合逻辑

   ```cpp
   void FlowChartScene::calculateNodeLevels() {
       // 从"开始"节点初始化层级
       if (m_nodes.contains("开始")) {
           m_nodeLevels["开始"] = 0;
           m_levelNodes[0].append("开始");
       }
       // BFS遍历计算层级
       std::queue<QString> nodeQueue;
       nodeQueue.push("开始");
       // ... 层级传播逻辑
   }
   ```

2. **节点排列**：同层级节点横向均匀分布，层级间保持固定间距（`m_layerSpacing = 250`），避免节点重叠

3. **冲突修正**：对反向连接（如循环流程）进行层级调整，确保`目标节点层级 = 源节点层级 + 1`

#### 图例与辅助元素

自动创建状态图例，直观展示不同颜色对应的节点状态（运行中-绿色、等待中-橙色、已运行-蓝色等），增强流程图的可读性。

### 2. 视图控制（FlowChartView）

视图层负责用户交互与显示控制，核心功能包括：

- **缩放控制**：通过鼠标滚轮实现缩放，限制最小（10%）和最大（500%）缩放比例

  ```cpp
  void FlowChartView::wheelEvent(QWheelEvent *event) {
      double newScale = event->angleDelta().y() > 0 ? current * 1.15 : current / 1.15;
      if (newScale >= m_minScale && newScale <= m_maxScale) {
          scale(scaleFactor, scaleFactor);
      }
  }
  ```

- **自适应布局**：窗口大小变化时，在未进行手动缩放的情况下自动调整视图，保持场景完整显示

- **拖拽平移**：启用`ScrollHandDrag`模式，支持鼠标拖拽平移场景

### 3. 节点元素（FlowNode）

节点是流程图的核心信息载体，实现了丰富的可视化特性：

- **状态可视化**：通过不同颜色和边框样式区分节点状态（正常、激活、等待、错误等），并支持"跳过"状态的特殊样式（紫色）

  ```cpp
  void FlowNode::paint(...) {
      // 根据状态选择画笔和画刷
      switch (m_state) {
          case ActiveState:      // 运行中-绿色
          case WaitingState:     // 等待中-橙色
          case ErrorState:       // 错误-红色
          // ... 状态样式处理
      }
  }
  ```

- **交互反馈**：支持鼠标悬停显示完整标签（解决标签过长问题），选中状态通过黄色虚线边框高亮

- **样式优化**：使用渐变色填充增强视觉层次感，圆角矩形设计提升美观度

### 4. 边元素（FlowEdge）

边负责连接节点并展示流程关系，关键特性包括：

- **智能路径规划**：根据节点距离自动选择路径类型：
  - 短距离（<500）：采用正交路径，通过中间点微调避免重叠
  - 中长距离（500-1200）：使用贝塞尔曲线，动态计算偏移量
  - 长距离（>1200）：优化绕行路径，避免与其他节点冲突

- **连接优化**：通过通道索引（`sourceChannelIndex`/`targetChannelIndex`）实现多入边/出边的分散布局，解决密集连接的重叠问题

- **状态同步**：边的状态与源节点保持一致，通过线条颜色和粗细变化反映流程激活状态

## 数据流转与扩展

系统通过GRPC客户端接收流程数据（`MainWindow::initGrpcClient`），解析为JSON格式后传递给`SystemImageWidget`，最终由`FlowChartView`转发给场景进行渲染，形成完整的数据链路。

该框架具备良好的扩展性：

- 可通过新增`NodeState`枚举值扩展状态类型
- 可通过修改`applySmartLayout`实现自定义布局算法
- 可通过重写`FlowNode::paint`和`FlowEdge::drawOrthogonalPath`定制外观风格

## 总结

该流程图框架基于Qt的Graphics View架构，通过清晰的职责划分（场景管数据、视图管显示、元素管自身）实现了高效的流程图可视化。其核心优势在于：

1. 自动化的布局算法，减少手动调整成本
2. 丰富的状态可视化，直观反映流程进展
3. 友好的交互体验，支持缩放、平移、悬停等操作

当然，本项目只是一个简单的实现，提供一些思路和代码，实际应用中可能需要根据具体需求进行修改和扩展。
[项目地址](https://gitee.com/shan-jie6/sharecode/tree/master/flowchart_dy)
