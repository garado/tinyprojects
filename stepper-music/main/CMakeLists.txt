idf_component_register(SRCS "main.c"
                            "midi.c"
                    INCLUDE_DIRS "." "include")

set(image ../fatfs_image)

if(CONFIG_EXAMPLE_FATFS_DEFAULT_DATETIME)
  fatfs_create_rawflash_image(storage ${image} FLASH_IN_PROJECT)
else()
  fatfs_create_rawflash_image(storage ${image} FLASH_IN_PROJECT PRESERVE_TIME)
endif()
