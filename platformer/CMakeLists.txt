add_executable(${PROJECT_NAME} main.cpp)

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/engine)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
target_link_libraries(${PROJECT_NAME} PRIVATE ECS)
enable_warnings(${PROJECT_NAME})

# The lab build links the bundled SFML .so files by absolute path. Without a runtime path the
# executable cannot find them when it is run from build/, so bake the bundle's lib dir in.
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND DEFINED SFML_ROOT)
    set_target_properties(${PROJECT_NAME} PROPERTIES BUILD_RPATH "${SFML_ROOT}/lib")
endif()
