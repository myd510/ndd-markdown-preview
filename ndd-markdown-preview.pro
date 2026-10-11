#--------------------------------------------------------------------
# Notepad-- Markdown 预览插件 —— qmake 工程（备用构建方式）
#
# 用法（导入库模式，无 ndd 源码）：
#   qmake QSCINT_LIB=<仓库>/libs/qmyedit_qt5.lib \
#         NDD_PLUGIN_DIR=<Notepad-- 插件目录>
#   nmake            # 或 jom / make
#
# 用法（源码模式，有 ndd 源码树）：
#   qmake NDD_ROOT=<ndd 源码根目录>
#
# 说明：qmake 的 QT 变量只认 kit 内的 Qt，请使用与 Notepad-- 一致的 Qt 版本
#       （推荐 Qt 5.15.2 msvc2019_64）。未指定 NDD_PLUGIN_DIR 时只构建、不安装。
#--------------------------------------------------------------------
TEMPLATE = lib
LANGUAGE = C++
CONFIG  += qt warn_on release c++17
QT      += core gui widgets webenginewidgets

# MSVC：源码/执行字符集固定为 UTF-8（源码含中文注释与中文字符串字面量）
msvc: QMAKE_CXXFLAGS += /utf-8

# ---- 路径（默认值均相对本工程，可用 qmake 变量覆盖）----
isEmpty(NDD_ROOT)   { NDD_ROOT   = $$clean_path($$PWD/../..) }
isEmpty(QSCINT_LIB) { QSCINT_LIB = $$PWD/libs/qmyedit_qt5.lib }

INCLUDEPATH += $$PWD/src

# 源码模式下追加真实的 QScintilla 头文件目录
exists($$NDD_ROOT/qscint/src) {
    INCLUDEPATH += $$NDD_ROOT/qscint/src \
                   $$NDD_ROOT/qscint/src/Qsci \
                   $$NDD_ROOT/qscint/scintilla/include
}

HEADERS += \
    src/pluginGl.h \
    src/pluginctx.h \
    src/version.h \
    src/qsciscintilla_ndd.h \
    src/instanceobj.h \
    src/mdpreview.h

SOURCES += \
    src/dllmain.cpp \
    src/instanceobj.cpp \
    src/mdpreview.cpp

RESOURCES += resources/resources.qrc

# 输出名不带 lib 前缀，便于 Notepad-- 直接加载
win32: QMAKE_TARGET_PRODUCT = ndd-markdown-preview

win32 {
    CONFIG(debug, debug|release) {
        DESTDIR = $$NDD_ROOT/x64/Debug/plugin
        LIBS   += -L$$NDD_ROOT/x64/Debug   -lqmyedit_qt5d
    } else {
        !isEmpty(NDD_PLUGIN_DIR) { DESTDIR = $$NDD_PLUGIN_DIR }
        # 优先用显式指定的导入库；否则按源码树目录查找
        exists($$QSCINT_LIB) {
            LIBS += $$QSCINT_LIB
        } else {
            LIBS += -L$$NDD_ROOT/x64/Release -lqmyedit_qt5
        }
    }
}

unix {
    !isEmpty(NDD_PLUGIN_DIR) { DESTDIR = $$NDD_PLUGIN_DIR }
    LIBS   += -L$$NDD_ROOT/x64/Release -lqmyedit_qt5
    UI_DIR = .ui
    MOC_DIR = .moc
    OBJECTS_DIR = .obj
}
