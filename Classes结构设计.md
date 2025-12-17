### Classes 目录结构设计

```
Classes/
├── Core/               # 核心入口与全局定义
├── Scenes/             # 场景管理（高层逻辑）
├── Entities/           # 游戏实体（核心 OOP 继承体系）
│   ├── Base/           # 基类
│   ├── Player/         # 玩家相关
│   ├── Enemy/          # 敌人与 AI
│   └── Objects/        # 投射物、道具、武器
├── Map/                # 地图生成与管理
├── UI/                 # 用户界面与 HUD
├── Managers/           # 单例管理器（全局控制）
└── Utils/              # 工具类与辅助函数
```

------



### 详细分类说明

#### 1. Core (核心层)

存放程序的入口点和全局通用的定义。

- **AppDelegate.h/cpp**: Cocos2d-x 程序的入口，负责初始化 GLContext，设置分辨率等。
- **GameMacros.h**: 存放全局宏定义（如屏幕宽高、物理掩码 BitMask、Tag 枚举）。
- **Constants.h**: 存放游戏常量（如移动速度基准值、文件路径字符串）。

#### 2. Scenes (场景层)

存放主要的游戏场景类，通常继承自 cocos2d::Scene。

- **MainMenuScene.h/cpp**: 开始菜单（开始游戏、设置、退出）。
- **GameScene.h/cpp**: 核心战斗场景，负责承载地图层、游戏逻辑层和 UI 层。
- **LoadingScene.h/cpp**: (可选) 场景切换时的加载过渡界面。
- **GameOverScene.h/cpp**: 死亡结算场景。

#### 3. Entities (实体层) - *OOP 重点考察区域*

这里是游戏对象的核心，利用**继承**和**多态**。

- **Base/**GameEntity.h/cpp: 所有游戏对象的基类（继承 cocos2d::Sprite 或 Node），包含 HP、位置、碰撞体积。Character.h/cpp: 继承自 GameEntity，增加移动、状态机、武器持有等属性。
- **Player/**Player.h/cpp: 玩家控制逻辑（输入处理）。Mage.h/cpp, Warrior.h/cpp: 具体职业，实现 useSkill() 虚函数。
- **Enemy/**Enemy.h/cpp: 敌人基类，包含寻路逻辑。MeleeEnemy.h/cpp, RangedEnemy.h/cpp: 近战/远程怪物的具体实现。BossEnemy.h/cpp: Boss 的特殊逻辑（多阶段、大血条）。
- **Objects/**Projectile.h/cpp: 子弹/火球基类。Weapon.h/cpp: 武器逻辑。Chest.h/cpp: 宝箱、掉落物。

#### 4. Map (地图层) - *Roguelike 核心算法*

存放地图生成和管理的逻辑。

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