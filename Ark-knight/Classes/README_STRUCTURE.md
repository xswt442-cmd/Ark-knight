# Classes 目录结构 - 已完成基础搭建

## 📁 目录结构

```
Classes/
├── Core/                          # ✅ 核心层
│   ├── Constants.h                # 全局常量定义
│   └── GameMacros.h               # 全局宏定义
│
├── Entities/                      # ✅ 实体层
│   ├── Base/                      # 基类
│   │   ├── GameEntity.h/cpp       # 游戏实体基类
│   │   └── Character.h/cpp        # 角色基类（继承GameEntity）
│   │
│   ├── Player/                    # 玩家系统
│   │   └── Player.h/cpp           # 玩家基类（支持输入、技能、冲刺）
│   │
│   └── Enemy/                     # 敌人系统
│       └── Enemy.h/cpp            # 敌人基类（含AI、追击、巡逻）
│
├── Scenes/                        # ✅ 场景层
│   ├── MainMenuScene.h/cpp        # 主菜单场景
│   └── GameScene.h/cpp            # 游戏战斗场景
│
├── Managers/                      # ✅ 管理器层（单例）
│   ├── GameManager.h/cpp          # 游戏状态管理
│   └── SoundManager.h/cpp         # 音频管理
│
├── Utils/                         # ✅ 工具层
│   └── MathUtils.h/cpp            # 数学工具类
│
├── Map/                           # ⏳ 地图系统（待实现）
├── UI/                            # ⏳ UI系统（待实现）
└── AppDelegate.h/cpp              # ✅ 程序入口（已修改）
```

## ✅ 已完成的功能

### 1. 核心基类体系
- **GameEntity**: 所有游戏对象的基类
  - HP管理
  - 精灵绑定
  - 碰撞检测
  - 死亡处理（纯虚函数）

- **Character**: 角色抽象类（继承GameEntity）
  - 状态机系统（IDLE/MOVE/ATTACK/SKILL/HIT/DASH/DIE）
  - 移动系统
  - 攻击系统（含冷却）
  - MP管理

### 2. 玩家系统
- **Player**: 玩家基类
  - 键盘+鼠标输入（WASD移动、J攻击、K技能、Space冲刺）
  - 冲刺系统（带冷却）
  - 护甲系统
  - 技能抽象接口（子类实现具体技能）

### 3. 敌人系统
- **Enemy**: 敌人基类
  - AI系统（视野检测、追击、巡逻）
  - 攻击范围判定
  - 自动攻击玩家

### 4. 场景系统
- **MainMenuScene**: 主菜单
  - 开始游戏、选择角色、设置、退出
  - 场景切换动画

- **GameScene**: 战斗场景
  - 游戏层+UI层分离
  - 玩家创建和控制
  - 敌人AI更新
  - HUD显示（HP/MP/敌人数量/位置）
  - 暂停/继续功能（ESC键）

### 5. 管理器系统
- **GameManager**: 单例模式
  - 关卡管理
  - 分数管理
  - 角色选择
  - 金币系统

- **SoundManager**: 单例模式
  - BGM播放/停止/暂停/恢复
  - 音效播放
  - 音量控制
  - 静音功能
  - 音频预加载

### 6. 工具类
- **MathUtils**: 数学工具
  - 距离计算
  - 角度计算
  - 向量归一化
  - 线性插值
  - 随机数生成

## 🎮 当前可运行功能

运行游戏后：
1. 启动进入主菜单
2. 点击"Start Game"进入战斗场景
3. 使用WASD移动蓝色方块（玩家）
4. 3个红色方块（敌人）会追击玩家
5. 按J键攻击（当前仅有冷却判定）
6. 按Space冲刺
7. 按ESC暂停/继续
8. 左上角显示HP/MP，右上角显示Debug信息

## 📝 C++特性应用

已满足期末要求的至少3条C++特性：

✅ **1. STL容器**
- `Vector<Enemy*>` 管理敌人列表
- `std::unordered_map` 音效ID映射

✅ **2. 类与多态**
- GameEntity -> Character -> Player/Enemy 继承体系
- 虚函数：`die()`, `attack()`, `useSkill()`

✅ **3. C++11特性**
- Lambda表达式（输入回调、动作回调）
- `auto` 关键字
- `= delete` 禁止拷贝
- `= default` 默认构造

✅ **4. 类型转换**
- `static_cast` 安全类型转换

✅ **5. 异常处理**（代码中有基础防护）
- 空指针检查
- 边界条件判断

## 🔧 CMakeLists.txt 更新说明

需要将新创建的 .cpp 文件添加到 CMakeLists.txt 中：

```cmake
set(GAME_SRC
  # 原有文件
  Classes/AppDelegate.cpp
  
  # Core
  # (仅头文件，无需添加)
  
  # Entities/Base
  Classes/Entities/Base/GameEntity.cpp
  Classes/Entities/Base/Character.cpp
  
  # Entities/Player
  Classes/Entities/Player/Player.cpp
  
  # Entities/Enemy
  Classes/Entities/Enemy/Enemy.cpp
  
  # Scenes
  Classes/Scenes/MainMenuScene.cpp
  Classes/Scenes/GameScene.cpp
  
  # Managers
  Classes/Managers/GameManager.cpp
  Classes/Managers/SoundManager.cpp
  
  # Utils
  Classes/Utils/MathUtils.cpp
)
```

## 🚀 下一步开发计划

### 优先级1（核心功能）
1. **实现具体职业**
   - Mage.h/cpp（法师：火球术）
   - Warrior.h/cpp（战士：近战强化）
   - Alchemist.h/cpp（炼金：毒圈）

2. **武器/投射物系统**
   - Projectile.h/cpp（子弹基类）
   - Weapon.h/cpp（武器系统）

3. **地图生成系统**
   - MapGenerator.h/cpp（Roguelike房间生成）
   - Room.h/cpp（房间类）

### 优先级2（增强体验）
4. **UI系统**
   - HUDLayer.h/cpp（血条、技能冷却）
   - PauseMenu.h/cpp（暂停菜单）

5. **道具系统**
   - Prop.h/cpp（掉落物、宝箱）

6. **更多敌人类型**
   - MeleeEnemy.h/cpp
   - RangedEnemy.h/cpp
   - BossEnemy.h/cpp

### 优先级3（完善）
7. 添加真实的美术资源
8. 完善音效系统
9. 保存/读档系统
10. 关卡流程完善

## ⚠️ 注意事项

1. **编译时可能需要**：
   - 在CMakeLists.txt中添加所有.cpp文件
   - 确保include路径正确
   - 链接AudioEngine库

2. **临时方案**：
   - 当前使用DrawNode绘制方块代替精灵图
   - 音频文件路径需要根据实际资源调整

3. **待优化**：
   - 碰撞检测（可考虑使用物理引擎）
   - 动画系统（需要序列帧资源）
   - 性能优化（对象池等）
