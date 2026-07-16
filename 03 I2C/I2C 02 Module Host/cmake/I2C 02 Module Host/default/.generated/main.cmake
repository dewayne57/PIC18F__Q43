include("${CMAKE_CURRENT_LIST_DIR}/rule.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/file.cmake")

set(I2C_02_Module_Host_default_library_list )

# Handle files with suffix (s|as|asm|AS|ASM|As|aS|Asm), for group default-XC8
if(I2C_02_Module_Host_default_default_XC8_FILE_TYPE_assemble)
add_library(I2C_02_Module_Host_default_default_XC8_assemble OBJECT ${I2C_02_Module_Host_default_default_XC8_FILE_TYPE_assemble})
    I2C_02_Module_Host_default_default_XC8_assemble_rule(I2C_02_Module_Host_default_default_XC8_assemble)
    list(APPEND I2C_02_Module_Host_default_library_list "$<TARGET_OBJECTS:I2C_02_Module_Host_default_default_XC8_assemble>")

endif()

# Handle files with suffix S, for group default-XC8
if(I2C_02_Module_Host_default_default_XC8_FILE_TYPE_assemblePreprocess)
add_library(I2C_02_Module_Host_default_default_XC8_assemblePreprocess OBJECT ${I2C_02_Module_Host_default_default_XC8_FILE_TYPE_assemblePreprocess})
    I2C_02_Module_Host_default_default_XC8_assemblePreprocess_rule(I2C_02_Module_Host_default_default_XC8_assemblePreprocess)
    list(APPEND I2C_02_Module_Host_default_library_list "$<TARGET_OBJECTS:I2C_02_Module_Host_default_default_XC8_assemblePreprocess>")

endif()

# Handle files with suffix [cC], for group default-XC8
if(I2C_02_Module_Host_default_default_XC8_FILE_TYPE_compile)
add_library(I2C_02_Module_Host_default_default_XC8_compile OBJECT ${I2C_02_Module_Host_default_default_XC8_FILE_TYPE_compile})
    I2C_02_Module_Host_default_default_XC8_compile_rule(I2C_02_Module_Host_default_default_XC8_compile)
    list(APPEND I2C_02_Module_Host_default_library_list "$<TARGET_OBJECTS:I2C_02_Module_Host_default_default_XC8_compile>")

endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../PPSLIB/default/.generated/exported.cmake")
    include("${CMAKE_CURRENT_SOURCE_DIR}/../../PPSLIB/default/.generated/exported.cmake")
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../UARTLIB/default/.generated/exported.cmake")
    include("${CMAKE_CURRENT_SOURCE_DIR}/../../UARTLIB/default/.generated/exported.cmake")
endif()

# Main target for this project
add_executable(I2C_02_Module_Host_default_image_OpOTjqAD ${I2C_02_Module_Host_default_library_list})

set_target_properties(I2C_02_Module_Host_default_image_OpOTjqAD PROPERTIES
    OUTPUT_NAME "default"
    SUFFIX ".elf"
    ADDITIONAL_CLEAN_FILES "${output_extensions}"
    RUNTIME_OUTPUT_DIRECTORY "${I2C_02_Module_Host_default_output_dir}")
target_link_libraries(I2C_02_Module_Host_default_image_OpOTjqAD PRIVATE ${I2C_02_Module_Host_default_default_XC8_FILE_TYPE_link})

# Add the link options from the rule file.
I2C_02_Module_Host_default_link_rule( I2C_02_Module_Host_default_image_OpOTjqAD)

target_link_libraries(I2C_02_Module_Host_default_image_OpOTjqAD
PRIVATE
 ${CMAKE_LIBRARY_TO_LINK_VARPPSLIB_default_sdtLu5S1})
target_link_libraries(I2C_02_Module_Host_default_image_OpOTjqAD
PRIVATE
 ${CMAKE_LIBRARY_TO_LINK_VARUARTLIB_default_0YYJo8c4})

