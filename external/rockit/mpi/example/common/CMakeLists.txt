cmake_minimum_required( VERSION 2.8.8 )

set(RK_TEST_MOD_SRC
    #test_mod_vpss.cpp
)

set(RK_TEST_COMMON_SRC
    test_comm_argparse.c
    # test_comm_avs.cpp
    test_comm_utils.c
    #test_comm_bmp.cpp
    test_comm_imgproc.c
    test_comm_sys.c
    # test_comm_rgn.cpp
    test_comm_venc.c
    # test_comm_vpss.cpp
    #test_comm_vdec.cpp
    #test_comm_vo.cpp
    #test_comm_ao.cpp
    test_comm_tde.c
    # test_comm_vgs.cpp
    #tmedia/test_comm_tmd.cpp
)

add_library(${RT_TEST_COMM_STATIC} STATIC ${RK_TEST_COMMON_SRC} ${RK_TEST_MOD_SRC})
set_target_properties(${RT_TEST_COMM_STATIC} PROPERTIES FOLDER "rt_test_comm")
target_link_libraries(${RT_TEST_COMM_STATIC})
