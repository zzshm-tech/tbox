idf_component_register(SRCS "app_main.c"
                     "./app/src/app_gnss.c"
                     "./app/src/app_led.c"
                     "./app/src/app_products.c"
                     "./app/src/app_shell.c"
                     "./app/src/app_lte.c"
                     "./app/src/app_can_recv.c"
                     "./app/src/app_can_send.c"
                     "./app/src/version.c"
                     "./app/src/app_in.c"
                     "./app/src/app_files.c"
                     "./app/src/app_gb4.c"
                     "./app/src/app_at.c"
                     "./app/src/app_acl16.c"
                     "./app/src/app_packet.c"
                     "./app/src/app_fifo.c"
                     "./app/src/app_iap.c"
                     "./app/src/app_archive.c"
                     "./app/src/app_sms.c"
                     

                     "./lib/src/drv_can.c"
                     "./lib/src/drv_rtc.c"
                     "./lib/src/board.c"
                     "./lib/src/drv_uart.c"
                     "./lib/src/drv_in.c"
                     "./lib/src/drv_spi.c"
                     "./lib/src/drv_i2c.c"
                     "./lib/src/drv_aw9523b.c"
                     "./lib/src/drv_gpio.c"
                     "./lib/src/drv_acl16.c"
                     "./lib/src/drv_power.c"
                     "./lib/src/common.c"

                     "./pro_data/md5_code.c"
                     "./pro_data/md5.c"
                     "./pro_data/pro_data.c"

                     "./ringbuffer/ringbuffer.c"


                     
                    INCLUDE_DIRS "."
                    INCLUDE_DIRS "./app/inc"
                    INCLUDE_DIRS "./lib/inc"
                    INCLUDE_DIRS "./pro_data"
                    INCLUDE_DIRS "./ringbuffer"
                    )
#
# ULP support additions to component CMakeLists.txt.
#
# 1. The ULP app name must be unique (if multiple components use ULP).

set(ulp_app_name ulp_${COMPONENT_NAME})
#
# 2. Specify all assembly source files.
#    Files should be placed into a separate directory (in this case, ulp/),
#    which should not be added to COMPONENT_SRCS.
set(ulp_s_sources "ulp/adc.S")
#
# 3. List all the component source files which include automatically
#    generated ULP export file, ${ulp_app_name}.h:
#set(ulp_exp_dep_srcs "ulp_adc_example_main.c")
#
# 4. Call function to build ULP binary and embed in project using the argument
#    values above.
ulp_embed_binary(${ulp_app_name} "${ulp_s_sources}" "${ulp_exp_dep_srcs}")


