cmake_minimum_required(VERSION 3.16)

project(ndd-markdown-preview LANGUAGES CXX)

# ------------------------------------------------------------------
# 基础设置
# ------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

# ------------------------------------------------------------------
# Qt（需要 WebEngineWidgets 用于承载渲染页）
# ------------------------------------------------------------------
find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets WebEngineWidgets)

# ------------------------------------------------------------------
# Notepad-- 主程序根目录与 QScintilla 依赖
#   默认假设本工程位于 <ndd-root>/plugin/ndd-markdown-preview/
#   可用 -DNDD_ROOT=... 覆盖；也可用环境变量 NDD_ROOT
# ------------------------------------------------------------------
if(NOT DEFINED NDD_ROOT)
    if(DEFINED ENV{NDD_ROOT})
        set(NDD_ROOT "$ENV{NDD_ROOT}")
    else()
        get_filename_component(NDD_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
    endif()
endif()
message(STATUS "NDD_ROOT = ${NDD_ROOT}")

# ------------------------------------------------------------------
# 两种构建模式，自动判定：
#   A) 源码模式：NDD_ROOT 下存在 qscint\ → 使用真实的 QScintilla 头文件
#   B) 导入库模式：只有已安装的 Notepad--（无源码）→ 使用
#      src/qsciscintilla_ndd.h 里的最小声明 + 由 tools/gen-qmyedit-implib.ps1
#      从 qmyedit_qt5.dll 生成的导入库
# ------------------------------------------------------------------
if(EXISTS "${NDD_ROOT}/qscint/src")
    set(NDD_HAVE_QSCINT_SOURCE TRUE)
    message(STATUS "检测到 NDD 源码树，使用真实 QScintilla 头文件")
else()
    set(NDD_HAVE_QSCINT_SOURCE FALSE)
    message(STATUS "未检测到 NDD 源码树，使用最小 QScintilla 声明 + 导入库模式")
endif()

# QScintilla 头文件目录（仅源码模式需要）
set(QSCINT_INCLUDE_DIR "${NDD_ROOT}/qscint/src" CACHE PATH "QScintilla src 目录（源码模式）")

# qmyedit_qt5 导入库：可直接用 -DQSCINT_LIB=<xxx.lib> 指定完整路径
if(NOT DEFINED QSCINT_LIB)
    find_library(QSCINT_LIB
        NAMES qmyedit_qt5 qmyedit_qt5d
        PATHS "${NDD_ROOT}/x64/Release" "${NDD_ROOT}/x64/Debug"
              "${CMAKE_CURRENT_SOURCE_DIR}/libs"
        NO_DEFAULT_PATH
    )
endif()
if(NOT QSCINT_LIB)
    message(WARNING
        "未找到 qmyedit_qt5 导入库。\n"
        "  - 有 NDD 源码：用 -DQSCINT_LIB_DIR=<ndd>/x64/Release 指定\n"
        "  - 仅装了 Notepad--：先运行 tools\\gen-qmyedit-implib.bat 生成\n"
        "    libs/qmyedit_qt5.lib，再用 -DQSCINT_LIB=<路径> 指定")
else()
    message(STATUS "qmyedit_qt5 导入库: ${QSCINT_LIB}")
endif()

# 插件输出目录（安装目标）
#   源码模式   -> <NDD_ROOT>/x64/Release/plugin
#   导入库模式 -> 需由 -DNDD_PLUGIN_DIR=<路径> 或环境变量 NDD_PLUGIN_DIR 指定
# 未指定时不做安装拷贝，只产出 build/Release/ndd-markdown-preview.dll
if(NOT DEFINED NDD_PLUGIN_DIR)
    if(NDD_HAVE_QSCINT_SOURCE)
        set(NDD_PLUGIN_DIR "${NDD_ROOT}/x64/Release/plugin" CACHE PATH "插件输出目录")
    elseif(DEFINED ENV{NDD_PLUGIN_DIR})
        set(NDD_PLUGIN_DIR "$ENV{NDD_PLUGIN_DIR}" CACHE PATH "插件输出目录")
    else()
        set(NDD_PLUGIN_DIR "" CACHE PATH "插件输出目录（留空则不自动安装）")
    endif()
endif()
if(NDD_PLUGIN_DIR)
    message(STATUS "插件输出目录: ${NDD_PLUGIN_DIR}")
else()
    message(STATUS "插件输出目录: 未指定（仅构建，不自动安装）")
endif()

# ------------------------------------------------------------------
# 目标
# ------------------------------------------------------------------
set(PLUGIN_SOURCES
    src/dllmain.cpp
    src/instanceobj.cpp
    src/mdpreview.cpp
)

set(PLUGIN_HEADERS
    src/pluginGl.h
    src/pluginctx.h
    src/version.h
    src/qsciscintilla_ndd.h
    src/instanceobj.h
    src/mdpreview.h
)

add_library(${PROJECT_NAME} SHARED
    ${PLUGIN_SOURCES}
    ${PLUGIN_HEADERS}
    resources/resources.qrc
)

target_include_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${QSCINT_INCLUDE_DIR}"
    "${QSCINT_INCLUDE_DIR}/Qsci"
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt5::Core Qt5::Gui Qt5::Widgets Qt5::WebEngineWidgets
)

if(QSCINT_LIB)
    target_link_libraries(${PROJECT_NAME} PRIVATE "${QSCINT_LIB}")
endif()

# MSVC：把源码字符集与执行字符集都固定为 UTF-8。
# 源码含中文注释与中文字符串字面量（如 QStringLiteral("预览 Markdown")），
# 若不指定，MSVC 会按系统代码页（简中为 GBK/936）解析，导致 C2001/C4819 等错误。
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /utf-8)
endif()

# 输出名：Windows 上不要 lib 前缀，便于 NDD 直接加载
set_target_properties(${PROJECT_NAME} PROPERTIES
    PREFIX ""
    OUTPUT_NAME "ndd-markdown-preview"
)

# 构建后自动拷贝到 NDD 插件目录
if(NDD_PLUGIN_DIR AND EXISTS "${NDD_PLUGIN_DIR}")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:${PROJECT_NAME}>" "${NDD_PLUGIN_DIR}"
        COMMENT "拷贝插件到 ${NDD_PLUGIN_DIR}")
endif()

# ------------------------------------------------------------------
# 辅助目标：重建前端渲染资源（需 Node.js）
# ------------------------------------------------------------------
find_program(NODE_EXECUTABLE node)
if(NODE_EXECUTABLE)
    add_custom_target(renderer
        COMMAND ${NODE_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/renderer/build.mjs"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/renderer"
        COMMENT "重建前端渲染资源 (preview.bundle.js 等)"
        USES_TERMINAL)
endif()
