cmake_minimum_required(VERSION 2.8.3)
project(associative-memory)

add_definitions(-std=c++11)


######################################################
##            libassociative-memory                 ##
######################################################
######################################################

find_package(Eigen3 REQUIRED)
include_directories(${EIGEN3_INCLUDE_DIR})

add_library(${PROJECT_NAME} SHARED src/memory_network.cpp)
target_link_libraries(${PROJECT_NAME} 
                      ${EIGEN3_LIBRARIES})

set(HEADERS src/memory_network.hpp)

install(TARGETS ${PROJECT_NAME}
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(FILES
    ${HEADERS}
    DESTINATION include/${PROJECT_NAME}
)

######################################################
##               experiment runner                  ##
######################################################
######################################################

option(WITH_RUNNER "Compile the experiment runner" ON)

if(WITH_RUNNER)

    find_package(Boost COMPONENTS program_options REQUIRED)

    include_directories(src/
                        ${Boost_INCLUDE_DIRS})

    file(GLOB_RECURSE SRC src-runner/*.cpp)

    add_executable(runner ${SRC})
    target_link_libraries(runner
                            ${PROJECT_NAME} 
                            ${Boost_LIBRARIES})

    install(TARGETS runner
        RUNTIME DESTINATION bin
    )

endif()


######################################################
##                 memory-view                      ##
######################################################
######################################################

option(WITH_VISUALIZATION "Compile the OpenGL visualization tool" ON)

if(WITH_VISUALIZATION)

    find_package(PkgConfig REQUIRED)
    find_package(OpenGL REQUIRED)
    find_package(SDL REQUIRED)
    find_package(SDL_image REQUIRED)
    find_package(Boost COMPONENTS program_options REQUIRED)

    pkg_search_module(FTGL REQUIRED ftgl)
    pkg_search_module(JSONCPP REQUIRED jsoncpp)

    include_directories(src/
                        ${OPENGL_INCLUDE_DIRS}
                        ${SDL_INCLUDE_DIR}
                        ${SDL_IMAGE_INCLUDE_DIRS}
                        ${Boost_INCLUDE_DIRS}
                        ${FTGL_INCLUDE_DIRS} 
                        ${JSONCPP_INCLUDE_DIRS})

    file(GLOB_RECURSE SRC src-viz/*.cpp)

    add_executable(memory-view ${SRC})
    target_link_libraries(memory-view
                            ${PROJECT_NAME} 
                            ${OPENGL_LIBRARIES} 
                            ${SDL_LIBRARY} 
                            ${SDL_IMAGE_LIBRARIES} 
                            ${Boost_LIBRARIES} 
                            ${FTGL_LIBRARIES}
                            ${JSONCPP_LIBRARIES})

    install(TARGETS memory-view
        RUNTIME DESTINATION bin
    )

endif()


