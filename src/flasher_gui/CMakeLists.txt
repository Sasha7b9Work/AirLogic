# (c) Aleksandr Shevchenko e-mail : Sasha7b9@tut.by

project (Client-PC)

#set(CMAKE_UNITY_BUILD OFF)

#set(CMAKE_UNITY_BUILD_BATCH_SIZE 8)

include_directories(
    .
    ../../VS/ThirdParty/wxWidgets/include
    ../../generated/ThirdParty/lib/vc_dll/mswud
    ../../generated/ThirdParty/lib/vc_lib/mswud
)

link_directories(
    ../../generated/ThirdParty/lib/vc_dll
    ../../generated/ThirdParty/lib/vc_lib
)


if(WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -MP -Wall -MTd -std:c++17")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS")
    
    add_compile_options(
        /wd4365    # 'action' : conversion from 'type_1' to 'type_2', signed/unsigned mismatch
        /wd4514    # 'function' : unreferenced inline function has been removed
        /wd4625    # 'derived class' : copy constructor was implicitly defined as deleted because a base class copy constructor is inaccessible or deleted
        /wd4626    # 'derived class' : assignment operator was implicitly defined as deleted because a base class assignment operator is inaccessible or deleted
        /wd4820    # 'bytes' bytes padding added after construct 'member_name'
        /wd4996    # Your code uses a function, class member, variable, or typedef that's marked deprecated
        /wd5026    # move constructor was implicitly defined as deleted
        /wd5027    # move assignment operator was implicitly defined as deleted
        /wd5039    # 'xxx': pointer or reference to potentially throwing function passed to extern C
                                    # function under -EHc.
        /wd5045    # Compiler will insert Spectre mitigation for memory load if /Qspectre switch specified
    )
endif()

file(GLOB RESOURCES               resources/*.*)
file(GLOB SRC                     *.h *.cpp)
file(GLOB COMMUNICATOR            Communicator/*.*)
file(GLOB COMMUNICATOR_COMPORT    Communicator/ComPort/*.*)
file(GLOB COMMUNICATOR_HC12       Communicator/HC12/*.*)
file(GLOB COMMUNICATOR_HTTP       Communicator/HTTP/*.*)
file(GLOB COMMUNICATOR_LAN        Communicator/LAN/*.*)
file(GLOB COMMUNICATOR_LAN_LIBNET Communicator/LAN/libnet/*.*)
file(GLOB COMMUNICATOR_RS232      Communicator/RS232/*.*)
file(GLOB COMMUNICATOR_SERVER     Communicator/Server/*.*)
file(GLOB CONTROLS                Controls/*.*)
file(GLOB DATA                    Data/*.*)
file(GLOB DISPLAY                 Display/*.*)
file(GLOB DISPLAY_DIAGRAM         Display/Diagram/*.*)
file(GLOB DISPLAY_TABLE           Display/Table/*.*)
file(GLOB SETTINGS                Settings/*.*)
file(GLOB UTILS                   Utils/*.*)

source_group(resources               FILES ${RESOURCES})
source_group(src                     FILES ${SRC})
source_group(Communicator            FILES ${COMMUNICATOR})
source_group(Communicator/ComPort    FILES ${COMMUNICATOR_COMPORT})
source_group(Communicator/HC12       FILES ${COMMUNICATOR_HC12})
source_group(Communicator/HTTP       FILES ${COMMUNICATOR_HTTP})
source_group(Communicator/LAN        FILES ${COMMUNICATOR_LAN})
source_group(Communicator/LAN/libnet FILES ${COMMUNICATOR_LAN_LIBNET})
source_group(Communicator/RS232      FILES ${COMMUNICATOR_RS232})
source_group(Communicator/Server     FILES ${COMMUNICATOR_SERVER})
source_group(Controls                FILES ${CONTROLS})
source_group(Data                    FILES ${DATA})
source_group(Display                 FILES ${DISPLAY})
source_group(Display/Diagram         FILES ${DISPLAY_DIAGRAM})
source_group(Display/Table           FILES ${DISPLAY_TABLE})
source_group(Settings                FILES ${SETTINGS})
source_group(Utils                   FILES ${UTILS})

set_source_files_properties(
    Communicator/LAN/libnet/netserver.cpp
    Communicator/LAN/libnet/netbase.cpp
    Communicator/LAN/libnet/netclient.cpp
    PROPERTIES SKIP_UNITY_BUILD_INCLUSION ON
)


add_executable( ${PROJECT_NAME}
    ${RESOURCES}
    ${SRC}
    ${COMMUNICATOR}
    ${COMMUNICATOR_COMPORT}
    ${COMMUNICATOR_HC12}
    ${COMMUNICATOR_HTTP}
    ${COMMUNICATOR_LAN}
    ${COMMUNICATOR_LAN_LIBNET}
    ${COMMUNICATOR_RS232}
    ${COMMUNICATOR_SERVER}
    ${CONTROLS}
    ${DATA}
    ${DISPLAY}
    ${DISPLAY_DIAGRAM}
    ${DISPLAY_TABLE}
    ${SETTINGS}
    ${UTILS}
)

target_link_libraries(${PROJECT_NAME}
    wxbase32ud wxmsw32ud_core wxbase32ud_net wxzlibd wxpngd
    Ws2_32 Comctl32 Rpcrt4
)
