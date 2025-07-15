if(CONFIG_MZST_NETWORK_MODULE)
    set(_srcs NetworkModule.c)
elseif(CONFIG_MZST_TRANSMITTER_MODULE)
    set(_srcs TransmitterModule.c)
endif()

list(APPEND _srcs "CommonModule.c")

idf_component_register(SRCS "CommonModule.c" "${_srcs}"
                    INCLUDE_DIRS "include"
                    REQUIRES NVSUtil Server esp_driver_gpio esp_driver_ledc esp_timer driver)
