cmake_minimum_required(VERSION 3.8)

project(amp_monitor)

if (LVGL_V9)
	MESSAGE(FATAL_ERROR "This app no support LVGL9, please switch to LVGL8")
endif()

include_directories(${PROJECT_SOURCE_DIR}/./)

aux_source_directory(. SRCS)

add_executable(${PROJECT_NAME} ${SRCS})

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
install(PROGRAMS overlay/cyclictest_ui.sh DESTINATION bin)
install(FILES overlay/logo_lvgl.png DESTINATION /usr/share/icons)
install(FILES overlay/05-amp-monitor-launcher.ini DESTINATION /etc/xdg/weston/weston.ini.d)

