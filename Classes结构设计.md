### Classes 目录结构设计

```
Classes/
├── Core/               # ✅ 核心入口与全局定义
├── Scenes/             # ✅ 场景管理（高层逻辑）
├── Entities/           # 🔄 游戏实体（核心 OOP 继承体系）
│   ├── Base/           # 基类
│   ├── Player/         # ✅ 玩家相关
│   ├── Enemy/          # 🔄 敌人与 AI
│   └── Objects/        # 投射物、道具、武器
├── Map/                # ✅ 地图生成与管理
├── UI/                 # 用户界面与 HUD
├── Managers/           # 单例管理器（全局控制）
└── Utils/              # 工具类与辅助函数
```

**图例：** ✅ 已实现 | 🔄 部分实现 | ⚪ 待实现

------



### 详细分类说明

#### 1. Core (核心层) ✅

存放程序的入口点和全局通用的定义。

- **✅ AppDelegate.h/cpp**: Cocos2d-x 程序的入口，负责初始化 GLContext，设置分辨率等。
- **✅ Constants.h**: 存放游戏常量（地图配置、房间尺寸、瓦片大小、方向定义、ZOrder等）。
  - 地图配置：`ROOM_TILES_W=26`, `ROOM_TILES_H=18`, `ROOM_CENTER_DIST=960`, `DOOR_WIDTH=4`
  - 方向定义：`DIR_UP/RIGHT/DOWN/LEFT`
  - ZOrder层级：`FLOOR`, `WALL_BELOW`, `WALL_ABOVE`, `PLAYER`, `ENEMY`
- **⚪ GameMacros.h**: (可选) 全局宏定义、Tag 枚举。

#### 2. Scenes (场景层) ✅

存放主要的游戏场景类，通常继承自 cocos2d::Scene。

- **⚪ MainMenuScene.h/cpp**: 开始菜单（开始游戏、设置、退出）。
- **✅ GameScene.h/cpp**: 核心战斗场景，负责承载地图层、游戏逻辑层和 UI 层。
  - 地图系统管理 (`MapGenerator`, `MiniMap`)
  - 玩家实例化与控制
  - 碰撞检测系统 (`updateMapSystem`)
  - 房间/走廊边界检测与位置限制🔄 - *OOP 重点考察区域*

这里是游戏对象的核心，利用**继承**和**多态**。

- **Base/**
  - **⚪ GameEntity.h/cpp**: 所有游戏对象的基类（继承 cocos2d::Sprite 或 Node），包含 HP、位置、碰撞体积。
  - **⚪ Character.h/cpp**: 继承自 GameEntity，增加移动、状态机、武器持有等属性。
  
- **Player/** ✅
  - **✅ Player.h/cpp**: 玩家控制逻辑（输入处理、移动、碰撞半径16px）。
  - **⚪ Mage.h/cpp, Warrior.h/cpp**: 具体职业，实现 useSkill() 虚函数。
  
- **Enemy/** 🔄
  - **✅ Enemy.h/cpp**: 敌人基类（已有基础实现）。
  - **⚪ MeleeEnemy✅ - *Roguelike 核心算法*

存放地图生成和管理的逻辑。

- **✅ MapGenerator.h/cpp**: 核心算法类，负责生成房间坐标、连接路径。
  - **BFS 地图生成算法**：从起始房开始，广度优先生成最多6个房间
  - 5×5网格矩阵管理房间位置
  - 自动生成房间间的走廊连接
  - 房间类型分配（BEGIN ⚪

存放纯 UI 逻辑，与游戏实体逻辑解耦。

- **⚪ HUDLayer.h/cpp**: 战斗界面（左上角血条、蓝条、技能冷却图标）。
- **⚪ PauseMenu.h/cpp**: 暂停弹窗。
- **⚪ DamageNumber.h/cpp**: 飘血效果（受到伤害时冒出的数字）。
- **⚪ VirtualJoystick.h/cpp**: (如果做手游操作) 虚拟摇杆。

**注**: 目前小地图 (MiniMap) 已在 Map/ 目录实现
  - 敌人生成点管理
  - 房间类型与尺寸调整（Boss房更大、道具房更小）
  
- **✅ Hallway.h/cpp**: 走廊类，连接相邻房间。
  - 水平走廊：4×4瓦片（128×128px）
  - 垂直走廊：4×12瓦片（128×384px）
  - 自动生成走廊墙壁（上下或左右）
  - 方向感知的边界检测（水平限制Y轴，垂直限制X轴）
  
- **✅ MiniMap.h/cpp**: 小地图显示。
  - 右上角显示房间布局
  - 实时追踪玩家位置（蓝色方块）
  - 显示已访问/未访问房间状态
   ⚪

存放**单例模式 (Singleton)** 类，用于跨场景管理数据。

- **⚪ GameManager.h/cpp**: 管理全局状态（当前分数、当前关卡层数、玩家选择的角色）。
- **⚪ SoundManager.h/cpp**: 封装 CocosDenshion 或 AudioEngine，统一管理背景音乐和音效播放。
- **⚪ CollisionManager.h/cpp**: (可选) 如果不用物理引擎，可在此处统一处理碰撞检测。

**注**: 目前碰撞检测在 Game ⚪

存放通用的静态函数或工具类。

- **⚪ MathUtils.h/cpp**: 额外的数学计算（如计算两个节点间的角度、向量运算）。
- **⚪ AnimUtils.h/cpp**: 快捷创建序列帧动画（Animation）的辅助函数。
- **⚪ CSVReader.h/cpp**: (可选) 如果怪物数值存在 CSV 表里，用这个读取。

---

## 技术实现细节

### 地图系统算法

**BFS 房间生成：**
```cpp
1. 从中心点(2,2)开始作为起始房
2. 使用队列进行广度优先搜索
3. 每次随机选择1-2个方向扩展
4. 最多生成6个房间
5. 避免房间重叠和超出5×5边界
```

**碰撞检测方案：**
- 边界框 (AABB) 检测，无物理引擎
- 考虑玩家碰撞半径 (16px = 0.5 tile)
- 边界向内缩进 48px (1.5 tiles) = 墙壁32px + 玩家半径16px
- 门口区域特殊处理：判断玩家Y/X坐标是否在门宽度范围内

**偶数瓦片对齐：**
```cpp
// 对于26个瓦片（偶数），中心在两个瓦片之间
startX = centerX - tileSize * (width/2 - 0.5)  // 第0列中心
      = centerX - 32 * 12.5 = centerX - 400
```

### 下一步开发重点

1. **摄像机系统** (camera 分支)
   - 相机跟随玩家移动
   - 房间切换时平滑过渡
   - 边界限制

2. **敌人系统** (enemy 分支)
   - AI 寻路（A*或简单追踪）
   - 8种敌人的具体实现
   - 动画播放系统

3. **战斗系统** (combat 分支)
   - 子弹/投射物
   - 伤害计算
   - 技能系统
- **MapGenerator.h/cpp**: 核心算法类，负责生成房间坐标、连接路径（DFS/随机游走算法）。
- **Room.h/cpp**: 单个房间的数据结构（门的位置、刷怪点、是否已通关）。
- **TileHelper.h/cpp**: 辅助类，用于处理瓦片地图（TMXTiledMap）的坐标转换。

#### 5. UI (界面层)

存放纯 UI 逻辑，与游戏实体逻辑解耦。

- **HUDLayer.h/cpp**: 战斗界面（左上角血条、蓝条、技能冷却图标）。
- **PauseMenu.h/cpp**: 暂停弹窗。
- **DamageNumber.h/cpp**: 飘血效果（受到伤害时冒出的数字）。
- **VirtualJoystick.h/cpp**: (如果做手游操作) 虚拟摇杆。

#### 6. Managers (管理器层)

存放**单例模式 (Singleton)** 类，用于跨场景管理数据。

- **GameManager.h/cpp**: 管理全局状态（当前分数、当前关卡层数、玩家选择的角色）。
- **SoundManager.h/cpp**: 封装 CocosDenshion 或 AudioEngine，统一管理背景音乐和音效播放。
- **CollisionManager.h/cpp**: (可选) 如果不用物理引擎，可在此处统一处理碰撞检测。

#### 7. Utils (工具层)

存放通用的静态函数或工具类。

- **MathUtils.h/cpp**: 额外的数学计算（如计算两个节点间的角度、向量运算）。
- **AnimUtils.h/cpp**: 快捷创建序列帧动画（Animation）的辅助函数。
- **CSVReader.h/cpp**: (可选) 如果怪物数值存在 CSV 表里，用这个读取。