if(CONFIG_MZST_NETWORK_MODULE)
    set(srcs NetworkModule.c)
elseif(CONFIG_MZST_TRANSMITTER_MODULE)
    set(srcs TransmitterModule.c)
endif()

idf_component_register(SRCS "${srcs}"
                    INCLUDE_DIRS "include"
                    REQUIRES Server)
