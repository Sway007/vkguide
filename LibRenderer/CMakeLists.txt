find_package(Vulkan REQUIRED)

set(VULKAN_SDK_DIR "~/VulkanSDK/1.4.328.1/macOS/include")

file(GLOB_RECURSE LIB_SOURCES ${CMAKE_CURRENT_LIST_DIR}/src/*.cpp)
set(LIB_INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR}/include PARENT_SCOPE)

add_library(${LIB_NAME} ${LIB_SOURCES})

target_include_directories(${LIB_NAME} PUBLIC ${THIRD_PARTY_DIR} ${Vulkan_INCLUDE_DIR} ${VULKAN_SDK_DIR} ${THIRD_PARTY_DIR})
target_link_libraries(${LIB_NAME} Vulkan::Vulkan)
