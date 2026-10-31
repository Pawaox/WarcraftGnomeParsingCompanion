function(embed_resources target)
    set(outputDirectory "${CMAKE_CURRENT_BINARY_DIR}/generated/resources")
    file(MAKE_DIRECTORY "${outputDirectory}")

    foreach(resourceFile IN LISTS ARGN)
        get_filename_component(name "${resourceFile}" NAME_WE)
        get_filename_component(extension "${resourceFile}" LAST_EXT)
        string(SUBSTRING "${extension}" 1 -1 extension)
        set(symbol "${name}_${extension}")

        file(READ "${resourceFile}" hex HEX)
        string(LENGTH "${hex}" hexLength)
        math(EXPR byteCount "${hexLength} / 2")
        string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," bytes "${hex}")
        string(REGEX REPLACE "((0x..,)(0x..,)(0x..,)(0x..,)(0x..,)(0x..,)(0x..,)(0x..,)(0x..,)(0x..,)(0x..,)(0x..,)(0x..,)(0x..,)(0x..,)(0x..,))" "\\1\n    " bytes "${bytes}")

        file(WRITE "${outputDirectory}/${symbol}.h.tmp"
            "#pragma once\n\ninline constexpr unsigned char ${symbol}[${byteCount}] =\n{\n    ${bytes}\n};\n")
        file(COPY_FILE "${outputDirectory}/${symbol}.h.tmp" "${outputDirectory}/${symbol}.h" ONLY_IF_DIFFERENT)
        file(REMOVE "${outputDirectory}/${symbol}.h.tmp")

        set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${resourceFile}")
    endforeach()

    target_include_directories(${target} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated")
endfunction()
