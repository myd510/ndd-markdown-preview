#pragma once
/*
 * InstanceObj —— 插件实例，承载菜单项与预览面板的生命周期。
 * 由 NDD_PROC_MAIN 创建；父对象设为主窗口，主程序释放时自动析构。
 */
#include <QObject>
#include <QString>

class QWidget;
class QMenu;
class MarkdownPreview;

class InstanceObj : public QObject
{
    Q_OBJECT
public:
    InstanceObj(QWidget* notepad, QMenu* rootMenu, const QString& pluginFilePath, QObject* parent = nullptr);
    ~InstanceObj() override;

public slots:
    // 延迟创建菜单项，加快插件加载速度。
    // 注意：需为 public —— 主程序入口 dllmain.cpp 会取它的成员函数指针做 connect。
    void onMenuAboutToShow();

private:
    void ensurePreview();

private:
    QWidget*         m_notepad = nullptr;
    QMenu*           m_rootMenu = nullptr;
    QString          m_pluginFilePath;
    MarkdownPreview* m_preview = nullptr;
    bool             m_menuInited = false;

    InstanceObj(const InstanceObj&) = delete;
    InstanceObj& operator=(const InstanceObj&) = delete;
};
