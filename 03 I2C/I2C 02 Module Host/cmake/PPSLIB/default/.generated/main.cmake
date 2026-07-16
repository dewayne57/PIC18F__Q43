include("${CMAKE_CURRENT_LIST_DIR}/rule.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/file.cmake")

set(PPSLIB_default_library_list )

# Handle files with suffix (s|as|asm|AS|ASM|As|aS|Asm), for group default-XC8
if(PPSLIB_default_default_XC8_FILE_TYPE_assemble)
add_library(PPSLIB_default_default_XC8_assemble OBJECT ${PPSLIB_default_default_XC8_FILE_TYPE_assemble})
    PPSLIB_default_default_XC8_assemble_rule(PPSLIB_default_default_XC8_assemble)
    list(APPEND PPSLIB_default_library_list "$<TARGET_OBJECTS:PPSLIB_default_default_XC8_assemble>")

endif()

# Handle files with suffix S, for group default-XC8
if(PPSLIB_default_default_XC8_FILE_TYPE_assemblePreprocess)
add_library(PPSLIB_default_default_XC8_assemblePreprocess OBJECT ${PPSLIB_default_default_XC8_FILE_TYPE_assemblePreprocess})
    PPSLIB_default_default_XC8_assemblePreprocess_rule(PPSLIB_default_default_XC8_assemblePreprocess)
    list(APPEND PPSLIB_default_library_list "$<TARGET_OBJECTS:PPSLIB_default_default_XC8_assemblePreprocess>")

endif()

# Handle files with suffix [cC], for group default-XC8
if(PPSLIB_default_default_XC8_FILE_TYPE_compile)
add_library(PPSLIB_default_default_XC8_compile OBJECT ${PPSLIB_default_default_XC8_FILE_TYPE_compile})
    PPSLIB_default_default_XC8_compile_rule(PPSLIB_default_default_XC8_compile)
    list(APPEND PPSLIB_default_library_list "$<TARGET_OBJECTS:PPSLIB_default_default_XC8_compile>")

endif()

add_library(
    PPSLIB_default_image_sdtLu5S1
    ${PPSLIB_default_library_list})
set_target_properties(PPSLIB_default_image_sdtLu5S1 PROPERTIES
    OUTPUT_NAME "default"
    SUFFIX ".elf"
    ADDITIONAL_CLEAN_FILES "${output_extensions}"
    ARCHIVE_OUTPUT_DIRECTORY "${PPSLIB_default_output_dir}")
foreach(lib ${PPSLIB_default_FILE_TYPE__link})
    target_link_libraries(PPSLIB_default_image_sdtLu5S1
    PRIVATE
     ${lib})
endforeach()
# Add the archiver options from the rule file.
PPSLIB_default_archiver_rule( PPSLIB_default_image_sdtLu5S1)


