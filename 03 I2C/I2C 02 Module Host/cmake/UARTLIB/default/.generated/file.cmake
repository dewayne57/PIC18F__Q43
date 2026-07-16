# The following variables contains the files used by the different stages of the build process.
set(UARTLIB_default_default_XC8_FILE_TYPE_assemble)
set_source_files_properties(${UARTLIB_default_default_XC8_FILE_TYPE_assemble} PROPERTIES LANGUAGE ASM)

# For assembly files, add "." to the include path for each file so that .include with a relative path works
foreach(source_file ${UARTLIB_default_default_XC8_FILE_TYPE_assemble})
        set_source_files_properties(${source_file} PROPERTIES INCLUDE_DIRECTORIES "$<PATH:NORMAL_PATH,$<PATH:REMOVE_FILENAME,${source_file}>>")
endforeach()

set(UARTLIB_default_default_XC8_FILE_TYPE_assemblePreprocess)
set_source_files_properties(${UARTLIB_default_default_XC8_FILE_TYPE_assemblePreprocess} PROPERTIES LANGUAGE ASM)

# For assembly files, add "." to the include path for each file so that .include with a relative path works
foreach(source_file ${UARTLIB_default_default_XC8_FILE_TYPE_assemblePreprocess})
        set_source_files_properties(${source_file} PROPERTIES INCLUDE_DIRECTORIES "$<PATH:NORMAL_PATH,$<PATH:REMOVE_FILENAME,${source_file}>>")
endforeach()

set(UARTLIB_default_default_XC8_FILE_TYPE_compile "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../Libraries/UARTLIB/uartlib.c")
set_source_files_properties(${UARTLIB_default_default_XC8_FILE_TYPE_compile} PROPERTIES LANGUAGE C)
set(UARTLIB_default_default_XC8_FILE_TYPE_archive)
set(CMAKE_STATIC_LIBRARY_PREFIX )
set(CMAKE_STATIC_LIBRARY_SUFFIX .elf)
set(UARTLIB_default_image_name "default")
set(UARTLIB_default_image_base_name "default")

# The output directory of the final image.
set(UARTLIB_default_output_dir "${CMAKE_CURRENT_SOURCE_DIR}/../../../out/UARTLIB")

# The full path to the final image.
set(UARTLIB_default_full_path_to_image ${UARTLIB_default_output_dir}/${UARTLIB_default_image_name})

# Potential output file extensions
set(output_extensions
    .hex
    .hxl
    .mum
    .o
    .sdb
    .sym
    .cmf)
list(TRANSFORM output_extensions PREPEND "${UARTLIB_default_output_dir}/${UARTLIB_default_image_base_name}")
