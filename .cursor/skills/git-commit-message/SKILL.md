---
name: git-commit-message
description: >-
  Reads staged git changes and generates a commit message with a one-sentence
  summary and bullet-point details. Use when the user asks to write, generate,
  or suggest a commit message, or mentions staged changes, git commit, or
  提交信息 / commit message.
disable-model-invocation: true
---

# Git Commit Message

Generate commit messages from **staged** changes only. Output one summary sentence plus bullet points. Do not run `git commit` unless the user explicitly asks to commit.

## Workflow

### Step 1: Gather context (run in parallel)

```bash
git status
git diff --staged
git log -5 --oneline
```

Optional when the diff is large or split across many files:

```bash
git diff --staged --stat
git diff --staged --name-status
```

If nothing is staged, say so and suggest `git add` — do not invent changes.

### Step 2: Analyze the diff

Focus on **why** and **impact**, not a file-by-file inventory.

Group related hunks into logical changes. Prefer fewer, meaningful bullets over one bullet per touched file.

Skip noise: formatting-only edits, import reordering, generated lockfiles (unless that is the main purpose).

Do not include secrets (.env, keys, credentials). Warn if staged files look sensitive.

### Step 3: Choose language and style

- Match recent commit language from `git log` when clear.
- Otherwise match the user's language in the current request.
- Follow existing repo conventions (Conventional Commits, prefixes, tense) when present in `git log`.

### Step 4: Write the message

Use this structure:

```text
<一句话总结>

- <更新点 1：做了什么、为何重要>
- <更新点 2>
- <更新点 3（如有）>
```

Rules for the summary line:

- One sentence, imperative mood (e.g.「添加…」「修复…」「重构…」或 `Add…` / `Fix…`).
- Capture the **main intent** of the whole staged set, not the first file in the diff.
- Keep it concise; aim for ≤ 72 characters when possible.

Rules for bullets:

- 2–5 bullets for typical changes; 1 is fine for tiny diffs; merge or cap long lists.
- Each bullet = one logical change (feature, fix, refactor, docs, test, config).
- Start with a verb; mention component or module when it aids clarity.
- No trailing period unless the repo consistently uses them.

## Output to the user

Present the message in a fenced code block so it is easy to copy:

```text
fix(player): 修复 GStreamer pipeline 在 EOS 后未正确释放的问题

- 在 bus 回调中调用 gst_element_set_state(NULL) 释放 pipeline
- 移除重复的 unref，避免 double-free
- 补充 EOS 场景的单元测试
```

If multiple distinct themes are staged (unrelated refactors + feature), note that and either:

1. Suggest splitting into separate commits, or
2. Offer one combined message only if the user wants a single commit.

When the user asks to **commit**, use the generated message with a HEREDOC:

```bash
git commit -m "$(cat <<'EOF'
一句话总结

- 更新点 1
- 更新点 2

EOF
)"
```

Follow the user's git safety rules: never `--no-verify`, never amend unless explicitly allowed, never force-push.

## Examples

**Feature addition**

Staged: new API endpoint + route registration + tests

```text
feat(api): 添加用户配置查询接口

- 新增 GET /api/v1/user/config 及 handler 实现
- 在 router 中注册路由并接入鉴权中间件
- 补充正常路径与 404 的集成测试
```

**Bug fix**

Staged: null check + error logging in one module

```text
fix(cache): 修复缓存键为空时导致 panic

- 在 Get 前校验 key，空值返回 ErrInvalidKey
- 增加 debug 日志便于定位异常调用栈
```

**Docs only**

```text
docs(gstreamer): 补充 application 笔记中的 pipeline 生命周期说明

- 描述 PLAYING → PAUSED → NULL 的状态转换
- 添加 EOS 与 error 处理的示例代码片段
```

**Refactor**

```text
refactor(auth): 将 token 校验逻辑提取为独立 middleware

- 从 handler 中剥离 ValidateToken，统一错误响应格式
- 更新受影响的三个 handler，行为保持不变
```

## Quality checklist

Before returning the message, verify:

- [ ] Summary reflects the full staged change set
- [ ] Bullets are grouped by intent, not raw diff order
- [ ] Wording matches repo / user language
- [ ] No secrets or irrelevant noise
- [ ] User did not ask to commit, or commit command uses the exact generated text
