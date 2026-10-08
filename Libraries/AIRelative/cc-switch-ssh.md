# SSH 下用 CC-Switch CLI 配置 Codex

先在 VSCode Remote SSH 中安装 Codex 扩展，以下命令在远程集成终端执行。默认配置目录为 `~/.codex`。

## 1. 安装

```bash
curl -fsSL https://github.com/SaladDay/cc-switch-cli/releases/latest/download/install.sh | bash
cc-switch --version
```

## 2. 添加供应商

```bash
cc-switch
```

在 TUI 中选择 Codex，添加供应商并填写：

- 名称：`My Codex`
- API 地址：`https://www.shenlanqaq.com/v1`
- API Key：自己的有效密钥
- 模型：`gpt-6.1-sol`

若界面提供相应选项，协议选择 `responses`，关闭 OpenAI 账号认证，推理强度选择 `medium`。

## 3. 启用供应商

在 TUI 中启用，或通过命令切换：

```bash
cc-switch --app codex provider list
cc-switch --app codex provider switch <供应商ID>
```

## 4. 重载 VSCode

在远程窗口按 `Ctrl+Shift+P`，执行 `Developer: Reload Window`，然后打开 Codex 新建对话。

[CC-Switch CLI 中文文档](https://github.com/SaladDay/cc-switch-cli/blob/main/README_ZH.md)
