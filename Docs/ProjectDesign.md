# Ark-Knights 架构设计文档

## 概述

本文档详细描述了Ark-Knights项目的整体架构设计，包括Classes目录结构、技术实现细节、各模块功能以及C++特性应用。项目基于Cocos2d-x 4.0游戏引擎，采用面向对象设计理念和模块化架构。

---

## Classes 目录结构

```
Classes/
├── Core/               # ✅ 核心入口与全局定义
├── Scenes/             # ✅ 场景管理（高层逻辑）
├── Entities/           # ✅ 游戏实体（核心 OOP 继承体系）
│   ├── Base/           # ✅ 基类 (GameEntity, Character)
│   ├── Player/         # ✅ 玩家相关 (Player, Mage, Nymph, Wisdael, Mudrock)
│   ├── Enemy/          # ✅ 敌人与 AI (11种敌人 + 1个Boss)
│   └── Objects/        # ✅ 投射物、道具、武器
├── Map/                # ✅ 地图生成与管理
├── UI/                 # ✅ 用户界面与 HUD
├── Managers/           # ✅ 单例管理器（全局控制）
└── Utils/              # ✅ 工具类与辅助函数
```

**图例：** ✅ 已实现 | 🔄 部分实现 | ⚪ 待实现

---

## 核心架构模块

### 1. Core (核心层) ✅

程序入口点和全局通用定义。

- **AppDelegate.h/cpp**: Cocos2d-x程序入口，负责初始化GLContext，设置分辨率
- **Constants.h**: 游戏常量定义
  - 地图配置：`ROOM_TILES_W=26`, `ROOM_TILES_H=18`, `ROOM_CENTER_DIST=960`, `DOOR_WIDTH=4`
  - 方向定义：`DIR_UP/RIGHT/DOWN/LEFT`
  - ZOrder层级：`FLOOR`, `WALL_BELOW`, `WALL_ABOVE`, `PLAYER`, `ENEMY`

### 2. Scenes (场景层) ✅

主要游戏场景类，继承自 cocos2d::Scene。

- **MainMenuScene.h/cpp**: 开始菜单（开始游戏、设置、退出）
- **GameScene.h/cpp**: 核心战斗场景
  - 地图系统管理 (`MapGenerator`, `MiniMap`)
  - 玩家实例化与控制
  - 碰撞检测系统 (`updateMapSystem`)
  - 房间/走廊边界检测与位置限制
  - **模块化重构**: 使用 `GameHUD` 和 `GameMenus` 独立模块（1732行→1212行，减少30%）
  - **交互系统**: E键交互优先级（传送门 > 道具拾取 > 宝箱开启）
  - **中文本地化支持**: UTF-8编码处理

### 3. Entities (实体层) ✅ - *OOP 重点考察区域*

游戏对象核心，利用**继承**和**多态**。

#### Base/ ✅
- **GameEntity.h/cpp**: 所有游戏对象基类（继承 cocos2d::Sprite），包含 HP、位置、碰撞体积
- **Character.h/cpp**: 继承自 GameEntity，增加移动、状态机、攻击冷却、法力值等属性

#### Player/ ✅
- **Player.h/cpp**: 玩家控制逻辑（输入处理、移动、碰撞半径16px）
- **Mage.h/cpp**: 法师职业实现
- **Nymph.h/cpp**: 妮芙 - 远程攻击，粉色子弹，被动毒伤叠加，主动攻击+300%
- **Wisdael.h/cpp**: 维什戴尔 - 远程AOE，10%眩晕，20%闪避，范围爆炸
- **Mudrock.h/cpp**: 泥岩 - 近战范围，攻击5次获得护盾，最多9层

#### Enemy/ ✅

**敌人系统完整实现 (11种敌人 + 1个Boss):**

| 类型 | 敌人 | 特点 |
|------|------|------|
| 小怪 | Ayao(阿咬) | 近战，巡逻+追击 |
| 小怪 | KongKaZi(恐卡兹) | 红标敌人死亡生成，定时炸弹 |
| 小怪 | DeYi(得意) | 接近后自爆 |
| 精英 | Cup(魂灵圣杯) | 飞行，分摊90%伤害 |
| 精英 | Du(妒) | 远程高伤子弹 |
| 精英 | TangHuang(堂皇) | 隐身/回血光环，死后生成铁灯盘 |
| 精英 | XinXing(新硎) | 近战高伤，死后生成3个铁矛头 |
| 衍生 | IronLightCup(铁灯盘) | 35次伤害击杀，阻挡子弹 |
| 衍生 | IronLance(铁矛头) | 45次伤害击杀，阻挡子弹 |
| Boss召唤 | NiLuFire(尼卢火) | 配合Boss【清火执】技能 |
| Boss召唤 | Boat(托生莲座) | 碰撞扣生命上限，无敌2秒 |
| **Boss** | KuiLongBoss(奎隆) | 三阶段：入定→自在→无忧觉 |

**Enemy基类高级特性:**
- **毒伤系统**: `applyNymphPoison()` 支持Nymph的毒层叠加
- **隐身系统**: `Stealth` 多来源管理，支持TangHuang的隐身光环
- **红标系统**: `tryApplyRedMark()` 概率标记，死后生成KongKaZi
- **伤害分担**: Cup的 `absorbDamage()` 机制

#### Objects/ ✅
- **Item.h/cpp**: 道具定义系统，包含15种道具配置
  - 低阶道具6种（锈蚀刀片、急救药箱、坚守盾牌、投币玩具、活玫瑰、快乐水）
  - 高阶道具5种（复仇者、未知仪器、古老的铠甲、迷梦香精、金酒之杯）
  - 国王道具4种（国王的新枪、诸王的冠冕、国王的铠甲、国王的延伸）
  - 稀有度权重系统：低阶60%、高阶30%、国王10%
- **ItemDrop.h/cpp**: 地上可拾取道具系统
- **Chest.h/cpp**: 宝箱系统（已重构独立）
- **Portal.h/cpp**: 传送门实体，支持7帧动画+4帧闪电效果

### 4. Map (地图层) ✅ - *Roguelike 核心算法*

地图生成和管理逻辑。

- **MapGenerator.h/cpp**: 核心算法类
  - **BFS 地图生成算法**：从起始房开始，广度优先生成最多6个房间
  - 5×5网格矩阵管理房间位置
  - 自动生成房间间的走廊连接
  - 房间类型分配（BEGIN→NORMAL→END）

- **Room.h/cpp**: 房间管理系统
  - **地形生成**: 地板、墙壁、门口的自动生成
  - **敌人生成点管理**: 支持多种敌人类型和数量配置
  - **宝箱管理**: 房间中心生成宝箱，支持开启交互
  - **道具掉落管理**: ItemDrop对象生成、位置设置、拾取检测
  - **传送门系统**: 终点房间传送门生成和交互检测
  - **碰撞检测**: 玩家与房间边界、门口的精确检测

- **Hallway.h/cpp**: 走廊类，连接相邻房间
  - 水平走廊：4×4瓦片（128×128px）
  - 垂直走廊：4×12瓦片（128×384px）
  - 自动生成走廊墙壁（上下或左右）
  - 方向感知的边界检测

- **TerrainLayouts.h/cpp**: 地形生成系统
  - **10种地形布局**: layoutFiveBoxes、layoutNineBoxes等地形生成算法
  - **TerrainLayoutHelper静态类**: 提供复用的地形生成逻辑
  
- **BossFloor.h/cpp**: Boss房间特殊地形
  - 火焰地板生成逻辑
  - Boss战专用地形布局

- **MiniMap.h/cpp**: 小地图显示
  - 右上角显示房间布局
  - 实时追踪玩家位置（蓝色方块）
  - 显示已访问/未访问房间状态

### 5. UI (界面层) ✅

纯UI逻辑，与游戏实体逻辑解耦。

- **GameHUD.h/cpp**: 游戏HUD管理系统
  - 血条、蓝条显示与更新
  - 技能图标和冷却进度显示
  - 道具栏管理（血条蓝条下方，32px图标，每行5个）
  - 交互提示系统（动态显示"[E]获取{道具名}：{效果描述}"）
  - Debug信息显示
  - 操作说明面板

- **GameMenus.h/cpp**: 游戏菜单管理系统
  - 暂停菜单（继续游戏、设置、返回主菜单、退出）
  - 游戏结束界面（R键重新开始、Q键返回主菜单）
  - 胜利界面（通关成功提示）
  - 半透明遮罩层管理
  - 键盘监听和回调处理

- **FloatingText.h/cpp**: 飘字效果，如伤害飘字显示
- **SettingsLayer.h/cpp**: 设置界面（音量调节、画面设置等）

### 6. Managers (管理器层) ✅

**单例模式 (Singleton)** 类，用于跨场景管理数据。

- **GameManager.h/cpp**: 管理全局状态（当前分数、当前关卡层数、玩家选择的角色）
- **SoundManager.h/cpp**: 封装 CocosDenshion 或 AudioEngine，统一管理背景音乐和音效播放

### 7. Utils (工具层) ✅

通用的静态函数或工具类。

- **MathUtils.h/cpp**: 额外的数学计算（如计算两个节点间的角度、向量运算）
- **AnimUtils.h/cpp**: 快捷创建序列帧动画（Animation）的辅助函数

---

## 技术实现细节

### 地图系统算法

#### BFS 房间生成算法
```cpp
1. 从中心点(2,2)开始作为起始房
2. 使用队列进行广度优先搜索
3. 每次随机选择1-2个方向扩展
4. 最多生成6个房间
5. 避免房间重叠和超出5×5边界
```

#### 碰撞检测方案
- 边界框 (AABB) 检测，无物理引擎
- 考虑玩家碰撞半径 (16px = 0.5 tile)
- 边界向内缩进 48px (1.5 tiles) = 墙壁32px + 玩家半径16px
- 门口区域特殊处理：判断玩家Y/X坐标是否在门宽度范围内

#### 偶数瓦片对齐
```cpp
// 对于26个瓦片（偶数），中心在两个瓦片之间
startX = centerX - tileSize * (width/2 - 0.5)  // 第0列中心
      = centerX - 32 * 12.5 = centerX - 400
```

### 道具系统设计

#### 三层稀有度系统
- **低阶道具**: 60%概率，基础属性提升
- **高阶道具**: 30%概率，特殊效果组合
- **国王道具**: 10%概率，强大的套装效果

#### 道具效果系统
```cpp
// 道具效果应用示例
switch(itemType) {
    case RUSTY_BLADE: player->addAttack(15); break;
    case FIRST_AID_KIT: player->heal(100); break;
    case GUARD_SHIELD: player->addDamageReduction(10); break;
    // ... 更多道具效果
}
```

### 敌人AI系统

#### AI状态机
```cpp
enum class AIState {
    PATROL,     // 巡逻状态
    CHASE,      // 追击状态
    ATTACK,     // 攻击状态
    DEAD        // 死亡状态
};
```

#### 视线检测算法
```cpp
bool Enemy::isPlayerInSight(Player* player) const {
    float distance = player->getPosition().distance(this->getPosition());
    return distance <= _sightRange && !player->isStealthed();
}
```

---

## 架构特点

### 设计模式应用

1. **单例模式**: GameManager、SoundManager等全局管理器
2. **工厂模式**: 敌人和道具的CREATE_FUNC宏
3. **观察者模式**: UI更新响应游戏状态变化
4. **策略模式**: 不同职业的技能实现
5. **组合模式**: Node树结构的场景管理

### 模块化设计

- **GameScene重构**: 1732行→1212行（减少30%），HUD和菜单系统独立
- **Room系统重构**: 从1039行减至~740行，地形生成系统独立
- **松耦合架构**: 使用回调机制实现菜单与场景解耦
- **单一职责原则**: GameHUD专注显示，GameMenus专注交互，GameScene专注游戏逻辑

### OOP特性应用

- **完整继承体系**: `GameEntity → Character → Enemy/Player → 具体实现`
- **多态应用**: 虚函数 `attack()`, `die()`, `executeAI()`, `isPoisonable()` 等
- **封装**: 私有成员变量，公有接口函数
- **抽象**: Character基类定义通用行为接口

---

## 重构成果总览

| 文件 | 重构前 | 重构后 | 行数变化 | 说明 |
|------|--------|--------|----------|------|
| **GameScene.cpp** | ~2000行 | 1420行 | **- ~580行 (29%)** | 拆分HUD和菜单系统 |
| **Room.cpp** | ~1100行 | 684行 | **- ~416行 (39%)** | 拆分地形生成系统、可交互物件及Boss层特殊系统 |
| **新增模块** | - | 7个文件 | **+ ~1200行** | GameHUD、GameMenus、TerrainLayouts、Portal、Chest、Item、BossFloor |

**重构效果:**
- **单文件复杂度降低**: 最大文件从1732行降到1212行
- **职责分离清晰**: HUD显示、菜单交互、地形生成、传送门管理等各司其职
- **可维护性提升**: 模块独立，便于调试和扩展
- **代码复用增强**: 地形布局算法可被其他系统调用

---

## C++ 特性使用统计

| 特性 | 使用位置 | 说明 |
|------|----------|------|
| **STL容器** | `std::vector<Enemy*>`, `std::map<ItemType, ItemConfig>`, `std::set<void*>` | 敌人列表、道具配置、隐身来源管理 |
| **迭代器** | Enemy遍历、道具列表 | 范围for循环、标准迭代器 |
| **类与多态** | Entity继承体系 | `GameEntity→Character→Enemy/Player` |
| **虚函数** | `virtual attack()`, `virtual die()` | 多态行为实现 |
| **模板** | Cocos2d-x CREATE_FUNC | 工厂模式宏 |
| **异常处理** | 资源加载 | try-catch保护 |
| **函数重载** | `takeDamage(int)`, `takeDamage(int, DamageType)` | 多版本伤害处理 |
| **C++11特性** | `override`, `nullptr`, `auto`, lambda表达式 | 现代C++特性 |
| **智能指针** | `std::function<void()>` | 回调函数管理 |
| **常量表达式** | `constexpr` | 编译期常量优化 |

---

## 资源文件结构

```
Resources/
├── Enemy/                    # 敌人动画资源
│   ├── AYao/                 # 阿咬 (Attack/Die/Move)
│   ├── Cup/                  # 魂灵圣杯 (Die/Idle)
│   ├── KongKaZi/             # 恐卡兹 (Attack/Die/Move)
│   ├── DeYi/                 # 得意 (Die/Move)
│   ├── Du/                   # 妒 (Attack/Bullet/Die/Move)
│   ├── TangHuang&&Iron LightCup/  # 堂皇+铁灯盘
│   ├── XinXing&&Iron Lance/  # 新硎+铁矛头
│   ├── NiLu Fire/            # 尼卢火 (Attack/Burning)
│   ├── Boat/                 # 托生莲座 (Die/Idle/Move)
│   └── _BOSS_kuiLong/        # 奎隆Boss (多阶段动画)
├── Player/                   # 玩家角色动画
│   ├── Nymph/                # 妮芙 (Attack/Bullet/Die/Idle/Move/Skill_*)
│   ├── Wisdael/              # 维什戴尔 (Attack/Bullet/Die/Idle/Move/Skill_*)
│   └── Mudrock/              # 泥岩 (Attack/Die/Idle/Move/Skill_*)
├── Map/                      # 地图素材
│   ├── Floor/Wall/Door/      # 基础地形
│   ├── Barrier/              # 障碍物（木箱等）
│   ├── Chest/                # 宝箱
│   ├── Portal/               # 传送门
│   └── Fire Generator/       # 火焰地块
├── Property/                 # 道具图标
│   ├── LowLevel/             # 低阶藏品 (6种)
│   └── HighLevel/            # 高阶藏品 (5种) + 国王藏品 (4种)
└── UIs/                      # UI素材
    ├── Skills/               # 技能图标
    └── StatusBars/           # 血条蓝条素材
```

---

## 总结

Ark-Knights项目采用了模块化、面向对象的架构设计，充分利用了C++的语言特性和设计模式。通过持续的重构优化，代码质量和可维护性得到了显著提升。项目展现了完整的游戏开发流程，从底层的实体系统到上层的UI交互，形成了一个结构清晰、功能完整的Roguelike游戏架构。

**核心技术亮点:**
- BFS地图生成算法确保房间连通性
- 完整的OOP继承体系和多态应用
- 模块化设计降低代码耦合度
- 丰富的道具和敌人系统设计
- 现代C++特性的合理运用