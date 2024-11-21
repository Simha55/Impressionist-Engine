#CMakeLists in Root
cmake_minimum_required(VERSION 3.5)
project(ImpressionEngine)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/modules/" ${CMAKE_MODULE_PATH})
set(CMAKE_ASSET_PATH "${PROJECT_SOURCE_DIR}/assets/" ${CMAKE_ASSET_PATH})
set(CMAKE_ASSET_BUILD_PATH "${PROJECT_SOURCE_DIR}/build/src/assets/" ${CMAKE_ASSET_BUILD_PATH})

# Add Corrade as a subproject
add_subdirectory(corrade EXCLUDE_FROM_ALL)

# Add Magnum as a subproject, enable Sdl2Application
set(CMAKE_PREFIX_PATH ${PROJECT_SOURCE_DIR}/SDL2-2.0.10 ${CMAKE_PREFIX_PATH})
set(MAGNUM_WITH_SDL2APPLICATION ON CACHE BOOL "" FORCE)

# If you want to load just basic OBJs
set(MAGNUM_WITH_ANYIMAGEIMPORTER ON CACHE BOOL "" FORCE)
set(MAGNUM_WITH_ANYSCENEIMPORTER ON CACHE BOOL "" FORCE)
set(MAGNUM_WITH_OBJIMPORTER ON CACHE BOOL "" FORCE)

add_subdirectory(magnum EXCLUDE_FROM_ALL)
set(MAGNUM_WITH_STBIMAGEIMPORTER ON CACHE BOOL "" FORCE)
# If you want to load (textured) glTF as well

set(MAGNUM_WITH_GLTFIMPORTER ON CACHE BOOL "" FORCE)
# git clone https://github.com/mosra/magnum-plugins.git
add_subdirectory(magnum-plugins EXCLUDE_FROM_ALL)


add_subdirectory(src)


