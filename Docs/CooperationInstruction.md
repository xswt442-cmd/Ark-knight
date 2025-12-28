# Ark-knight 团队协作流程

---

## 1. 协作基本原则

- **禁止直接在 main 分支开发**
- **所有合并必须先在 backup 分支验证**
- **保持提交信息规范化**（详见下方提交规范表）
- **每日同步代码，避免大规模冲突**

---

## 2. 模块分工

| 模块 | 负责范围 | 主要文件 |
|------|----------|----------|
| **Core/Entities** | 玩家、敌人、游戏实体 | `Classes/Entities/`, `Classes/Core/` |
| **Map** | 地图生成、房间系统 | `Classes/Map/` |
| **UI/Scenes** | 界面、场景管理 | `Classes/UI/`, `Classes/Scenes/` |
| **Managers/Utils** | 管理器、工具类 | `Classes/Managers/`, `Classes/Utils/` |

---

## 3. Git 协作流程

### 日常开发四步法

1. **同步最新代码**
   ```bash
   git checkout main && git pull --rebase
   git checkout backup && git pull --rebase
   ```

2. **创建功能分支**
   ```bash
   git checkout main
   git checkout -b feat/your-feature
   # 开发并提交
   git add . && git commit -m "[Feat] 功能描述"
   ```

3. **backup分支验证**（安全检查）
   ```bash
   git checkout backup
   git merge --no-ff feat/your-feature -m "[Merge] 测试合并：功能描述"
   # 运行测试，确保无问题
   ```

4. **合并到main**（正式上线）
   ```bash
   git checkout main
   git merge --no-ff feat/your-feature -m "[Merge] 实现功能：功能描述"
   git push origin main
   git checkout backup && git push origin backup
   ```

### 注意事项
- **必须使用 `--no-ff`**：保持清晰的合并历史
- **backup分支是试验场**：在这里发现问题不会影响main
- **频繁commit**：完成小功能就add/rm并commit，便于问题定位与记录历史

---

## 4. 提交信息规范

### 格式
```
[类型(模块)] 简短描述
```

### 类型表

| 类型 | 含义 | 示例 |
|------|------|------|
| **[Feat/Add]** | 新功能 | `[Feat] 添加角色冲刺功能` |
| **[Fix]** | 修复Bug | `[Fix] 修复敌人死亡后不消失的问题` |
| **[Refactor]** | 代码重构 | `[Refactor] 重构Room类，独立出Portal` |
| **[Docs]** | 文档修改 | `[Docs] 完善敌人说明文档` |
| **[Request]** | 问题提示 | `[Request] 目前存在菜单显示bug，请修复` |
| **[Chore]** | 杂项 | `[Chore] 更新杂项文件` |
| **[Merge]** | 合并操作 | `[Merge] 重构并独立出Portal类` |

### 提交最佳实践
- **描述具体**：不要写空洞描述
- **一次一件事**：一个commit最好只做一个功能点
- **中英文都可以**：以团队理解为准

---

## 5. 问题处理

### Bug修复优先级
- **P0**：崩溃、无法运行 → 立即修复
- **P1**：核心功能异常 → 当日修复  
- **P2**：次要功能问题 → 3日内修复
- **P3**：体验优化 → 版本迭代时修复

### 代码冲突解决
1. **预防为主**：每日同步代码，避免大规模冲突
2. **在backup分支发现冲突**：由修改者解决，必要时协商
3. **解决步骤**：
   ```bash
   # 解决冲突文件
   git add 解决好的文件
   git rebase --continue  # 不要用 git commit
   ```

---

## 6. 代码规范

### 编码风格
- **统一使用Google C++ Style Guide**
- **变量命名**：驼峰命名法，成员变量加下划线前缀（如 `_player`）
- **注释规范**：重要逻辑必须有中文注释
- **文件组织**：头文件和实现文件分离，相关文件放在同一目录

### 项目结构
```
Classes/
├── Core/              # 核心常量、宏定义
├── Entities/          # 游戏实体
│   ├── Base/          #   基类（GameEntity, Character）
│   ├── Player/        #   玩家类（Player, Mage, Gunner, Warrior）
│   ├── Enemy/         #   敌人类（各种怪物和Boss）
│   └── Objects/       #   游戏对象（Item, Chest, ItemDrop, Portal）
├── Map/               # 地图系统
├── Scenes/            # 场景管理
├── UI/                # 用户界面
├── Managers/          # 管理器类
└── Utils/             # 工具函数
```

---

## 7. 协作技巧

### 开发建议
- **小步快跑**：频繁提交，避免大批量修改
- **及时沟通**：遇到问题主动寻求帮助  
- **代码复用**：优先使用已有组件，避免重复造轮子
- **测试验证**：重要功能要在backup分支充分测试，或自建新备用分支大型测试

### 常见问题
- **Q: merge冲突怎么办？**
  A: 在backup分支解决，不要在main上直接处理
  
- **Q: 忘记创建分支直接在main开发了？** 
  A: 立即创建分支并切换：`git checkout -b feat/your-work`
  
- **Q: commit message写错了？**
  A: 使用 `git commit --amend` 修改最后一次提交信息

---

通过遵循本流程，确保代码质量稳定、团队协作高效、项目进展可控。