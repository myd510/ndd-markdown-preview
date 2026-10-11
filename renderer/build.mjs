/*
 * 渲染层构建脚本
 *   1) 用 esbuild 把 src/index.js（markdown-it + 全套插件 + KaTeX + highlight.js）
 *      打包为单文件 IIFE：resources/js/preview.bundle.js
 *   2) 复制 mermaid.min.js、KaTeX CSS/字体、GitHub 主题 CSS、highlight.js 主题 CSS
 * 用法：node build.mjs
 */
import { build, transform } from 'esbuild';
import { mkdirSync, copyFileSync, readdirSync, existsSync, rmSync, readFileSync, writeFileSync } from 'node:fs';
import { dirname, join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';

const __dirname = dirname(fileURLToPath(import.meta.url));
const ROOT = resolve(__dirname, '..');
const NM = join(__dirname, 'node_modules');
const JS_OUT = join(ROOT, 'resources', 'js');
const CSS_OUT = join(ROOT, 'resources', 'css');
const FONT_OUT = join(CSS_OUT, 'fonts');

/*
 * 目标内核：Qt 5.14.x 的 QtWebEngine = Chromium 77。
 * 把目标定在 chrome77，esbuild 会把 ?. / ?? 等语法降级，
 * 配合 polyfills.js 补齐 Chromium 77 缺失的运行时 API，
 * 从而在 Qt 5.14 / 5.15 / 6.x 上都能运行。
 */
const BROWSER_TARGET = ['chrome77'];

function ensure(dir) { mkdirSync(dir, { recursive: true }); }
function copy(src, dst) {
  if (!existsSync(src)) throw new Error('缺少文件: ' + src);
  ensure(dirname(dst));
  copyFileSync(src, dst);
  console.log('  copy', src.replace(NM, 'node_modules') + '  ->  ' + dst.replace(ROOT, '.'));
}

console.log(`[1/4] 打包 markdown-it 渲染内核 (target=${BROWSER_TARGET}) ...`);
await build({
  entryPoints: [join(__dirname, 'src', 'index.js')],
  bundle: true,
  format: 'iife',
  target: BROWSER_TARGET,
  platform: 'browser',
  minify: true,
  legalComments: 'none',
  outfile: join(JS_OUT, 'preview.bundle.js'),
  logLevel: 'info',
});

console.log(`[2/4] 降级 Mermaid 到 ${BROWSER_TARGET} ...`);
{
  const src = readFileSync(join(NM, 'mermaid', 'dist', 'mermaid.min.js'), 'utf8');
  const out = await transform(src, {
    loader: 'js',
    target: BROWSER_TARGET,
    minify: true,
    legalComments: 'none',
  });
  ensure(JS_OUT);
  writeFileSync(join(JS_OUT, 'mermaid.min.js'), out.code, 'utf8');
  console.log('  transpile node_modules/mermaid/dist/mermaid.min.js -> ./resources/js/mermaid.min.js');
}

console.log('[3/4] 复制静态资源 ...');
ensure(JS_OUT); ensure(CSS_OUT); ensure(FONT_OUT);

// Chromium 77 兼容垫片（原样拷贝，语法已按 ES5/ES2015 编写）
copy(join(__dirname, 'src', 'polyfills.js'), join(JS_OUT, 'polyfills.js'));

// KaTeX 样式 + 字体（只保留 woff2，Chromium 内核已足够）
copy(join(NM, 'katex', 'dist', 'katex.min.css'), join(CSS_OUT, 'katex.min.css'));
rmSync(FONT_OUT, { recursive: true, force: true });
ensure(FONT_OUT);
const katexFonts = join(NM, 'katex', 'dist', 'fonts');
let fontCount = 0;
for (const f of readdirSync(katexFonts)) {
  if (f.endsWith('.woff2')) { copy(join(katexFonts, f), join(FONT_OUT, f)); fontCount++; }
}
console.log('  katex fonts:', fontCount, '个 woff2');

// highlight.js 主题
copy(join(NM, 'highlight.js', 'styles', 'github.min.css'), join(CSS_OUT, 'highlight-github-light.min.css'));
copy(join(NM, 'highlight.js', 'styles', 'github-dark.min.css'), join(CSS_OUT, 'highlight-github-dark.min.css'));

// 说明：GitHub Markdown 主题已由下面的「主题」步骤统一拷贝到
// resources/themes/markdown/，此处不再重复拷贝到 css/。

// ------------------------------------------------------------------
// 主题：按 renderer/src/themes.json 拷贝 Markdown 主题 CSS，
// 并生成运行期清单 resources/themes/themes.json
// ------------------------------------------------------------------
console.log('[4/4] 主题 ...');
const themeDef = JSON.parse(readFileSync(join(__dirname, 'src', 'themes.json'), 'utf8'));
const MD_THEME_OUT = join(ROOT, 'resources', 'themes', 'markdown');
ensure(MD_THEME_OUT);

const manifest = { markdown: [], mermaid: themeDef.mermaid };
for (const t of themeDef.markdown) {
  const srcPath = join(NM, t.src);
  const dstPath = join(MD_THEME_OUT, t.out);

  if (t.keepFonts) {
    // 保留内嵌/外链字体（会让插件显著变大）
    copy(srcPath, dstPath);
  } else {
    // 默认剥离 @font-face：字体文件动辄数 MB（latex.css 内嵌 Latin Modern 达 3 MB），
    // 而各主题的 font-family 回退栈都写得比较完整（如 Latin Modern,Georgia,Times,serif），
    // 去掉后仍能保持排版风格，只是改用系统字体。
    const css = readFileSync(srcPath, 'utf8');
    const stripped = css.replace(/@font-face\s*\{[^}]*\}\s*/gi, '');
    ensure(dirname(dstPath));
    writeFileSync(dstPath, stripped, 'utf8');
    const saved = css.length - stripped.length;
    if (saved > 0) {
      console.log(`  strip fonts: ${t.src} -> themes/markdown/${t.out}  (省 ${Math.round(saved / 1024)} KB)`);
    } else {
      console.log(`  copy        ${t.src} -> themes/markdown/${t.out}`);
    }
  }

  manifest.markdown.push({
    id: t.id,
    label: t.label,
    file: 'themes/markdown/' + t.out,   // 相对 preview.html
    dark: !!t.dark,
  });
}
writeFileSync(
  join(ROOT, 'resources', 'themes', 'themes.json'),
  JSON.stringify(manifest, null, 2),
  'utf8',
);
console.log(`  themes.json: ${manifest.markdown.length} 个 Markdown 主题, ${manifest.mermaid.length} 个 Mermaid 主题`);

console.log('\n构建完成 ✓');

