# (c) Aleksandr Shevchenko e-mail : Sasha7b9@tut.by

project (flasher_gui)

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

file(GLOB SRC       src/*.h src/*.cpp)

source_group(src    FILES ${SRC})

set_source_files_properties(
    Communicator/LAN/libnet/netserver.cpp
    Communicator/LAN/libnet/netbase.cpp
    Communicator/LAN/libnet/netclient.cpp
    PROPERTIES SKIP_UNITY_BUILD_INCLUSION ON
)


add_executable( ${PROJECT_NAME}
    ${SRC}
)

target_link_libraries(${PROJECT_NAME}
    wxbase32ud wxmsw32ud_core wxbase32ud_net wxzlibd wxpngd
    Ws2_32 Comctl32 Rpcrt4
)
