configure_file("mynet_config.h.in" "mynet_config.h")

#add_executable(arrdemo arrdemo.cpp)
include_directories(${PROJECT_SOURCE_DIR}/include)
set(MYLIB_SOURCES  
    client.cpp  
    connect.cpp  
    epoll.cpp
    evbuffer.cpp event_base.cpp event_callback.cpp event.cpp
    factory.cpp listen.cpp log.cpp serv_conn.cpp server.cpp thread.cpp util.cpp
) 

add_library(mynet_static STATIC ${MYLIB_SOURCES})
#SET_TARGET_PROPERTIES(mynet_static PROPERTIES LINKER_LANGUAGE C++)

add_library(mynet_dyn SHARED ${MYLIB_SOURCES}) 
#SET_TARGET_PROPERTIES(mynet_dyn PROPERTIES LINKER_LANGUAGE C++)

#install(TARGETS arrdemo DESTINATION bin)
#add_test(arr1 arrdemo)
#set_tests_properties(arr1 PROPERTIES pass_regular_expression "xxx")