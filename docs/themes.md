# 主题清单（Markdown / Mermaid / LaTeX）

本文档汇总本插件已内置的主题，以及更多可用的开源主题方案（含链接与许可证），
便于按需扩展。

---

## 一、插件已内置的主题

Markdown 主题与 Mermaid 主题**相互独立**，分别在菜单的 **「Markdown 主题」** 与
**「Mermaid 主题」** 两个子菜单中切换，选择会被记住（QSettings）。

### 1.1 Markdown 主题（10 个，切换 CSS）

| 菜单名 | id | 风格 | 上游项目 | 许可证 |
|---|---|---|---|---|
| GitHub 浅色 | `github-light` | GitHub 官方排版，清爽（**默认**） | [github-markdown-css](https://github.com/sindresorhus/github-markdown-css) | MIT |
| GitHub 深色 | `github-dark` | GitHub 暗色 | 同上 | MIT |
| Water 浅色 | `water-light` | 极简、无类名，柔和的蓝色点缀 | [Water.css](https://github.com/kognise/water.css) | MIT |
| Water 深色 | `water-dark` | 同上，暗色版 | 同上 | MIT |
| Sakura 粉樱 | `sakura` | 樱花粉，温暖柔和 | [Sakura](https://github.com/oxalorg/sakura) | MIT |
| Sakura 深色 | `sakura-dark` | 樱花暗色 | 同上 | MIT |
| Sakura 大地 | `sakura-earthly` | 大地色系，护眼 | 同上 | MIT |
| Marx 复古 | `marx` | 复古印刷风，衬线标题 | [Marx](https://github.com/marx-css/marx) | MIT |
| LaTeX 论文 | `latex` | 拉丁现代字体，论文/作业风 | [LaTeX.css](https://github.com/vincentdoerig/latex-css) | MIT |
| Tufte 书籍 | `tufte` | Edward Tufte 书籍排版，宽边注 | [Tufte CSS](https://github.com/edwardtufte/tufte-css) | MIT |

> **默认主题**：Markdown = `github-light`（GitHub 浅色），Mermaid = `default`（Mermaid 默认）。
> 选择会被记住，可在菜单中随时切换。
>
> **关于字体**：构建时会**剥离所有 `@font-face`**（只保留样式）。原因：`latex.css` 内嵌
> Latin Modern 字体使单文件达 3 MB，会让插件从 1.8 MB 涨到 5 MB；各主题的
> `font-family` 回退栈都比较完整（如 `Latin Modern,Georgia,Cambria,Times,serif`），
> 去掉后仍保持排版风格，只是改用系统字体。剥离后 10 套主题合计仅约 **150 KB**。
> 若某个主题确实需要自带字体，在其 `themes.json` 条目上加 `"keepFonts": true` 即可。

### 1.2 Mermaid 主题（13 个，切换调色板）

| 菜单名 | id | 类型 |
|---|---|---|
| Mermaid 默认 | `default` | Mermaid 内置（**默认**） |
| Mermaid 深色 | `dark` | Mermaid 内置 |
| Mermaid 森林 | `forest` | Mermaid 内置 |
| Mermaid 中性 | `neutral` | Mermaid 内置（黑白灰，适合打印） |
| Dracula | `dracula` | 社区配色 |
| Catppuccin Mocha | `catppuccin-mocha` | 社区配色 |
| Catppuccin Latte | `catppuccin-latte` | 社区配色 |
| Nord | `nord` | 社区配色 |
| Tokyo Night | `tokyo-night` | 社区配色 |
| Solarized Dark | `solarized-dark` | 社区配色 |
| Solarized Light | `solarized-light` | 社区配色 |
| One Dark | `one-dark` | 社区配色 |
| Gruvbox | `gruvbox` | 社区配色 |

社区配色通过 Mermaid 官方的 `theme: 'base'` + `themeVariables` 实现（见
[官方主题文档](https://mermaid.js.org/config/theming.html)），覆盖节点、连线、
时序图、甘特图、饼图、Git 图、思维导图、象限图、XY 图等全部图元。

### 1.3 LaTeX（KaTeX）

KaTeX 本身没有"主题"概念，公式颜色通过 `currentColor` **自动跟随** Markdown 主题
的正文色，因此在深色/浅色主题下都能正常显示。插件已内嵌 KaTeX 样式与 20 个 woff2
字体，离线可用。

需要注意的例外：**显式指定颜色的公式不会自适应**，例如 `\color{black}{x}`
在深色主题下会看不见。建议用 `\color{red}` 这类彩色，或干脆不指定颜色。

---

## 二、更多可选的 Markdown 主题（未内置）

以下都是开源项目，可按 §四 的方法自行加入。

### 2.1 无类名（classless）CSS 框架

这类框架直接美化裸 HTML 标签，与 Markdown 输出天然契合：

| 项目 | 特点 | 许可证 |
|---|---|---|
| [Pico.css](https://github.com/picocss/pico) | 现代、语义化，自带明暗与多种主色（amber/blue/…） | MIT |
| [new.css](https://github.com/xz/new.css) | 极简，仅 ~4.5 KB | MIT |
| [Simple.css](https://github.com/kevquirk/simple.css) | 轻量，开箱即用 | MIT |
| [Water.css](https://github.com/kognise/water.css) | 已内置 | MIT |
| [Sakura](https://github.com/oxalorg/sakura) | 已内置 | MIT |
| [Marx](https://github.com/marx-css/marx) | 已内置 | MIT |

> 汇总清单：[dbohdan/classless-css](https://github.com/dbohdan/classless-css)
> （收录 40+ 个无类 CSS 框架，含在线预览）

### 2.2 Markdown 专用预览主题

| 项目 | 说明 | 许可证 |
|---|---|---|
| [mweb-themes](https://github.com/imageslr/mweb-themes) | **30+ 款 Markdown 预览主题**，含 Typo.css，附主题开发模板 | MIT |
| [Typora 主题](https://github.com/typora/typora-default-themes) | Typora 官方默认主题（Vue / Newsprint / Night / Pixyll / Whitey 等）；社区主题在 [typora 组织](https://github.com/typora) 下 | MIT |
| [mdBook 主题](https://github.com/rust-lang/mdBook) | Rust 官方书籍工具，内置 Light / Rust / Coal / Navy / Ayu 五套 | MPL-2.0 |
| [Docsify 主题](https://github.com/docsifyjs/docsify) | vue / dark / buble / pure / dolphin | MIT |
| [Tailwind Typography](https://github.com/tailwindlabs/tailwindcss-typography) | `prose` 类，适合与 Tailwind 生态配合 | MIT |
| [LaTeX.css](https://github.com/vincentdoerig/latex-css) | 已内置 | MIT |
| [Tufte CSS](https://github.com/edwardtufte/tufte-css) | 已内置 | MIT |
| [github-markdown-css](https://github.com/sindresorhus/github-markdown-css) | 已内置 | MIT |

### 2.3 代码高亮主题

当前内置 GitHub 明/暗两套（来自 highlight.js）。highlight.js 自带 **200+ 套**
主题，位于 `node_modules/highlight.js/styles/`，常用的还有：

`atom-one-dark`、`monokai`、`dracula`、`nord`、`tokyo-night-dark`、
`vs2015`、`xt256`、`a11y-dark`、`stackoverflow-light` 等。

替换方法：把对应 CSS 拷到 `resources/css/` 并在 `preview.html` 中改 `<link>` 即可。

---

## 三、更多可选的 Mermaid 主题

| 来源 | 说明 |
|---|---|
| [Mermaid 官方主题文档](https://mermaid.js.org/config/theming.html) | `default` / `dark` / `forest` / `neutral` 四个内置主题 + 完整 `themeVariables` 变量表（约 100 个变量），照此可自定义任意配色 |
| [Catppuccin](https://github.com/catppuccin) | 社区配色组织，含 [mermaid 端口](https://github.com/catppuccin/mermaid)，4 个 flavor × 26 色 |
| [Dracula](https://github.com/dracula/mermaid) | 高对比暗紫配色，各编辑器/工具均有端口 |
| [Nord](https://github.com/nordtheme/nord) | 北极蓝灰，低饱和 |
| [Tokyo Night](https://github.com/folke/tokyonight.nvim) | 现代暗色，含多个变体 |
| [Gruvbox](https://github.com/morhetz/gruvbox) | 复古暖色 |
| [Solarized](https://github.com/altercation/solarized) | 精确对比度设计，明暗双版 |
| [One Dark](https://github.com/atom/one-dark-syntax) | Atom 编辑器经典配色 |
| Mermaid Boost（Obsidian 插件） | 29 套主题分组（Blueprint / Newspaper / Chalkboard / Synthwave 等），可参考其配色与滤镜思路 |

> 插件已内置上表中除"官方文档 + Mermaid Boost"以外的全部配色。

---

## 四、如何新增主题

### 4.1 新增 Markdown 主题

1. 把 CSS 加入依赖：`cd renderer && npm install <包名>`
2. 在 `renderer/src/themes.json` 的 `markdown` 数组里追加一项：

   ```json
   { "id": "pico-light", "label": "Pico 浅色",
     "src": "@picocss/pico/css/pico.classless.min.css",
     "out": "pico-light.css", "dark": false }
   ```

   - `src`：`node_modules` 下的相对路径
   - `out`：拷到 `resources/themes/markdown/` 后的文件名
   - `dark`：true 表示深色主题（决定代码高亮配色与 `data-theme`）

3. 重建资源并更新资源清单：

   ```bash
   cd renderer && node build.mjs
   ```

   然后把新生成的 `resources/themes/markdown/<out>` 按同样格式补进
   `resources/resources.qrc`（新增文件才需要）。

4. 重新编译插件即可，菜单会自动出现新主题（C++ 侧从 `themes.json` 动态构建菜单）。

### 4.2 新增 Mermaid 主题

在 `renderer/src/index.js` 的 `MERMAID_PALETTES` 中按现有格式加一个调色板
（字段：`dark/bg/surface/surface2/fg/line/accent/accent2/c3..c7`），
再在 `renderer/src/themes.json` 的 `mermaid` 数组里加 `{ "id": "...", "label": "..." }`，
然后执行 §4.1 的第 3、4 步。

### 4.3 主题清单的单一来源

`renderer/src/themes.json` 是唯一的主题清单来源；`build.mjs` 据此拷贝 CSS 并生成
运行期清单 `resources/themes/themes.json`，C++ 侧读取后者来构建菜单。

---

## 五、说明

所有内置主题均已逐一验证：10 套 Markdown 主题的 CSS 可正常加载，
13 套 Mermaid 配色均可成功重绘全部图表类型。
