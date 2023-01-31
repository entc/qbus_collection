# abstract operation-system layer
INCLUDE_DIRECTORIES("..")
SET(CMAKE_BUILD_TYPE Release)                                   # build type

add_executable          (ut_fmt_json ut_fmt_json.c)
target_link_libraries   (ut_fmt_json cape)

add_executable          (ut_stc_map ut_stc_map.c)
target_link_libraries   (ut_stc_map cape)

add_executable          (ut_stc_list ut_stc_list.c)
target_link_libraries   (ut_stc_list cape)

add_executable          (ut_stc_string ut_stc_string.c)
target_link_libraries   (ut_stc_string cape)

add_executable          (ut_stc_udc ut_stc_udc.c)
target_link_libraries   (ut_stc_udc cape)

add_executable          (ut_sys_exec ut_sys_exec.c)
target_link_libraries   (ut_sys_exec cape)

add_executable          (ut_sys_file ut_sys_file.c)
target_link_libraries   (ut_sys_file cape)

add_executable          (ut_hpp_stc ut_hpp_stc.cc)
target_link_libraries   (ut_hpp_stc cape)

add_executable          (ut_aio_socket_udp ut_aio_socket_udp.c)
target_link_libraries   (ut_aio_socket_udp cape)

add_executable          (ut_aio_timer ut_aio_timer.c)
target_link_libraries   (ut_aio_timer cape)

add_executable          (ut_aio_socket_icmp ut_aio_socket_icmp.c)
target_link_libraries   (ut_aio_socket_icmp cape)

add_executable          (ut_sys_time ut_sys_time.c)
target_link_libraries   (ut_sys_time cape)

add_executable          (ut_sys_queue ut_sys_queue.c)
target_link_libraries   (ut_sys_queue cape)

add_executable          (ut_fmt_template ut_fmt_template.c)
target_link_libraries   (ut_fmt_template cape)

add_executable          (ut_fmt_tokenizer ut_fmt_tokenizer.c)
target_link_libraries   (ut_fmt_tokenizer cape)
