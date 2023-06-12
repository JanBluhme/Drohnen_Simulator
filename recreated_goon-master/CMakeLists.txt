cmake_minimum_required(VERSION 3.11)

if(NOT RECREATED_GOON_CROSSCOMPILING)

    project(recreated_goon)

    find_package(FLEX REQUIRED)
    find_package(BISON REQUIRED)

    bison_target(Parser Parser.y ${CMAKE_CURRENT_BINARY_DIR}/Parser.cpp)
    flex_target(Scanner Scanner.l  ${CMAKE_CURRENT_BINARY_DIR}/Scanner.cpp)
    add_flex_bison_dependency(Scanner Parser)


    add_library(parser ${BISON_Parser_OUTPUTS} ${FLEX_Scanner_OUTPUTS})
    target_include_directories(parser PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc ${CMAKE_CURRENT_BINARY_DIR})
    set_property(TARGET parser PROPERTY CXX_STANDARD 20)
    set_property(TARGET parser PROPERTY CXX_EXTENSIONS FALSE)

    add_executable(generator
        src/generator.cpp
    )

    target_link_libraries(generator PRIVATE parser)
    set_property(TARGET generator PROPERTY CXX_STANDARD 20)
    set_property(TARGET generator PROPERTY CXX_EXTENSIONS FALSE)

else()
    execute_process(
        COMMAND cmake -E env CC=gcc CXX=g++ cmake -G "Unix Makefiles" ${CMAKE_CURRENT_LIST_DIR}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )

    add_custom_target(generator
        COMMAND make
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
endif()

find_program(clang-format clang-format)

function(goon_generate targetname filename gen)

    set(iname ${CMAKE_CURRENT_SOURCE_DIR}/${filename})
    set(dname ${CMAKE_CURRENT_BINARY_DIR}/generated)
    set(oname ${dname}/${targetname}.h)

    get_target_property(build_dir generator BINARY_DIR)

    set(command ${build_dir}/generator
                --generator=${gen}
                --input=${iname}
                --output=${oname}
                ${ARGN})

    if(NOT EXISTS ${clang-format})
        add_custom_command(
            COMMAND cmake -E make_directory ${dname}
            COMMAND ${command}
            DEPENDS generator ${iname}
            OUTPUT ${oname}
        )
    else()
        add_custom_command(
            COMMAND cmake -E make_directory ${dname}
            COMMAND ${command}
            COMMAND ${clang-format} -i ${oname}
            DEPENDS generator ${iname}
            OUTPUT ${oname}
        )
    endif()

    if(${CMAKE_VERSION} VERSION_LESS "3.19.0")
        set_source_files_properties(
            ${oname}
            PROPERTIES GENERATED TRUE
        )
        add_custom_target(
            ${targetname}_helper
            DEPENDS ${oname}
        )
        add_library(${targetname} INTERFACE)
        add_dependencies(${targetname} ${targetname}_helper)
    else()
        add_library(${targetname} INTERFACE ${oname})
    endif()

    target_include_directories(${targetname} INTERFACE ${dname})

endfunction()

