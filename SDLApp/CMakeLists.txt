find_package(SDL3 REQUIRED)

file(GLOB_RECURSE APP_SOURCES ${CMAKE_CURRENT_LIST_DIR}/*.cpp)
add_executable(${APP_NAME} ${APP_SOURCES})

target_link_libraries(${APP_NAME} PRIVATE ${LIB_NAME} SDL3::SDL3)
target_include_directories(${APP_NAME} PRIVATE ${LIB_INCLUDE_DIR} ${SDL3_INCLUDE_DIRS})
