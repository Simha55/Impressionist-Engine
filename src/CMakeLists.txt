#CMakeLists in src
find_package(Corrade REQUIRED Main)
find_package(Magnum REQUIRED 
    GL 
    MeshTools
    Shaders 
    SceneGraph
    Trade
    Sdl2Application)
set_directory_properties(PROPERTIES CORRADE_USE_PEDANTIC_FLAGS ON)
add_executable(ImpressionEngine ImpressionEngine.cpp
${texturequad_RESOURCES})

add_dependencies(ImpressionEngine 
Magnum::AnyImageImporter
Magnum::AnySceneImporter
Magnum::ObjImporter
MagnumPlugins::StbImageImporter
MagnumPlugins::GltfImporter)

target_link_libraries(ImpressionEngine PRIVATE
    Corrade::Main
    Magnum::Application
    Magnum::GL
    Magnum::Magnum
    Magnum::MeshTools
    Magnum::SceneGraph
    Magnum::Shaders
    Magnum::Trade)

install(TARGETS ImpressionEngine DESTINATION ${MAGNUM_BINARY_INSTALL_DIR})

message(STATUS "Project Asset Directory: ${PROJECT_SOURCE_DIR}/assets/scene.glb")
message(STATUS "Project Destination Directory: ${CMAKE_ASSET_BUILD_PATH}")

install(FILES  ${PROJECT_SOURCE_DIR}/assets/scene.glb DESTINATION  ${CMAKE_ASSET_BUILD_PATH})

# Make the executable a default target to build & run in Visual Studio
set_property(DIRECTORY ${PROJECT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ImpressionEngine)