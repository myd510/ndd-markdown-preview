#pragma once
/*
 * QScintilla 头文件适配层
 * ------------------------------------------------------------
 * 插件只用到 QsciScintilla 的一个接口：QString text() const。
 *
 * 两条路径自动选择：
 *   1) 若 include 路径上能找到 ndd 源码里的 <qsciscintilla.h>，直接用它（首选）；
 *   2) 否则使用下面的最小声明 —— 它只声明插件真正调用的成员，
 *      链接时由从 qmyedit_qt5.dll 生成的导入库提供符号
 *      （见 tools/gen-qmyedit-implib.ps1）。
 *
 * 为什么最小声明是安全的：
 *   - QsciScintilla::text() 是**非虚**成员函数，DLL 已导出
 *     ?text@QsciScintilla@@QEBA?AVQString@@XZ；
 *   - 插件从不构造 / 析构 QsciScintilla，也不调用其任何虚函数，
 *     因此不依赖该类的 vtable 布局；
 *   - 指针由主程序（ndd）通过 getCurEdit 回调传入。
 */
#if defined(__has_include)
#  if __has_include(<qsciscintilla.h>)
#    include <qsciscintilla.h>
#    define NDD_QSCINTILLA_FROM_SOURCE 1
#  endif
#endif

#ifndef NDD_QSCINTILLA_FROM_SOURCE

#include <QString>

class QWidget;

#if defined(_MSC_VER) || defined(__MINGW32__)
#  define NDD_QSCINT_IMPORT __declspec(dllimport)
#else
#  define NDD_QSCINT_IMPORT
#endif

class NDD_QSCINT_IMPORT QsciScintilla
{
public:
    QString text() const;
};

#endif  // NDD_QSCINTILLA_FROM_SOURCE
