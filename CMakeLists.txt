cmake_minimum_required(VERSION 2.8)

project(metaballs)

set(cpps src/main.cpp src/shader.cpp src/scene.cpp src/metaball.cpp)
set(headers src/shader.h src/common.h src/scene.h src/metaball.h)
set(shaders shaders/metaball.fp shaders/metaball.vp shaders/metaball.geom shaders/cubemap.vp shaders/cubemap.fp)

IF (WIN32)
   set(EXTERNAL_LIBS ${PROJECT_SOURCE_DIR}/../../ext CACHE STRING "external libraries location")

   include_directories(${EXTERNAL_LIBS}/AntTweakBar/include)
   include_directories(${EXTERNAL_LIBS}/freeglut/include)
   include_directories(${EXTERNAL_LIBS}/glew-1.10.0/include)
   include_directories(${EXTERNAL_LIBS}/glm)
   include_directories(${EXTERNAL_LIBS}/FreeImage/include)

   link_directories(${EXTERNAL_LIBS}/AntTweakBar/lib)
   link_directories(${EXTERNAL_LIBS}/freeglut/lib)
   link_directories(${EXTERNAL_LIBS}/glew-1.10.0/lib/Release/Win32)
   link_directories(${EXTERNAL_LIBS}/FreeImage/lib)

   add_executable(main ${cpps} ${headers} ${shaders})
   source_group(shaders FILES ${shaders})

   target_link_libraries(main freeglut glew32 AntTweakBar FreeImage)
   
   add_custom_command(TARGET main POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${EXTERNAL_LIBS}/AntTweakBar/lib"
        $<TARGET_FILE_DIR:main>)
   add_custom_command(TARGET main POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${EXTERNAL_LIBS}/glew-1.10.0/bin/Release/Win32"
        $<TARGET_FILE_DIR:main>)
   add_custom_command(TARGET main POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${EXTERNAL_LIBS}/freeglut/bin"
        $<TARGET_FILE_DIR:main>)
    add_custom_command(TARGET main POST_BUILD
     COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${EXTERNAL_LIBS}/FreeImage/bin"
        $<TARGET_FILE_DIR:main>)

   add_custom_command(TARGET main POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_SOURCE_DIR}/shaders"
        $<TARGET_FILE_DIR:main>/shaders)
    add_custom_command(TARGET main POST_BUILD
     COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_SOURCE_DIR}/content"
        $<TARGET_FILE_DIR:main>/content)

ELSE (WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++11")

    add_executable(main ${cpps} ${headers})

    find_package(OpenGL REQUIRED)
    find_package(GLUT REQUIRED)

    find_package(GLEW REQUIRED)
    include_directories(${GLEW_INCLUDE_DIRS})
    link_directories(${GLEW_LIBRARY_DIRS})
    add_definitions(${GLEW_DEFINITIONS})
    if(NOT GLEW_FOUND)
       message(Error "GLEW not found")
    endif(NOT GLEW_FOUND)

   include_directories(${OPENGL_INCLUDE_DIRS}  ${GLUT_INCLUDE_DIRS} ${GLEW_INCLUDE_DIRS})
   target_link_libraries(main AntTweakBar X11 GL glut GLEW)
ENDIF (WIN32)
