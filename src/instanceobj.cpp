#include "instanceobj.h"
#include "mdpreview.h"

#include <QAction>
#include <QActionGroup>
#include <QKeySequence>
#include <QMenu>
#include <QSignalBlocker>

InstanceObj::InstanceObj(QWidget* notepad, QMenu* rootMenu, const QString& pluginFilePath, QObject* parent)
    : QObject(parent)
    , m_notepad(notepad)
    , m_rootMenu(rootMenu)
    , m_pluginFilePath(pluginFilePath)
{
}

InstanceObj::~InstanceObj() = default;

void InstanceObj::ensurePreview()
{
    if (!m_preview) {
        m_preview = new MarkdownPreview(m_notepad, m_pluginFilePath, this);
    }
}

void InstanceObj::onMenuAboutToShow()
{
    if (m_menuInited || !m_rootMenu) {
        return;
    }
    m_menuInited = true;

    ensurePreview();

    // 主功能
    m_rootMenu->addAction(QStringLiteral("预览 Markdown"), m_preview,
                          &MarkdownPreview::togglePreview, QKeySequence(QStringLiteral("Ctrl+Alt+P")));
    m_rootMenu->addAction(QStringLiteral("刷新预览"), m_preview,
                          &MarkdownPreview::refreshNow, QKeySequence(Qt::Key_F5));

    m_rootMenu->addSeparator();

    // 自动刷新（可勾选）
    // 注意：QAction::setChecked() 会发出 toggled，若直接连到 toggleXxx 会被反向触发一次，
    // 因此这里连"设置型"槽，并通过信号把状态同步回菜单项。
    QAction* autoAct = m_rootMenu->addAction(QStringLiteral("自动刷新"));
    autoAct->setCheckable(true);
    autoAct->setChecked(m_preview->isAutoRefresh());
    QObject::connect(autoAct, &QAction::toggled, m_preview, &MarkdownPreview::setAutoRefresh);
    QObject::connect(m_preview, &MarkdownPreview::autoRefreshChanged, autoAct,
                     [autoAct](bool on) {
                         const QSignalBlocker blocker(autoAct);
                         autoAct->setChecked(on);
                     });

    // ---- Markdown 主题子菜单 ----
    QMenu* mdMenu = m_rootMenu->addMenu(QStringLiteral("Markdown 主题"));
    auto* mdGroup = new QActionGroup(mdMenu);
    mdGroup->setExclusive(true);
    for (const ThemeEntry& e : m_preview->markdownThemes()) {
        QAction* a = mdMenu->addAction(e.label);
        a->setCheckable(true);
        a->setChecked(e.id == m_preview->currentMarkdownTheme());
        a->setData(e.id);
        mdGroup->addAction(a);
        const QString id = e.id;
        QObject::connect(a, &QAction::triggered, m_preview,
                         [this, id]() { m_preview->setMarkdownTheme(id); });
    }
    QObject::connect(m_preview, &MarkdownPreview::markdownThemeChanged, mdMenu,
                     [mdGroup](const QString& id) {
                         for (QAction* a : mdGroup->actions()) {
                             if (a->data().toString() == id) {
                                 const QSignalBlocker blocker(a);
                                 a->setChecked(true);
                             }
                         }
                     });

    // ---- Mermaid 主题子菜单 ----
    QMenu* mmMenu = m_rootMenu->addMenu(QStringLiteral("Mermaid 主题"));
    auto* mmGroup = new QActionGroup(mmMenu);
    mmGroup->setExclusive(true);
    for (const ThemeEntry& e : m_preview->mermaidThemes()) {
        QAction* a = mmMenu->addAction(e.label);
        a->setCheckable(true);
        a->setChecked(e.id == m_preview->currentMermaidTheme());
        a->setData(e.id);
        mmGroup->addAction(a);
        const QString id = e.id;
        QObject::connect(a, &QAction::triggered, m_preview,
                         [this, id]() { m_preview->setMermaidTheme(id); });
    }
    QObject::connect(m_preview, &MarkdownPreview::mermaidThemeChanged, mmMenu,
                     [mmGroup](const QString& id) {
                         for (QAction* a : mmGroup->actions()) {
                             if (a->data().toString() == id) {
                                 const QSignalBlocker blocker(a);
                                 a->setChecked(true);
                             }
                         }
                     });

    m_rootMenu->addSeparator();

    m_rootMenu->addAction(QStringLiteral("放大"), m_preview, &MarkdownPreview::zoomIn,
                          QKeySequence(QStringLiteral("Ctrl+=")));
    m_rootMenu->addAction(QStringLiteral("缩小"), m_preview, &MarkdownPreview::zoomOut,
                          QKeySequence(QStringLiteral("Ctrl+-")));
    m_rootMenu->addAction(QStringLiteral("重置缩放"), m_preview, &MarkdownPreview::zoomReset);

    m_rootMenu->addSeparator();

    m_rootMenu->addAction(QStringLiteral("导出为 HTML..."), m_preview, &MarkdownPreview::exportHtml);
}
