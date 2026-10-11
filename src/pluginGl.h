#pragma once
/*
 * Notepad-- 插件接口头文件
 * ------------------------------------------------------------
 * 与 Notepad-- 主程序 src/plugin/pluginGl.h 保持一致的 ABI：
 * 插件以动态库形式被主程序 dlopen/LoadLibrary 后，主程序会查找并调用
 *   - NDD_PROC_IDENTIFY : 返回插件名称/版本/作者/菜单类型
 *   - NDD_PROC_MAIN     : 插件入口，注入主窗口指针与编辑器访问回调
 *
 * 注意：请勿修改结构体字段顺序与类型，否则与主程序 ABI 不兼容。
 */
#include <QString>
#include <QMenu>

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

struct ndd_proc_data
{
    QString m_strPlugName;   // 插件名称 必选
    QString m_strFilePath;   // lib 插件的全局路径。必选。插件内部不用管，主程序传递下来
    QString m_strComment;    // 插件说明
    QString m_version;       // 版本号码。可选
    QString m_auther;        // 作者名称。可选
    int     m_menuType;      // 菜单类型。0：不使用二级菜单 1：创建二级菜单
    QMenu*  m_rootMenu;      // 如果 m_menuType = 1，给出二级根菜单的地址。其他值 nullptr

    ndd_proc_data() : m_menuType(0), m_rootMenu(nullptr) {}
};

typedef struct ndd_proc_data NDD_PROC_DATA;

typedef bool (*NDD_PROC_IDENTIFY_CALLBACK)(NDD_PROC_DATA* pProcData);
typedef void (*NDD_PROC_FOUND_CALLBACK)(NDD_PROC_DATA* pProcData, void* pUserData);
