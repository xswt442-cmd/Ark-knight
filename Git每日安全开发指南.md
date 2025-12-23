# Git 多分支开发与安全合并指南 (Main + Backup)

本指南旨在规范 Git 提交路径，目标是：

1. **保持主干清晰**：让提交树呈现“主干清晰，分支合并成圈（气泡）”的完美形态。
2. **安全第一**：引入 `backup1` 分支作为“防爆盾”，所有合并必须先在备份分支验证，确保无误后再合入主干。

------

### 0. 首次配置 (One-time Setup)

如果你本地还没有 `backup1` 分支，请先执行以下命令将其拉取到本地：

Bash

```
git fetch origin
git checkout -b backup1 origin/backup1
# 此时你本地应该同时拥有 main 和 backup1 两个分支
```

------

### 1. 每日工作起手式 (Daily Sync)

**非常重要**：每天开始写代码前，必须确保你的 `main` 和 `backup1` 都是最新的，避免合并冲突。

使用 `git pull --rebase` 可以保持本地历史整洁（防止产生无意义的 "Merge branch..." 节点）。

Bash

```
# 1. 更新主干
git checkout main
git pull --rebase

# 2. 更新备份分支
git checkout backup1
git pull --rebase
```

------

### 2. 开发阶段：新建分支 (Feature Branch)

永远不要在 `main` 或 `backup1` 上直接写代码。任何修改（新功能、修Bug、改资源）都要切新分支。

**步骤：**

1. **从主干切出新分支**：

   Bash

   ```
   git checkout main
   git checkout -b feature/scene-switch  # 分支名举例
   ```

2. **提交代码**： 保持提交信息规范（Conventional Commits），便于生成清晰的提交气泡。

   Bash

   ```
   git add .
   git commit -m "[feat]在新地图上配置了场景切换的boss trigger"
   
   git add .
   git commit -m "[fix]修复场景加载时的黑屏bug"
   ```
   
   **⚠️ 警告：如何同步代码？**
   
   如果在开发过程中需要同步最新的代码：
   
   1. **严禁** 执行 git merge backup1！不要把备份分支合到你的功能分支里。
   2. **严禁** 执行 git merge main！这会产生多余的合并气泡。
   3. **正确做法**：使用 Rebase 同步主干。
   
   ```
   git fetch origin
   git rebase origin/main
   ```

------

### 3. 关键步骤：双重合并流程 (The "Verify-then-Merge" Flow)

开发完成后，**不要直接合入 main**。我们需要先在 `backup1` 上“试错”。

#### 第一步：在 Backup 分支验证 (Safety Check)

这个步骤用来测试合并是否存在冲突，或者是否会破坏项目运行。

Bash

```
# 1. 切换到备份分支
git checkout backup1

# 2. 使用 --no-ff 强制生成合并节点（关键！）
# 格式：git merge --no-ff <你的功能分支> -m "<合并信息>"
git merge --no-ff feature/scene-switch -m "[merge]测试合并：场景切换组件"
```

- **如果合并有冲突**：在这里解决冲突，不会影响主干。
- **如果运行有Bug**：在这里修复，或者回退，主干依然是干净的。

#### 第二步：确认无误后，合入 Main (Production Merge)

只有在 `backup1` 上合并成功且项目运行正常后，才执行此步。

Bash

```
# 1. 切换回主干
git checkout main

# 2. 正式合并（同样必须用 --no-ff）
git merge --no-ff feature/scene-switch -m "[merge]实现场景(探索/战斗)切换组件"

# 3. 推送主干
git push origin main
```

------

### 4. 流程总结 (Cheat Sheet)

把这套连招练熟：

1. **早起更新**：`main` 和 `backup1` 都 `pull --rebase` 一下。
2. **干活**：`git checkout -b fix/bug-1` -> 写代码 -> `commit`。
3. **试合 (Backup)**：
   - `git checkout backup1`
   - `git merge --no-ff fix/bug-1`
   - *(跑一下项目，没问题？继续)*
4. **实合 (Main)**：
   - `git checkout main`
   - `git merge --no-ff fix/bug-1`
   - `git push`
5. **收尾**：`git branch -d fix/bug-1` (删除本地功能分支)

### 为什么这样做？

- **`--no-ff`**：保证了 Git Graph 上那个漂亮的“气泡”结构，以后回溯历史非常清晰，知道哪些代码属于同一个功能。
- **`backup1`**：是我们的“试验场”。如果直接合 `main` 炸了，全队都要停下来等你修；但在 `backup1` 炸了，只有你自己知道，修好再合主干即可。