include("${CMAKE_CURRENT_LIST_DIR}/rule.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/file.cmake")

set(UARTLIB_default_library_list )

# Handle files with suffix (s|as|asm|AS|ASM|As|aS|Asm), for group default-XC8
if(UARTLIB_default_default_XC8_FILE_TYPE_assemble)
add_library(UARTLIB_default_default_XC8_assemble OBJECT ${UARTLIB_default_default_XC8_FILE_TYPE_assemble})
    UARTLIB_default_default_XC8_assemble_rule(UARTLIB_default_default_XC8_assemble)
    list(APPEND UARTLIB_default_library_list "$<TARGET_OBJECTS:UARTLIB_default_default_XC8_assemble>")

endif()

# Handle files with suffix S, for group default-XC8
if(UARTLIB_default_default_XC8_FILE_TYPE_assemblePreprocess)
add_library(UARTLIB_default_default_XC8_assemblePreprocess OBJECT ${UARTLIB_default_default_XC8_FILE_TYPE_assemblePreprocess})
    UARTLIB_default_default_XC8_assemblePreprocess_rule(UARTLIB_default_default_XC8_assemblePreprocess)
    list(APPEND UARTLIB_default_library_list "$<TARGET_OBJECTS:UARTLIB_default_default_XC8_assemblePreprocess>")

endif()

# Handle files with suffix [cC], for group default-XC8
if(UARTLIB_default_default_XC8_FILE_TYPE_compile)
add_library(UARTLIB_default_default_XC8_compile OBJECT ${UARTLIB_default_default_XC8_FILE_TYPE_compile})
    UARTLIB_default_default_XC8_compile_rule(UARTLIB_default_default_XC8_compile)
    list(APPEND UARTLIB_default_library_list "$<TARGET_OBJECTS:UARTLIB_default_default_XC8_compile>")

endif()

add_library(
    UARTLIB_default_image_0YYJo8c4
    ${UARTLIB_default_library_list})
set_target_properties(UARTLIB_default_image_0YYJo8c4 PROPERTIES
    OUTPUT_NAME "default"
    SUFFIX ".elf"
    ADDITIONAL_CLEAN_FILES "${output_extensions}"
    ARCHIVE_OUTPUT_DIRECTORY "${UARTLIB_default_output_dir}")
foreach(lib ${UARTLIB_default_FILE_TYPE__link})
    target_link_libraries(UARTLIB_default_image_0YYJo8c4
    PRIVATE
     ${lib})
endforeach()
# Add the archiver options from the rule file.
UARTLIB_default_archiver_rule( UARTLIB_default_image_0YYJo8c4)


