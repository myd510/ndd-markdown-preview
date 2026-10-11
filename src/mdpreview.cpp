#include "mdpreview.h"
#include "pluginctx.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QDockWidget>
#include <QFile>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QLabel>
#include <QMainWindow>
#include <QMessageBox>
#include <QSettings>
#include <QTimer>
#include <QToolButton>
#include <QUrl>
#include <QVBoxLayout>
#include <QWebEngineSettings>
#include <QWebEngineView>
#include <QtGlobal>
#include "qsciscintilla_ndd.h"

namespace {
QString readResource(const QString& path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        return QString();
    }
    return QString::fromUtf8(f.readAll());
}

// 把 QString 编码成合法的 JS 字符串字面量。
// 注意：QJsonDocument 只能承载对象或数组，fromVariant(QString) 得到的是标量文档，
// toJson() 会返回**空**；必须包一层数组再剥掉中括号。
QString jsStringLiteral(const QString& text)
{
    QJsonArray arr;
    arr.append(text);
    QString json = QString::fromUtf8(QJsonDocument(arr).toJson(QJsonDocument::Compact));
    if (json.size() >= 2) {
        json.remove(0, 1);   // 去掉开头的 '['
        json.chop(1);        // 去掉结尾的 ']'
    }
    return json.isEmpty() ? QStringLiteral("\"\"") : json;
}

const char* kSettingsOrg = "ndd";
const char* kSettingsApp = "markdown-preview";
} // namespace

/* ================================================================== *
 * PreviewPage
 * ================================================================== */
PreviewPage::PreviewPage(QObject* parent) : QWebEnginePage(parent) {}

bool PreviewPage::acceptNavigationRequest(const QUrl& url, NavigationType type, bool isMainFrame)
{
    if (type == QWebEnginePage::NavigationTypeLinkClicked) {
        const QString scheme = url.scheme();
        if (scheme == QLatin1String("http") || scheme == QLatin1String("https") ||
            scheme == QLatin1String("mailto")) {
            QDesktopServices::openUrl(url);   // 交给系统浏览器
            return false;
        }
        // 页内锚点（#xxx）等仍由页面自行处理
    }
    return QWebEnginePage::acceptNavigationRequest(url, type, isMainFrame);
}

/* ================================================================== *
 * MarkdownPreview
 * ================================================================== */
MarkdownPreview::MarkdownPreview(QWidget* notepad, const QString& pluginFilePath, QObject* parent)
    : QObject(parent)
    , m_notepad(notepad)
{
    // 当前实现不需要插件自身路径；保留参数以与社区插件签名保持一致
    Q_UNUSED(pluginFilePath);

    loadThemes();
    loadSettings();

    m_timer = new QTimer(this);
    m_timer->setInterval(400);
    connect(m_timer, &QTimer::timeout, this, &MarkdownPreview::pollEditor);
}

MarkdownPreview::~MarkdownPreview()
{
    if (m_timer) {
        m_timer->stop();
    }
    if (m_dock) {
        m_dock->hide();
        m_dock->deleteLater();
    }
}

void MarkdownPreview::ensurePageReady()
{
    if (!m_dock) {
        buildDock();
    }
}

bool MarkdownPreview::isPreviewVisible() const
{
    return m_dock && m_dock->isVisible();
}

void MarkdownPreview::buildDock()
{
    auto* mainWindow = qobject_cast<QMainWindow*>(m_notepad);
    if (!mainWindow) {
        return;
    }

    // Qt5 的 QtWebEngine 要求：在创建 WebEngine 视图之前设置该属性。
    // 对以插件方式加载的场景，这里补设一次（须在任何 WebEngine 视图创建之前生效）。
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts, true);

    m_dock = new QDockWidget(QStringLiteral("Markdown 预览"), mainWindow);
    m_dock->setObjectName(QStringLiteral("nddMarkdownPreviewDock"));
    m_dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    m_dock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable |
                        QDockWidget::DockWidgetClosable);

    // ---- 自定义标题栏 ----
    auto* titleBar = new QWidget(m_dock);
    titleBar->setObjectName(QStringLiteral("nddMdTitleBar"));
    auto* lay = new QHBoxLayout(titleBar);
    lay->setContentsMargins(6, 2, 6, 2);
    lay->setSpacing(4);

    m_titleLabel = new QLabel(QStringLiteral("Markdown 预览"), titleBar);
    lay->addWidget(m_titleLabel);
    lay->addStretch(1);

    auto makeBtn = [&](const QString& text, const QString& tip) -> QToolButton* {
        auto* b = new QToolButton(titleBar);
        b->setText(text);
        b->setToolTip(tip);
        b->setAutoRaise(true);
        b->setCursor(Qt::PointingHandCursor);
        lay->addWidget(b);
        return b;
    };

    m_btnRefresh = makeBtn(QStringLiteral("⟳"), QStringLiteral("刷新"));
    m_btnAuto    = makeBtn(QStringLiteral("自动"), QStringLiteral("自动刷新开关"));
    m_btnExport  = makeBtn(QStringLiteral("导出"), QStringLiteral("导出为 HTML"));
    m_btnClose   = makeBtn(QStringLiteral("✕"), QStringLiteral("关闭面板"));

    m_btnAuto->setCheckable(true);
    m_btnAuto->setChecked(m_autoRefresh);

    m_dock->setTitleBarWidget(titleBar);

    // ---- Web 视图 ----
    m_view = new QWebEngineView(m_dock);
    m_view->setPage(new PreviewPage(m_view));
    m_view->settings()->setAttribute(QWebEngineSettings::JavascriptEnabled, true);
    m_view->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);
    m_view->setZoomFactor(m_zoom);
    m_dock->setWidget(m_view);

    mainWindow->addDockWidget(Qt::RightDockWidgetArea, m_dock);
    m_dock->hide();

    // ---- 信号 ----
    connect(m_view, &QWebEngineView::loadFinished, this, &MarkdownPreview::onLoadFinished);
    connect(m_btnRefresh, &QToolButton::clicked, this, &MarkdownPreview::refreshNow);
    connect(m_btnAuto,    &QToolButton::clicked, this, &MarkdownPreview::toggleAutoRefresh);
    connect(m_btnExport,  &QToolButton::clicked, this, &MarkdownPreview::exportHtml);
    connect(m_btnClose,   &QToolButton::clicked, this, [this] { m_dock->hide(); });

    connect(m_dock, &QDockWidget::visibilityChanged, this, [this](bool visible) {
        if (visible) {
            if (m_autoRefresh) {
                m_timer->start();
            }
            refreshNow();
        } else {
            m_timer->stop();
        }
    });

    // ---- 载入内嵌渲染页 ----
    m_view->load(QUrl(QStringLiteral("qrc:/ndd/preview.html")));
}

void MarkdownPreview::showPreview()
{
    ensurePageReady();
    if (!m_dock) {
        return;
    }
    m_dock->show();
    m_dock->raise();
    refreshNow();
}

void MarkdownPreview::togglePreview()
{
    ensurePageReady();
    if (!m_dock) {
        return;
    }
    if (m_dock->isVisible()) {
        m_dock->hide();
    } else {
        showPreview();
    }
}

void MarkdownPreview::refreshNow()
{
    ensurePageReady();
    if (!m_view) {
        return;
    }
    const QString text = currentText();
    if (!m_pageReady) {
        m_pendingRender = true;
        m_pendingText = text;
        return;
    }
    renderText(text, true);
}

void MarkdownPreview::onLoadFinished(bool ok)
{
    m_pageReady = ok;
    if (!ok) {
        return;
    }

    // 页面就绪后先把主题应用上去，再渲染内容
    applyMarkdownThemeToPage();
    applyMermaidThemeToPage();

    if (m_pendingRender) {
        m_pendingRender = false;
        const QString text = m_pendingText;
        m_pendingText.clear();
        renderText(text, false);
    } else if (m_dock && m_dock->isVisible()) {
        refreshNow();
    }
}

void MarkdownPreview::pollEditor()
{
    if (!m_dock || !m_dock->isVisible() || !m_autoRefresh || !m_pageReady) {
        return;
    }
    const QString text = currentText();
    if (text != m_lastRendered) {
        renderText(text, true);
    }
}

QString MarkdownPreview::currentText() const
{
    m_diag.clear();
    if (!g_getCurEdit) {
        m_diag = QStringLiteral("主程序未注入 getCurEdit 回调（g_getCurEdit 为空）。");
        return QString();
    }
    QsciScintilla* editor = g_getCurEdit(m_notepad);
    if (!editor) {
        // 少数情况下主程序期望传入的是主窗口指针以外的对象，这里做一次兜底尝试
        editor = g_getCurEdit(nullptr);
    }
    if (!editor) {
        m_diag = QStringLiteral(
            "getCurEdit() 返回空指针：未找到当前编辑器。\n"
            "请确认已打开一个文件并使其获得焦点。");
        return QString();
    }
    const QString t = editor->text();
    if (t.isEmpty()) {
        m_diag = QStringLiteral("编辑器指针有效，但 text() 返回空字符串（文档内容为空）。");
    }
    return t;
}

QString MarkdownPreview::currentFileName() const
{
    if (!m_notepad) {
        return QString();
    }
    return m_notepad->windowTitle();
}

void MarkdownPreview::updateTitle()
{
    if (!m_titleLabel) {
        return;
    }
    const QString name = currentFileName();
    if (name.isEmpty()) {
        m_titleLabel->setText(QStringLiteral("Markdown 预览"));
        m_titleLabel->setToolTip(QString());
    } else {
        m_titleLabel->setText(QStringLiteral("Markdown 预览 — %1").arg(name));
        m_titleLabel->setToolTip(name);
    }
}

void MarkdownPreview::renderText(const QString& text, bool preserveScroll)
{
    if (!m_view) {
        return;
    }
    if (!m_pageReady) {
        m_pendingRender = true;
        m_pendingText = text;
        return;
    }

    // 取不到编辑器内容时，直接把原因显示在面板上，避免与"空文档"混淆
    if (text.isEmpty() && !m_diag.isEmpty()) {
        m_lastRendered = text;
        showPanelError(QStringLiteral("未取到编辑器内容"), m_diag);
        updateTitle();
        return;
    }

    m_lastRendered = text;

    const QString json = jsStringLiteral(text);

    // 包一层 try/catch 并把结果回传，便于在渲染失败时给出明确提示
    const QString js = QStringLiteral(
        "(function(){try{"
        "  if(!window.NDDPreview) return 'ERR:NDDPreview 未定义（渲染页未就绪）';"
        "  var t = %1;"
        "  window.NDDPreview.render(t, {preserveScroll:%2});"
        "  return 'OK:' + t.length;"
        "}catch(e){ return 'EXC:' + (e && e.message ? e.message : e); }})()")
        .arg(json, preserveScroll ? QStringLiteral("true") : QStringLiteral("false"));

    m_view->page()->runJavaScript(js, [this](const QVariant& v) {
        const QString status = v.toString();
        if (status.startsWith(QLatin1String("OK:"))) {
            return;   // 正常
        }
        // 渲染失败：把原因显示到面板，避免看起来像"空文档"
        showPanelError(QStringLiteral("渲染失败"), status);
    });

    updateTitle();
}

void MarkdownPreview::showPanelError(const QString& title, const QString& detail)
{
    if (!m_view || !m_pageReady) {
        return;
    }
    QJsonArray a1;
    a1.append(title);
    QString titleJson = QString::fromUtf8(QJsonDocument(a1).toJson(QJsonDocument::Compact));
    QJsonArray a2;
    a2.append(detail);
    QString detailJson = QString::fromUtf8(QJsonDocument(a2).toJson(QJsonDocument::Compact));
    if (titleJson.size() >= 2)  { titleJson.remove(0, 1);  titleJson.chop(1); }
    if (detailJson.size() >= 2) { detailJson.remove(0, 1); detailJson.chop(1); }

    const QString js = QStringLiteral(
        "(function(){"
        "  var c = document.getElementById('content');"
        "  if (!c) return;"
        "  c.innerHTML = '';"
        "  var d = document.createElement('div'); d.className = 'ndd-error';"
        "  var t = document.createElement('div'); t.className = 'ndd-error-title';"
        "  t.textContent = %1;"
        "  var p = document.createElement('pre'); p.textContent = %2;"
        "  d.appendChild(t); d.appendChild(p); c.appendChild(d);"
        "})();").arg(titleJson, detailJson);

    m_view->page()->runJavaScript(js);
}

void MarkdownPreview::loadThemes()
{
    m_mdThemes.clear();
    m_mermaidThemes.clear();

    QFile f(QStringLiteral(":/ndd/themes/themes.json"));
    if (!f.open(QIODevice::ReadOnly)) {
        return;
    }
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    f.close();
    if (!doc.isObject()) {
        return;
    }
    const QJsonObject root = doc.object();

    const QJsonArray mds = root.value(QStringLiteral("markdown")).toArray();
    for (const QJsonValue& v : mds) {
        const QJsonObject o = v.toObject();
        ThemeEntry e;
        e.id    = o.value(QStringLiteral("id")).toString();
        e.label = o.value(QStringLiteral("label")).toString(e.id);
        e.file  = o.value(QStringLiteral("file")).toString();
        e.dark  = o.value(QStringLiteral("dark")).toBool();
        if (!e.id.isEmpty()) {
            m_mdThemes.append(e);
        }
    }

    const QJsonArray mms = root.value(QStringLiteral("mermaid")).toArray();
    for (const QJsonValue& v : mms) {
        const QJsonObject o = v.toObject();
        ThemeEntry e;
        e.id    = o.value(QStringLiteral("id")).toString();
        e.label = o.value(QStringLiteral("label")).toString(e.id);
        if (!e.id.isEmpty()) {
            m_mermaidThemes.append(e);
        }
    }

    if (m_mdThemeId.isEmpty() && !m_mdThemes.isEmpty()) {
        m_mdThemeId = m_mdThemes.first().id;
    }
    if (m_mermaidThemeId.isEmpty() && !m_mermaidThemes.isEmpty()) {
        m_mermaidThemeId = m_mermaidThemes.first().id;
    }
}

void MarkdownPreview::applyMarkdownThemeToPage()
{
    if (!m_view || !m_pageReady) {
        return;
    }
    // 找到当前主题条目
    QString file;
    bool dark = m_dark;
    for (const ThemeEntry& e : m_mdThemes) {
        if (e.id == m_mdThemeId) {
            file = e.file;
            dark = e.dark;
            break;
        }
    }
    m_dark = dark;

    const QString js = QStringLiteral(
        "window.NDDPreview && window.NDDPreview.setMarkdownTheme(%1, %2);")
        .arg(jsStringLiteral(file), dark ? QStringLiteral("true") : QStringLiteral("false"));
    m_view->page()->runJavaScript(js);
}

void MarkdownPreview::applyMermaidThemeToPage()
{
    if (!m_view || !m_pageReady) {
        return;
    }
    const QString js = QStringLiteral(
        "window.NDDPreview && window.NDDPreview.setMermaidTheme(%1);")
        .arg(jsStringLiteral(m_mermaidThemeId));
    m_view->page()->runJavaScript(js);
}

void MarkdownPreview::setMarkdownTheme(const QString& id)
{
    if (m_mdThemeId == id) {
        return;
    }
    m_mdThemeId = id;
    applyMarkdownThemeToPage();
    saveSettings();
    emit markdownThemeChanged(id);
}

void MarkdownPreview::setMermaidTheme(const QString& id)
{
    if (m_mermaidThemeId == id) {
        return;
    }
    m_mermaidThemeId = id;
    applyMermaidThemeToPage();
    saveSettings();
    emit mermaidThemeChanged(id);
}

void MarkdownPreview::setAutoRefresh(bool on)
{
    m_autoRefresh = on;
    if (m_btnAuto) {
        m_btnAuto->setChecked(on);
    }
    saveSettings();
    if (on) {
        if (m_dock && m_dock->isVisible()) {
            m_timer->start();
            refreshNow();
        }
    } else {
        m_timer->stop();
    }
    emit autoRefreshChanged(on);
}

void MarkdownPreview::toggleAutoRefresh()
{
    setAutoRefresh(!m_autoRefresh);
}

void MarkdownPreview::zoomIn()
{
    m_zoom = qMin(m_zoom + 0.1, 3.0);
    if (m_view) {
        m_view->setZoomFactor(m_zoom);
    }
    saveSettings();
}

void MarkdownPreview::zoomOut()
{
    m_zoom = qMax(m_zoom - 0.1, 0.4);
    if (m_view) {
        m_view->setZoomFactor(m_zoom);
    }
    saveSettings();
}

void MarkdownPreview::zoomReset()
{
    m_zoom = 1.0;
    if (m_view) {
        m_view->setZoomFactor(m_zoom);
    }
    saveSettings();
}

QString MarkdownPreview::buildStandaloneHtml(const QString& bodyHtml) const
{
    const QString katexCss = readResource(QStringLiteral(":/ndd/css/katex.min.css"));
    const QString hlCss    = readResource(m_dark ? QStringLiteral(":/ndd/css/highlight-github-dark.min.css")
                                                 : QStringLiteral(":/ndd/css/highlight-github-light.min.css"));
    const QString nddCss   = readResource(QStringLiteral(":/ndd/css/ndd-preview.css"));

    // 导出时使用"当前选中"的 Markdown 主题
    QString mdThemeFile;
    for (const ThemeEntry& e : m_mdThemes) {
        if (e.id == m_mdThemeId) {
            mdThemeFile = e.file;
            break;
        }
    }
    const QString mdCss = readResource(QStringLiteral(":/ndd/") + mdThemeFile);

    // 使用字符串拼接而非 QString::arg 占位符，避免正文中出现 %1 之类的序列被误替换
    QString doc;
    doc += QStringLiteral("<!DOCTYPE html>\n<html lang=\"zh-CN\" data-theme=\"%1\">\n<head>\n")
               .arg(m_dark ? QStringLiteral("dark") : QStringLiteral("light"));
    doc += QStringLiteral("<meta charset=\"utf-8\">\n");
    doc += QStringLiteral("<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n");
    doc += QStringLiteral("<title>%1</title>\n").arg(currentFileName().toHtmlEscaped());
    doc += QStringLiteral("<style>\n");
    doc += katexCss;  doc += QStringLiteral("\n");
    doc += mdCss;     doc += QStringLiteral("\n");
    doc += hlCss;     doc += QStringLiteral("\n");
    doc += nddCss;    doc += QStringLiteral("\n");
    doc += QStringLiteral("</style>\n</head>\n<body>\n");
    doc += QStringLiteral("<div id=\"scroller\">\n<article id=\"content\" class=\"markdown-body\">\n");
    doc += bodyHtml;
    doc += QStringLiteral("\n</article>\n</div>\n</body>\n</html>\n");
    return doc;
}

void MarkdownPreview::exportHtml()
{
    ensurePageReady();
    if (!m_view || !m_pageReady) {
        QMessageBox::information(m_notepad, QStringLiteral("Markdown 预览"),
                                 QStringLiteral("预览尚未就绪，请稍后再试。"));
        return;
    }

    m_view->page()->runJavaScript(
        QStringLiteral("window.NDDPreview ? window.NDDPreview.getHtml() : ''"),
        [this](const QVariant& value) {
            const QString body = value.toString();
            const QString html = buildStandaloneHtml(body);

            QString base = currentFileName();
            if (base.isEmpty()) {
                base = QStringLiteral("preview");
            }
            const QString path = QFileDialog::getSaveFileName(
                m_notepad, QStringLiteral("导出 HTML"), base + QStringLiteral(".html"),
                QStringLiteral("HTML 文件 (*.html)"));
            if (path.isEmpty()) {
                return;
            }

            QFile f(path);
            if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QMessageBox::warning(m_notepad, QStringLiteral("Markdown 预览"),
                                     QStringLiteral("无法写入文件：\n%1").arg(path));
                return;
            }
            f.write(html.toUtf8());
            f.close();

            QMessageBox::information(m_notepad, QStringLiteral("Markdown 预览"),
                                     QStringLiteral("已导出：\n%1").arg(path));
        });
}

void MarkdownPreview::loadSettings()
{
    QSettings s{QLatin1String(kSettingsOrg), QLatin1String(kSettingsApp)};
    m_mdThemeId      = s.value(QStringLiteral("mdTheme"), QStringLiteral("github-light")).toString();
    m_mermaidThemeId = s.value(QStringLiteral("mermaidTheme"), QStringLiteral("default")).toString();
    m_autoRefresh    = s.value(QStringLiteral("autoRefresh"), true).toBool();
    m_zoom           = s.value(QStringLiteral("zoom"), 1.0).toDouble();
    if (m_zoom < 0.4 || m_zoom > 3.0) {
        m_zoom = 1.0;
    }

    // 校验主题是否仍然存在（主题清单可能因版本更新而变化）
    auto ensureValid = [](const QVector<ThemeEntry>& list, QString& id) {
        for (const ThemeEntry& e : list) {
            if (e.id == id) {
                return;
            }
        }
        id = list.isEmpty() ? QString() : list.first().id;
    };
    ensureValid(m_mdThemes, m_mdThemeId);
    ensureValid(m_mermaidThemes, m_mermaidThemeId);

    // 依据当前 Markdown 主题推导明/暗（用于代码高亮与导出）
    for (const ThemeEntry& e : m_mdThemes) {
        if (e.id == m_mdThemeId) {
            m_dark = e.dark;
            break;
        }
    }
}

void MarkdownPreview::saveSettings() const
{
    QSettings s{QLatin1String(kSettingsOrg), QLatin1String(kSettingsApp)};
    s.setValue(QStringLiteral("mdTheme"), m_mdThemeId);
    s.setValue(QStringLiteral("mermaidTheme"), m_mermaidThemeId);
    s.setValue(QStringLiteral("autoRefresh"), m_autoRefresh);
    s.setValue(QStringLiteral("zoom"), m_zoom);
}
