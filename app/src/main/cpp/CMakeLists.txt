
# ------------------------------------------------------------------------------
# Sets the minimum CMake version required for this project.
cmake_minimum_required(VERSION 3.22.1)

# ------------------------------------------------------------------------------
# Secure kernel project
project(sst_native)

# ------------------------------------------------------------------------------
# Shared storage sources dir
set(SST_HOME_DIR ${CMAKE_CURRENT_LIST_DIR})
set(SST_SRC_DIR ${SK_HOME_DIR})
set(SST_INC_DIR ${SK_SRC_DIR})

# ------------------------------------------------------------------------------
# Source list
set(SST_SRC
        ${SST_HOME_DIR}/sst_main.cpp
        )

# ------------------------------------------------------------------------------
# SQLite sources dir
set(SQLITE_SRC_DIR ${SST_HOME_DIR}/sqlite-amalgamation-3410200)
set(SQLITE_INC_DIR ${SQLITE_SRC_DIR})

# Source list from dir
set(SQLITE_SRC ${SQLITE_SRC_DIR}/sqlite3.c)

# ------------------------------------------------------------------------------
# Configure Shared store project

# Add shared library
add_library(${PROJECT_NAME} SHARED ${SST_SRC} ${SQLITE_SRC})

# Add private include directories
target_include_directories(${PROJECT_NAME} PRIVATE
        ${SST_INC_DIR}
        ${SQLITE_INC_DIR})

# Link platform and third party libraries
target_link_libraries(${PROJECT_NAME}
        android
        log
        stdc++
        ${LIBS})

# Export only the required symbols
target_link_options(${PROJECT_NAME} PRIVATE "-Wl,--version-script=${PROJECT_SOURCE_DIR}/export.txt")
