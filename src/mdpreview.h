#pragma once
/*
 * MarkdownPreview —— 插件核心
 * ------------------------------------------------------------
 * 在 Notepad-- 主窗口右侧挂载一个停靠面板，内嵌 QWebEngineView，
 * 把当前编辑器的文本实时渲染为 Markdown / Mermaid / LaTeX。
 *
 * 渲染内核（markdown-it 全套扩展 + KaTeX + Mermaid + highlight.js）
 * 以 Qt 资源（qrc）形式内嵌，离线可用，见 resources/preview.html。
 */
#include <QObject>
#include <QString>
#include <QVector>
#include <QWebEnginePage>

class QWidget;
class QDockWidget;
class QTimer;
class QLabel;
class QToolButton;
class QWebEngineView;
class QsciScintilla;

/* 主题条目（来自 resources/themes/themes.json） */
struct ThemeEntry
{
    QString id;
    QString label;
    QString file;    // 相对 preview.html 的 CSS 路径（Markdown 主题才有）
    bool    dark = false;
};

/* 自定义 Web 页面：把点击的超链接交给系统浏览器打开，避免在面板内跳走 */
class PreviewPage : public QWebEnginePage
{
    Q_OBJECT
public:
    explicit PreviewPage(QObject* parent = nullptr);

protected:
    bool acceptNavigationRequest(const QUrl& url, NavigationType type, bool isMainFrame) override;
};

class MarkdownPreview : public QObject
{
    Q_OBJECT
public:
    explicit MarkdownPreview(QWidget* notepad, const QString& pluginFilePath, QObject* parent = nullptr);
    ~MarkdownPreview() override;

    bool isDarkTheme() const { return m_dark; }
    bool isAutoRefresh() const { return m_autoRefresh; }
    bool isPreviewVisible() const;

    // 主题
    const QVector<ThemeEntry>& markdownThemes() const { return m_mdThemes; }
    const QVector<ThemeEntry>& mermaidThemes() const { return m_mermaidThemes; }
    QString currentMarkdownTheme() const { return m_mdThemeId; }
    QString currentMermaidTheme() const { return m_mermaidThemeId; }

public slots:
    void togglePreview();       // 显示/隐藏预览面板
    void showPreview();         // 显示并立即渲染
    void refreshNow();          // 立即重新渲染
    void exportHtml();          // 导出为独立 HTML 文件
    void setMarkdownTheme(const QString& id);   // 切换 Markdown 主题（CSS）
    void setMermaidTheme(const QString& id);    // 切换 Mermaid 主题（调色板）
    void toggleAutoRefresh();   // 切换 自动刷新
    void setAutoRefresh(bool on);            // 设置自动刷新（供菜单项同步，避免被 setChecked 反向触发）
    void zoomIn();              // 放大
    void zoomOut();             // 缩小
    void zoomReset();           // 重置缩放

signals:
    void autoRefreshChanged(bool on);
    void markdownThemeChanged(const QString& id);
    void mermaidThemeChanged(const QString& id);

private slots:
    void onLoadFinished(bool ok);
    void pollEditor();          // 定时轮询当前编辑器文本变化

private:
    void buildDock();
    void loadSettings();
    void saveSettings() const;
    void loadThemes();
    void applyMarkdownThemeToPage();
    void applyMermaidThemeToPage();
    void ensurePageReady();
    QString currentText() const;
    QString currentFileName() const;
    void renderText(const QString& text, bool preserveScroll);
    void showPanelError(const QString& title, const QString& detail);
    void updateTitle();
    QString buildStandaloneHtml(const QString& bodyHtml) const;

private:
    QWidget*        m_notepad = nullptr;

    QDockWidget*    m_dock = nullptr;
    QWebEngineView* m_view = nullptr;
    QTimer*         m_timer = nullptr;

    QLabel*         m_titleLabel = nullptr;
    QToolButton*    m_btnRefresh = nullptr;
    QToolButton*    m_btnAuto = nullptr;
    QToolButton*    m_btnExport = nullptr;
    QToolButton*    m_btnClose = nullptr;

    bool            m_dark = true;
    bool            m_autoRefresh = true;
    bool            m_pageReady = false;
    bool            m_pendingRender = false;
    QString         m_pendingText;
    QString         m_lastRendered;
    mutable QString m_diag;         // 最近一次取文本失败的原因（用于面板提示）
    double          m_zoom = 1.0;

    // 主题
    QVector<ThemeEntry> m_mdThemes;
    QVector<ThemeEntry> m_mermaidThemes;
    QString             m_mdThemeId;
    QString             m_mermaidThemeId;
};
