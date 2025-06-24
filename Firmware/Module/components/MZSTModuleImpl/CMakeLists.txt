if(CONFIG_MZST_NETWORK_MODULE)
    set(srcs NetworkModule.c)
elseif(CONFIG_MZST_TRANSMITTER_MODULE)
    set(srcs TransmitterModule.c)
endif()

list(APPEND srcs "CommonModule.c")

idf_component_register(SRCS "CommonModule.c" "NetworkModule.c" "${srcs}"
                    INCLUDE_DIRS "include"
                    REQUIRES NVSUtil Server esp_driver_gpio esp_driver_ledc driver)
