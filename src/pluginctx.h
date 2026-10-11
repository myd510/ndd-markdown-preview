#pragma once
/*
 * 插件运行期共享上下文。
 * 主程序在 NDD_PROC_MAIN 中注入的函数回调，统一放在这里供各模块取用。
 */
#include <functional>

class QWidget;
class QsciScintilla;

// 由主程序注入：通过主窗口指针获取“当前正在编辑的” QsciScintilla 对象
extern std::function<QsciScintilla*(QWidget*)> g_getCurEdit;

// 由主程序注入：回调主程序内部功能（本项目暂未使用，保留以备扩展）
extern std::function<bool(QWidget*, int, void*)> g_pluginCallBack;
