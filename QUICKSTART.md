# Ark-Knights 快速开始指南

## 🎯 基础架构已搭建完成

### ✅ 已完成的工作
1. **完整的类继承体系**（满足OOP要求）
2. **场景系统**（主菜单 + 游戏场景）
3. **玩家系统**（输入处理、移动、攻击、冲刺）
4. **敌人AI系统**（追击、巡逻、攻击）
5. **管理器系统**（游戏状态、音频）
6. **工具类库**（数学计算）

### 📦 包含的C++特性
- ✅ STL容器（Vector, unordered_map）
- ✅ 类与多态（继承体系，虚函数）
- ✅ C++11特性（Lambda, auto, =delete, =default）
- ✅ 类型转换（static_cast）
- ✅ 异常处理（空指针检查）

---

## 🔧 编译步骤

### 方法1：使用CMake（推荐）

```bash
# 1. 进入项目目录
cd Ark-knight

# 2. 创建构建目录
mkdir build
cd build

# 3. 生成项目
cmake ..

# 4. 编译（Windows）
cmake --build . --config Debug
# 或者用Release模式
cmake --build . --config Release
```

### 方法2：使用Visual Studio（Windows）

1. 用VS打开 `cbuild/Ark-knight.sln`
2. 右键项目 -> 属性 -> C/C++ -> 常规 -> 附加包含目录
3. 确保包含 Classes 目录
4. 按F5运行

---

## 🎮 当前游戏功能

### 控制方式
- **WASD**: 移动角色
- **J键**: 普通攻击
- **K键**: 技能（当前未实现具体效果）
- **Space**: 冲刺
- **ESC**: 暂停/继续游戏

### 游戏玩法
1. 启动游戏进入主菜单
2. 点击"Start Game"进入战斗
3. 控制蓝色方块（玩家）
4. 躲避红色方块（敌人）的追击
5. 左上角显示HP和MP
6. 右上角显示敌人数量和玩家位置

---

## 📝 下一步开发计划

### 🔥 优先级1（必须完成）

#### 1. 实现3个角色职业
创建以下文件：

**Mage.h/cpp（法师）**
```cpp
// 位置: Classes/Entities/Player/Mage.h
class Mage : public Player {
    void useSkill() override; // 火球术
    int getSkillMPCost() const override { return 30; }
};
```

**Warrior.h/cpp（战士）**
```cpp
// 位置: Classes/Entities/Player/Warrior.h
class Warrior : public Player {
    void useSkill() override; // 旋风斩
    int getSkillMPCost() const override { return 20; }
};
```

**Alchemist.h/cpp（炼金术士）**
```cpp
// 位置: Classes/Entities/Player/Alchemist.h
class Alchemist : public Player {
    void useSkill() override; // 毒圈
    int getSkillMPCost() const override { return 40; }
};
```

#### 2. 投射物/武器系统

**Projectile.h/cpp（子弹基类）**
```cpp
// 位置: Classes/Entities/Objects/Projectile.h
class Projectile : public GameEntity {
    Vec2 _velocity;
    int _damage;
    void update(float dt) override;
};
```

**Weapon.h/cpp（武器系统）**
```cpp
// 位置: Classes/Entities/Objects/Weapon.h
class Weapon : public Node {
    void fire(Vec2 direction);
};
```

#### 3. 地图生成系统（核心Roguelike机制）

**Room.h/cpp**
```cpp
// 位置: Classes/Map/Room.h
class Room : public Node {
    RoomType _type;
    bool _isCleared;
    Vector<Enemy*> _enemies;
};
```

**MapGenerator.h/cpp**
```cpp
// 位置: Classes/Map/MapGenerator.h
class MapGenerator {
    static void generateMap(int width, int height);
    // DFS或随机游走算法
};
```

### ⚡ 优先级2（增强游戏性）

4. **更多敌人类型**
   - MeleeEnemy（近战怪）
   - RangedEnemy（远程怪）
   - BossEnemy（Boss怪）

5. **UI系统完善**
   - 血条显示
   - 技能冷却图标
   - 伤害数字飘字

6. **道具系统**
   - 血包
   - 蓝瓶
   - 武器道具

### 🎨 优先级3（视觉体验）

7. **美术资源替换**
   - 角色精灵图
   - 敌人精灵图
   - 地图贴图
   - 技能特效

8. **音效完善**
   - 战斗音效
   - UI音效
   - 背景音乐切换

---

## 📚 代码示例：如何扩展

### 示例1：创建法师职业

```cpp
// Mage.h
#ifndef __MAGE_H__
#define __MAGE_H__

#include "Player.h"

class Mage : public Player {
public:
    CREATE_FUNC(Mage);
    
    virtual bool init() override;
    void useSkill() override;
    int getSkillMPCost() const override { return 30; }
    
private:
    void castFireball();
};

#endif
```

```cpp
// Mage.cpp
#include "Mage.h"
#include "Entities/Objects/Projectile.h"

bool Mage::init() {
    if (!Player::init()) {
        return false;
    }
    
    // 法师特有属性
    setMaxHP(80);
    setHP(80);
    setMaxMP(150);
    setMP(150);
    setAttack(15);
    
    return true;
}

void Mage::useSkill() {
    if (!consumeMP(getSkillMPCost())) {
        return;
    }
    
    castFireball();
    resetSkillCooldown();
}

void Mage::castFireball() {
    // TODO: 创建火球投射物
    GAME_LOG("Mage casts Fireball!");
}
```

### 示例2：在GameScene中使用法师

```cpp
// GameScene.cpp
#include "Entities/Player/Mage.h"

void GameScene::createPlayer() {
    _player = Mage::create();  // 使用法师
    _player->setPosition(SCREEN_CENTER);
    
    // 创建精灵
    auto sprite = Sprite::create();
    // ... 绑定精灵
    
    _gameLayer->addChild(_player);
}
```

---

## 🐛 常见问题

### Q: 编译错误找不到头文件？
**A**: 确保CMakeLists.txt包含了所有.h和.cpp文件，重新运行cmake

### Q: 运行时闪退？
**A**: 检查：
1. Resources目录是否正确复制到输出目录
2. 音频文件路径是否正确
3. Debug模式下查看日志

### Q: 角色不响应输入？
**A**: 检查：
1. 是否调用了`registerInputEvents()`
2. Scene是否正确添加了Player节点
3. EventDispatcher是否正常工作

### Q: 敌人不移动？
**A**: 确保在GameScene的update中调用了`enemy->executeAI(_player, dt)`

---

## 📊 项目进度

- [x] 基础架构搭建
- [x] 核心类继承体系
- [x] 场景系统
- [x] 输入系统
- [x] 管理器系统
- [ ] 3个角色职业实现
- [ ] 投射物/武器系统
- [ ] 地图生成（Roguelike）
- [ ] 更多敌人类型
- [ ] UI系统完善
- [ ] 道具系统
- [ ] 美术资源
- [ ] 音效完善

---

## 💡 提示

1. **测试优先**：每完成一个功能就运行测试
2. **Git提交**：及时提交代码，保留每个人的commit记录
3. **代码规范**：遵循Google C++ Style Guide
4. **注释充分**：关键函数要有注释说明
5. **性能考虑**：注意对象池、避免频繁new/delete

---

## 📞 需要帮助？

- 查看 [Classes/README_STRUCTURE.md](Classes/README_STRUCTURE.md) 了解详细结构
- 参考学长项目代码：`examples/` 目录
- 阅读Cocos2d-x官方文档：https://docs.cocos.com/cocos2d-x/manual/zh/

**祝开发顺利！🚀**
