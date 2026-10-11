/*
 * Notepad-- 插件入口
 * ------------------------------------------------------------
 * 导出两个 C 符号供主程序查找调用：
 *   NDD_PROC_IDENTIFY —— 上报插件元信息
 *   NDD_PROC_MAIN     —— 插件主入口，注入主窗口与编辑器回调
 */
#include "pluginGl.h"
#include "pluginctx.h"
#include "instanceobj.h"
#include "version.h"

#include <QMenu>
#include <QCoreApplication>
#include <QObject>
#include <QString>
#include <QWidget>
#include <functional>

#include "qsciscintilla_ndd.h"

#define NDD_EXPORTDLL

#if defined(Q_OS_WIN)
    #if defined(NDD_EXPORTDLL)
        #define NDD_EXPORT __declspec(dllexport)
    #else
        #define NDD_EXPORT __declspec(dllimport)
    #endif
#else
    #define NDD_EXPORT __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

NDD_EXPORT bool NDD_PROC_IDENTIFY(NDD_PROC_DATA* pProcData);
NDD_EXPORT int NDD_PROC_MAIN(QWidget* pNotepad, const QString& strFileName,
                             std::function<QsciScintilla*(QWidget*)> getCurEdit,
                             std::function<bool(QWidget*, int, void*)> pluginCallBack,
                             NDD_PROC_DATA* procData);

#ifdef __cplusplus
}
#endif

/* ---- 共享上下文定义（在 pluginctx.h 中声明为 extern） ---- */
std::function<QsciScintilla*(QWidget*)> g_getCurEdit;
std::function<bool(QWidget*, int, void*)> g_pluginCallBack;

bool NDD_PROC_IDENTIFY(NDD_PROC_DATA* pProcData)
{
    if (pProcData == nullptr) {
        return false;
    }

    pProcData->m_strPlugName = QObject::tr(NDD_MD_PLUGIN_NAME);
    pProcData->m_strComment  = QObject::tr(NDD_MD_PLUGIN_COMMENT);
    pProcData->m_version     = QStringLiteral(NDD_MD_PLUGIN_VERSION);
    pProcData->m_auther      = QStringLiteral(NDD_MD_PLUGIN_AUTHOR);
    pProcData->m_menuType    = 1;   // 使用二级菜单

    return true;
}

int NDD_PROC_MAIN(QWidget* pNotepad, const QString& strFileName,
                  std::function<QsciScintilla*(QWidget*)> getCurEdit,
                  std::function<bool(QWidget*, int, void*)> pluginCallBack,
                  NDD_PROC_DATA* procData)
{
    if (procData == nullptr || procData->m_rootMenu == nullptr) {
        return -1;
    }

    g_getCurEdit     = getCurEdit;
    g_pluginCallBack = pluginCallBack;

    // Qt5 的 QtWebEngine 要求该属性在 WebEngine 初始化前设置好，
    // 否则会 qFatal 直接终止进程。这里在插件加载阶段（最早的可用时机）
    // 就设一次，mdpreview 里创建视图前还会再设一次作为兜底。
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts, true);

    // 以主窗口为父对象，主程序退出时自动析构
    auto* instance = new InstanceObj(pNotepad, procData->m_rootMenu, strFileName, pNotepad);
    instance->setObjectName(QStringLiteral("nddMarkdownPreviewPlugin"));

    // 菜单项延迟到首次展开时创建，降低插件加载耗时
    QObject::connect(procData->m_rootMenu, &QMenu::aboutToShow, instance,
                     &InstanceObj::onMenuAboutToShow);

    return 0;
}
