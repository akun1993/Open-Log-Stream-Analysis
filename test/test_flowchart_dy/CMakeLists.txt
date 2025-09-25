cmake_minimum_required(VERSION 3.5)

# ================== 基础项目设置 =================
set(PROJECT_NAME flowchart_dy)
project(${PROJECT_NAME})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# ================== 配置文件处理 =================
set(CONFIG_FILE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/config/config.ini")
add_compile_definitions(CONFIG_FILE_PATH="${CONFIG_FILE_PATH}")

# ================== 程序打包开关 =================
option(USE_PACKAGE "Enable packaging" OFF)
if(USE_PACKAGE)
  add_compile_definitions(USE_PACKAGE_ENABLE)
endif()

# ================== 第三方配置 ===================
#include(grpc.cmake)

# ================== Qt配置 ======================
find_package(Qt5 REQUIRED COMPONENTS
    Core
    Widgets
)

# ================== 源文件收集 ==================
set(UI_DIRS
    src
)

set(HEADER_DIRS
    ${UI_DIRS}
)

set(SOURCE_DIRS
    ${UI_DIRS}
)

# .ui
foreach(dir ${UI_DIRS})
    file(GLOB ui_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${dir}/*.ui)
    list(APPEND QT_FORMS ${ui_files})
endforeach()

# .qrc
file(GLOB QT_RESOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.qrc)

# .cpp
set(QT_SOURCES main.cpp)
foreach(dir ${SOURCE_DIRS})
    file(GLOB_RECURSE dir_sources 
        RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} 
        FOLLOW_SYMLINKS
        ${dir}/*.cpp
        ${dir}/*.cc
    )
    list(APPEND QT_SOURCES ${dir_sources})
endforeach()


# ================== 创建可执行文件 ==================
add_executable(${PROJECT_NAME}
    ${QT_SOURCES}
    ${QT_RESOURCES}
)

# ================== 目标链接 =====================
target_link_libraries(${PROJECT_NAME}
    Qt5::Core
    Qt5::Widgets
)

# ================== 包含目录设置 ==================
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${HEADER_DIRS}
)

# ================== 安装配置 ====================
install(TARGETS ${PROJECT_NAME} 
    RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
)