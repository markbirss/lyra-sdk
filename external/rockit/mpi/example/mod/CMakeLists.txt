cmake_minimum_required( VERSION 2.8.8 )
add_definitions(-Wno-attributes -Wno-deprecated-declarations -DANDROID_STL=c++_shared -DOS_LINUX)

set(RK_MPI_TEST_AO_SRC
    test_mpi_ao.c
)

set(RK_MPI_TEST_ADEC_SRC
    test_mpi_adec.c
)

# set(RK_MPI_TEST_VPSS_SRC
#     test_mpi_vpss.cpp
# )

# set(RK_MPI_TEST_AVS_SRC
#     test_mpi_avs.cpp
# )

# set(RK_MPI_TEST_VGS_SRC
#     test_mpi_vgs.cpp
# )

set(RK_MPI_TEST_MB_SRC
    test_mpi_mb.c
)

set(RK_MPI_TEST_SYS_SRC
    test_mpi_sys.c
)

# set(RK_MPI_TEST_SYS_DEV_SRC
#     sys/test_sys_device_priority.cpp
# )

#set(RK_MPI_TEST_VDEC_SRC
#    test_mpi_vdec.cpp
#)

#set(RK_MPI_TEST_SIMPLE_VDEC_SRC
#    test_mpi_simple_vdec.cpp
#)

set(RK_MPI_TEST_VENC_SRC
     test_mpi_venc.c
)

set(RK_MPI_TEST_TDE_SRC
     test_mpi_tde.c
)

set(RK_MPI_TEST_AI_SRC
    test_mpi_ai.c
)

# set(RK_MPI_TEST_RGN_SRC
#     test_mpi_rgn.cpp
# )

#set(RK_MPI_TEST_VO_SRC
#    test_mpi_vo.cpp
#)

set(RK_MPI_TEST_AENC_SRC
    test_mpi_aenc.c
)

set(RK_MPI_TEST_AMIX_SRC
    test_mpi_amix.c
)

# set(RK_MPI_TEST_VI_SRC
#     test_mpi_vi.cpp
# )

#set(RK_MPI_TEST_MMZ_SRC
#    test_mpi_mmz.cpp
#)

# set(RK_MPI_TEST_GDC_SRC
#     test_mpi_gdc.cpp
# )

# set(RK_MPI_TEST_DIS_SRC
#     test_mpi_dis.cpp
# )

#set(RK_MPI_TEST_AVIO_SRC
#    sys/test_sys_avio.cpp
#)

# set(RK_MPI_TEST_PVS_SRC
#     test_mpi_pvs.cpp
# )

#--------------------------
# rk_mpi_ao_test
#--------------------------
add_executable(rk_mpi_ao_test ${RK_MPI_TEST_AO_SRC} ${RK_MPI_TEST_COMMON_SRC})
target_link_libraries(rk_mpi_ao_test ${ROCKIT_DEP_COMMON_LIBS})
install(TARGETS rk_mpi_ao_test RUNTIME DESTINATION "bin")

#--------------------------
# rk_mpi_adec_test
#--------------------------
add_executable(rk_mpi_adec_test ${RK_MPI_TEST_ADEC_SRC} ${RK_MPI_TEST_COMMON_SRC})
target_link_libraries(rk_mpi_adec_test ${ROCKIT_DEP_COMMON_LIBS})
install(TARGETS rk_mpi_adec_test RUNTIME DESTINATION "bin")

# #--------------------------
# # rk_mpi_vpss_test
# #--------------------------
# add_executable(rk_mpi_vpss_test ${RK_MPI_TEST_VPSS_SRC} ${RK_MPI_TEST_COMMON_SRC})
# target_link_libraries(rk_mpi_vpss_test ${ROCKIT_DEP_COMMON_LIBS})
# install(TARGETS rk_mpi_vpss_test RUNTIME DESTINATION "bin")

# #--------------------------
# # rk_mpi_avs_test
# #--------------------------
# add_executable(rk_mpi_avs_test ${RK_MPI_TEST_AVS_SRC} ${RK_MPI_TEST_COMMON_SRC})
# target_link_libraries(rk_mpi_avs_test ${ROCKIT_DEP_COMMON_LIBS})
# install(TARGETS rk_mpi_avs_test RUNTIME DESTINATION "bin")

#--------------------------
# rk_mpi_mb_test
#--------------------------
add_executable(rk_mpi_mb_test ${RK_MPI_TEST_MB_SRC} ${RK_MPI_TEST_COMMON_SRC})
target_link_libraries(rk_mpi_mb_test ${ROCKIT_DEP_COMMON_LIBS})
install(TARGETS rk_mpi_mb_test RUNTIME DESTINATION "bin")

#--------------------------
# rk_mpi_sys_test
#--------------------------
add_executable(rk_mpi_sys_test ${RK_MPI_TEST_SYS_SRC} ${RK_MPI_TEST_COMMON_SRC})
target_link_libraries(rk_mpi_sys_test ${ROCKIT_DEP_COMMON_LIBS})
install(TARGETS rk_mpi_sys_test RUNTIME DESTINATION "bin")

#--------------------------
# rk_mpi_vdec_test
#add_executable(rk_mpi_vdec_test ${RK_MPI_TEST_VDEC_SRC} ${RK_MPI_TEST_COMMON_SRC})
#target_link_libraries(rk_mpi_vdec_test ${ROCKIT_DEP_COMMON_LIBS_FULL})
#install(TARGETS rk_mpi_vdec_test RUNTIME DESTINATION "bin")

#--------------------------
# rk_mpi_venc_test
#--------------------------
add_executable(rk_mpi_venc_test ${RK_MPI_TEST_VENC_SRC} ${RK_MPI_TEST_COMMON_SRC})
target_link_libraries(rk_mpi_venc_test ${ROCKIT_DEP_COMMON_LIBS})
install(TARGETS rk_mpi_venc_test RUNTIME DESTINATION "bin")

#--------------------------
# rk_mpi_tde_test
#--------------------------
add_executable(rk_mpi_tde_test ${RK_MPI_TEST_TDE_SRC} ${RK_MPI_TEST_COMMON_SRC})
target_link_libraries(rk_mpi_tde_test ${ROCKIT_DEP_COMMON_LIBS})
install(TARGETS rk_mpi_tde_test RUNTIME DESTINATION "bin")

#--------------------------
# rk_mpi_ai_test
#--------------------------
add_executable(rk_mpi_ai_test ${RK_MPI_TEST_AI_SRC} ${RK_MPI_TEST_COMMON_SRC})
target_link_libraries(rk_mpi_ai_test ${ROCKIT_DEP_COMMON_LIBS})
install(TARGETS rk_mpi_ai_test RUNTIME DESTINATION "bin")

# rk_mpi_rgn_test
#--------------------------
# add_executable(rk_mpi_rgn_test ${RK_MPI_TEST_RGN_SRC} ${RK_MPI_TEST_COMMON_SRC})
# target_link_libraries(rk_mpi_rgn_test ${ROCKIT_DEP_COMMON_LIBS})
# install(TARGETS rk_mpi_rgn_test RUNTIME DESTINATION "bin")

#--------------------------
# rk_mpi_vo_test
#--------------------------
#add_executable(rk_mpi_vo_test ${RK_MPI_TEST_VO_SRC} ${RK_MPI_TEST_COMMON_SRC})
#target_link_libraries(rk_mpi_vo_test ${ROCKIT_DEP_COMMON_LIBS_FULL})
#install(TARGETS rk_mpi_vo_test RUNTIME DESTINATION "bin")

# rk_mpi_aenc_test
#--------------------------
add_executable(rk_mpi_aenc_test ${RK_MPI_TEST_AENC_SRC} ${RK_MPI_TEST_COMMON_SRC})
target_link_libraries(rk_mpi_aenc_test ${ROCKIT_DEP_COMMON_LIBS})
install(TARGETS rk_mpi_aenc_test RUNTIME DESTINATION "bin")

#--------------------------
# rk_mpi_vi_test
#--------------------------
# add_executable(rk_mpi_vi_test ${RK_MPI_TEST_VI_SRC} ${RK_MPI_TEST_COMMON_SRC})
# target_link_libraries(rk_mpi_vi_test ${ROCKIT_DEP_COMMON_LIBS})
# install(TARGETS rk_mpi_vi_test RUNTIME DESTINATION "bin")

#--------------------------
# rk_mpi_mmz_test
#--------------------------
#add_executable(rk_mpi_mmz_test ${RK_MPI_TEST_MMZ_SRC} ${RK_MPI_TEST_COMMON_SRC})
#target_link_libraries(rk_mpi_mmz_test ${ROCKIT_DEP_COMMON_LIBS})
#install(TARGETS rk_mpi_mmz_test RUNTIME DESTINATION "bin")

# #--------------------------
# # rk_mpi_gdc_test
# #--------------------------
# add_executable(rk_mpi_gdc_test ${RK_MPI_TEST_GDC_SRC} ${RK_MPI_TEST_COMMON_SRC})
# target_link_libraries(rk_mpi_gdc_test ${ROCKIT_DEP_COMMON_LIBS})
# install(TARGETS rk_mpi_gdc_test RUNTIME DESTINATION "bin")

#--------------------------
# rk_mpi_avio_test
#--------------------------
#add_executable(rk_mpi_avio_test ${RK_MPI_TEST_AVIO_SRC} ${RK_MPI_TEST_COMMON_SRC})
#target_link_libraries(rk_mpi_avio_test ${ROCKIT_DEP_COMMON_LIBS})
#install(TARGETS rk_mpi_avio_test RUNTIME DESTINATION "bin")
#--------------------------
# rk_mpi_amix_test
#--------------------------
add_executable(rk_mpi_amix_test ${RK_MPI_TEST_AMIX_SRC} ${RK_MPI_TEST_COMMON_SRC})
target_link_libraries(rk_mpi_amix_test ${ROCKIT_DEP_COMMON_LIBS})
install(TARGETS rk_mpi_amix_test RUNTIME DESTINATION "bin")
