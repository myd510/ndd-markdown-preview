/*
 * Notepad-- Markdown 预览渲染层
 * ------------------------------------------------------------
 * 该文件在构建期由 esbuild 打包为 resources/js/preview.bundle.js，
 * 运行期由 Qt WebEngine 加载，对外暴露 window.NDDPreview 全局对象。
 *
 * 支持的完整扩展：
 *   Markdown : CommonMark + GFM(表格/任务列表/删除线/自动链接) + 脚注 + 定义列表
 *              + 缩写 + 属性 {} + 容器 ::: + 上下标 + 高亮 == + 插入 ++ + Emoji
 *              + 目录 TOC + GitHub 警告块 > [!NOTE] + multimd 复杂表格
 *   Mermaid  : 由 mermaid.min.js 提供（外部全局），覆盖全部图类型
 *   LaTeX    : KaTeX，支持行内 $...$ / \( \) 与块级 $$...$$ / \[ \]
 *   代码高亮 : highlight.js
 *   主题     : Markdown 主题（CSS 文件）+ Mermaid 主题（调色板）
 */
import MarkdownIt from 'markdown-it';
import abbr from 'markdown-it-abbr';
import anchor from 'markdown-it-anchor';
import attrs from 'markdown-it-attrs';
import container from 'markdown-it-container';
import deflist from 'markdown-it-deflist';
import footnote from 'markdown-it-footnote';
import { full as emoji } from 'markdown-it-emoji';
import githubAlerts from 'markdown-it-github-alerts';
import ins from 'markdown-it-ins';
import mark from 'markdown-it-mark';
import multimdTable from 'markdown-it-multimd-table';
import sub from 'markdown-it-sub';
import sup from 'markdown-it-sup';
import taskLists from 'markdown-it-task-lists';
import toc from 'markdown-it-toc-done-right';
import katexPluginModule from '@vscode/markdown-it-katex';
import hljs from 'highlight.js';

// @vscode/markdown-it-katex 为 CJS 包，ESM 互操作后可能是 { default: fn }，这里做兼容解包
const katexPlugin =
  typeof katexPluginModule === 'function' ? katexPluginModule : katexPluginModule.default;

/* ------------------------------------------------------------------ *
 * Markdown 引擎
 * ------------------------------------------------------------------ */
const md = new MarkdownIt({
  html: true,          // 允许内联 HTML
  linkify: true,       // 自动识别链接
  typographer: true,   // 智能标点
  breaks: false,
  highlight(code, lang) {
    // Mermaid 代码块：转成 mermaid 容器，交由 mermaid.run 渲染
    if (lang && /^mermaid$/i.test(lang.trim())) {
      return `<div class="mermaid">${md.utils.escapeHtml(code)}</div>`;
    }
    if (lang && hljs.getLanguage(lang)) {
      try {
        return `<pre class="hljs"><code class="language-${lang}">` +
          hljs.highlight(code, { language: lang, ignoreIllegals: true }).value +
          '</code></pre>';
      } catch (e) { /* 落到默认 */ }
    }
    return `<pre class="hljs"><code>${md.utils.escapeHtml(code)}</code></pre>`;
  },
});

md.use(abbr)
  .use(anchor, { permalink: anchor.permalink.headerLink({ safariReaderFix: true }) })
  .use(attrs)
  .use(container, 'info')
  .use(container, 'warning')
  .use(container, 'danger')
  .use(container, 'tip')
  .use(container, 'details')
  .use(deflist)
  .use(footnote)
  .use(emoji)
  .use(githubAlerts)
  .use(ins)
  .use(mark)
  .use(multimdTable, { multiline: true, rowspan: true, headerless: true, multibody: true, autolabel: true })
  .use(sub)
  .use(sup)
  .use(taskLists, { enabled: true, label: true, labelAfter: true })
  .use(toc, { level: [1, 2, 3, 4], listType: 'ul' })
  .use(katexPlugin, { throwOnError: false, errorColor: '#e05252', output: 'html' });

// 让所有链接默认在新窗口打开（由 C++ 侧决定是否外部浏览器打开）
const defaultLinkOpen =
  md.renderer.rules.link_open ||
  function (tokens, idx, options, env, self) {
    return self.renderToken(tokens, idx, options);
  };
md.renderer.rules.link_open = function (tokens, idx, options, env, self) {
  const targetIndex = tokens[idx].attrIndex('target');
  if (targetIndex < 0) tokens[idx].attrPush(['target', '_blank']);
  return defaultLinkOpen(tokens, idx, options, env, self);
};

/* ------------------------------------------------------------------ *
 * Mermaid 主题
 *   - default / dark / forest / neutral : Mermaid 内置主题
 *   - 其余 : 基于 base 主题 + themeVariables 的社区配色
 * ------------------------------------------------------------------ */
const MERMAID_PALETTES = {
  'dracula': {
    dark: true, bg: '#282a36', surface: '#44475a', surface2: '#383a4a', fg: '#f8f8f2',
    line: '#6272a4', accent: '#bd93f9', accent2: '#ff79c6',
    c3: '#8be9fd', c4: '#50fa7b', c5: '#ffb86c', c6: '#f1fa8c', c7: '#ff5555',
  },
  'catppuccin-mocha': {
    dark: true, bg: '#1e1e2e', surface: '#313244', surface2: '#181825', fg: '#cdd6f4',
    line: '#6c7086', accent: '#cba6f7', accent2: '#f5c2e7',
    c3: '#89dceb', c4: '#a6e3a1', c5: '#fab387', c6: '#f9e2af', c7: '#f38ba8',
  },
  'catppuccin-latte': {
    dark: false, bg: '#eff1f5', surface: '#e6e9ef', surface2: '#dce0e8', fg: '#4c4f69',
    line: '#9ca0b0', accent: '#8839ef', accent2: '#ea76cb',
    c3: '#04a5e5', c4: '#40a02b', c5: '#fe640b', c6: '#df8e1d', c7: '#d20f39',
  },
  'nord': {
    dark: true, bg: '#2e3440', surface: '#3b4252', surface2: '#434c5e', fg: '#d8dee9',
    line: '#4c566a', accent: '#88c0d0', accent2: '#b48ead',
    c3: '#8fbcbb', c4: '#a3be8c', c5: '#d08770', c6: '#ebcb8b', c7: '#bf616a',
  },
  'tokyo-night': {
    dark: true, bg: '#1a1b26', surface: '#24283b', surface2: '#16161e', fg: '#a9b1d6',
    line: '#3d59a1', accent: '#7aa2f7', accent2: '#bb9af7',
    c3: '#7dcfff', c4: '#9ece6a', c5: '#ff9e64', c6: '#e0af68', c7: '#f7768e',
  },
  'solarized-dark': {
    dark: true, bg: '#002b36', surface: '#073642', surface2: '#00252e', fg: '#839496',
    line: '#586e75', accent: '#268bd2', accent2: '#6c71c4',
    c3: '#2aa198', c4: '#859900', c5: '#cb4b16', c6: '#b58900', c7: '#dc322f',
  },
  'solarized-light': {
    dark: false, bg: '#fdf6e3', surface: '#eee8d5', surface2: '#f5efdc', fg: '#657b83',
    line: '#93a1a1', accent: '#268bd2', accent2: '#6c71c4',
    c3: '#2aa198', c4: '#859900', c5: '#cb4b16', c6: '#b58900', c7: '#dc322f',
  },
  'one-dark': {
    dark: true, bg: '#282c34', surface: '#3a3f4b', surface2: '#21252b', fg: '#abb2bf',
    line: '#4b5263', accent: '#61afef', accent2: '#c678dd',
    c3: '#56b6c2', c4: '#98c379', c5: '#d19a66', c6: '#e5c07b', c7: '#e06c75',
  },
  'gruvbox': {
    dark: true, bg: '#282828', surface: '#3c3836', surface2: '#32302f', fg: '#ebdbb2',
    line: '#665c54', accent: '#fabd2f', accent2: '#d3869b',
    c3: '#83a598', c4: '#b8bb26', c5: '#fe8019', c6: '#d79921', c7: '#fb4934',
  },
};

// 由调色板生成 Mermaid themeVariables
function paletteToVars(p) {
  return {
    darkMode: !!p.dark,
    background: p.bg,
    primaryColor: p.surface,
    primaryTextColor: p.fg,
    primaryBorderColor: p.accent,
    secondaryColor: p.surface,
    tertiaryColor: p.surface2,
    lineColor: p.line,
    textColor: p.fg,
    mainBkg: p.surface,
    nodeBorder: p.accent,
    nodeTextColor: p.fg,
    clusterBkg: p.surface2,
    clusterBorder: p.line,
    titleColor: p.fg,
    edgeLabelBackground: p.bg,
    labelColor: p.fg,
    labelTextColor: p.fg,
    altBackground: p.surface2,
    classText: p.fg,
    // 时序图
    actorBkg: p.surface,
    actorBorder: p.accent,
    actorTextColor: p.fg,
    actorLineColor: p.line,
    signalColor: p.fg,
    signalTextColor: p.fg,
    labelBoxBkgColor: p.surface,
    labelBoxBorderColor: p.accent,
    loopTextColor: p.fg,
    activationBkgColor: p.surface2,
    activationBorderColor: p.accent,
    noteBkgColor: p.surface2,
    noteTextColor: p.fg,
    noteBorderColor: p.accent,
    // 甘特图
    sectionBkgColor: p.surface2,
    altSectionBkgColor: p.bg,
    sectionBkgColor2: p.surface2,
    taskBkgColor: p.accent,
    taskTextColor: p.bg,
    taskTextOutsideColor: p.fg,
    taskBorderColor: p.accent,
    activeTaskBkgColor: p.accent2,
    activeTaskBorderColor: p.accent2,
    doneTaskBkgColor: p.line,
    doneTaskBorderColor: p.line,
    gridColor: p.line,
    todayLineColor: p.c7,
    // 饼图
    pie1: p.accent, pie2: p.accent2, pie3: p.c3, pie4: p.c4,
    pie5: p.c5, pie6: p.c6, pie7: p.c7, pie8: p.line,
    pieTitleTextColor: p.fg,
    pieSectionTextColor: p.bg,
    pieLegendTextColor: p.fg,
    pieStrokeColor: p.bg,
    pieOuterStrokeColor: p.bg,
    pieOpacity: '0.9',
    // XY 图 / 象限图
    xyChart: {
      backgroundColor: p.bg,
      titleColor: p.fg,
      xAxisLabelColor: p.fg,
      xAxisTitleColor: p.fg,
      xAxisLineColor: p.line,
      yAxisLabelColor: p.fg,
      yAxisTitleColor: p.fg,
      yAxisLineColor: p.line,
      plotColorPalette: [p.accent, p.accent2, p.c3, p.c4].join(','),
    },
    quadrant1Fill: p.c4, quadrant2Fill: p.c3, quadrant3Fill: p.c6, quadrant4Fill: p.c5,
    quadrant1TextFill: p.bg, quadrant2TextFill: p.bg,
    quadrant3TextFill: p.bg, quadrant4TextFill: p.bg,
    quadrantPointFill: p.accent, quadrantPointTextFill: p.bg,
    quadrantXAxisTextFill: p.fg, quadrantYAxisTextFill: p.fg,
    quadrantTitleFill: p.fg,
    quadrantInternalBorderStrokeFill: p.line,
    quadrantExternalBorderStrokeFill: p.line,
    // Git 图
    git0: p.accent, git1: p.accent2, git2: p.c3, git3: p.c4,
    git4: p.c5, git5: p.c6, git6: p.c7, git7: p.line,
    gitInv0: p.bg, gitInv1: p.bg, gitInv2: p.bg, gitInv3: p.bg,
    gitInv4: p.bg, gitInv5: p.bg, gitInv6: p.bg, gitInv7: p.bg,
    commitLabelColor: p.fg, commitLabelBackground: p.surface2,
    tagLabelColor: p.bg, tagLabelBackground: p.accent, tagLabelBorder: p.accent,
    branchLabelColor: p.fg,
    // 思维导图 / 时间线
    cScale0: p.accent, cScale1: p.accent2, cScale2: p.c3, cScale3: p.c4,
    cScale4: p.c5, cScale5: p.c6, cScale6: p.c7, cScale7: p.line,
    cScaleInv0: p.bg, cScaleInv1: p.bg, cScaleInv2: p.bg, cScaleInv3: p.bg,
    cScaleInv4: p.bg, cScaleInv5: p.bg, cScaleInv6: p.bg, cScaleInv7: p.bg,
    cScaleLabel0: p.fg, cScaleLabel1: p.fg, cScaleLabel2: p.fg, cScaleLabel3: p.fg,
    cScaleLabel4: p.fg, cScaleLabel5: p.fg, cScaleLabel6: p.fg, cScaleLabel7: p.fg,
  };
}

let currentMermaidTheme = 'dark';
let mermaidReady = false;

function initMermaid() {
  const mermaid = window.mermaid;
  if (!mermaid) return false;

  const name = currentMermaidTheme;
  const builtin = ['default', 'dark', 'forest', 'neutral'];
  const cfg = {
    startOnLoad: false,
    securityLevel: 'loose',
    fontFamily: 'Consolas, "Microsoft YaHei", monospace',
    flowchart: { useMaxWidth: true, htmlLabels: true },
    sequence: { useMaxWidth: true },
    gantt: { useMaxWidth: true },
    theme: builtin.indexOf(name) >= 0 ? name : 'base',
  };
  if (builtin.indexOf(name) < 0 && MERMAID_PALETTES[name]) {
    cfg.themeVariables = paletteToVars(MERMAID_PALETTES[name]);
  }

  mermaid.initialize(cfg);
  mermaidReady = true;
  return true;
}

let mermaidSeq = 0;
async function renderMermaid() {
  const mermaid = window.mermaid;
  if (!mermaid) return;
  if (!mermaidReady) initMermaid();

  const nodes = Array.from(document.querySelectorAll('.mermaid'));
  for (const node of nodes) {
    const code = node.textContent || '';
    try {
      const id = 'ndd-mermaid-' + (mermaidSeq++);
      const { svg } = await mermaid.render(id, code);
      node.innerHTML = svg;
      node.setAttribute('data-processed', 'true');
    } catch (err) {
      node.innerHTML =
        '<div class="ndd-error"><div class="ndd-error-title">Mermaid 渲染失败</div>' +
        '<pre>' + md.utils.escapeHtml(String(err && err.message ? err.message : err)) + '</pre></div>';
    }
  }
}

/* ------------------------------------------------------------------ *
 * Markdown 主题（CSS 文件切换）
 * ------------------------------------------------------------------ */
function setLinkEnabled(id, enabled) {
  const el = document.getElementById(id);
  if (el) el.disabled = !enabled;
}

// file 为相对 preview.html 的 URL；dark 决定代码高亮配色与 data-theme
function applyMarkdownTheme(file, dark) {
  const link = document.getElementById('css-theme');
  if (link && file && link.getAttribute('href') !== file) {
    link.setAttribute('href', file);
  }
  document.documentElement.setAttribute('data-theme', dark ? 'dark' : 'light');
  setLinkEnabled('css-hl-light', !dark);
  setLinkEnabled('css-hl-dark', dark);
}

/* ------------------------------------------------------------------ *
 * 对外 API
 * ------------------------------------------------------------------ */
let lastText = '';

window.NDDPreview = {
  /** 渲染 markdown 文本。opts: { preserveScroll:boolean } */
  async render(text, opts) {
    opts = opts || {};
    lastText = text == null ? '' : String(text);

    const scroller = document.getElementById('scroller');
    const prevTop = scroller ? scroller.scrollTop : 0;

    document.getElementById('content').innerHTML = md.render(lastText);

    // 外链标记，交给 C++ 侧用系统浏览器打开
    document.querySelectorAll('#content a[href^="http"]').forEach((a) => {
      a.setAttribute('rel', 'noopener noreferrer');
    });

    await renderMermaid();

    if (opts.preserveScroll && scroller) scroller.scrollTop = prevTop;
    return true;
  },

  /** 切换 Markdown 主题（只换 CSS，无需重新解析） */
  async setMarkdownTheme(file, dark) {
    applyMarkdownTheme(file, dark);
    return true;
  },

  /** 切换 Mermaid 主题（需重绘图表） */
  async setMermaidTheme(id) {
    currentMermaidTheme = id || 'default';
    initMermaid();
    if (lastText) {
      await this.render(lastText, { preserveScroll: true });
    }
    return true;
  },

  /** 返回当前正文 HTML，供导出使用 */
  getHtml() {
    return document.getElementById('content').innerHTML;
  },

  /** 页面是否就绪 */
  ready() { return true; },
};

// 页面就绪后通知宿主（C++ 侧监听）
window.addEventListener('DOMContentLoaded', () => {
  document.documentElement.setAttribute('data-ready', '1');
});
