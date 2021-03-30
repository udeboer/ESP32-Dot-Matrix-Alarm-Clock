idf_component_register(SRCS "clock001_main.c" "filesystem.c" "nvramfunctions.c" "eventhandler.c"
                    "networkstartstop.c" "webserver.c" "http_get.c" "http_post.c" "http_api_json.c"
                    "http_api_upload_files.c" "64bitpatch_localtime.c" "time_task.c" "app_queue.c" 
                    "display_clock.c" "max7219.c" "rotary_encoder.c" "display_functions.c"
                    "json_network.c" "json_files.c" "json_clock.c" "json_wavs.c"
                    "json_time.c" "sound.c" "i2c_functions.c" "ds3231.c"
                    INCLUDE_DIRS ".")

# Create a SPIFFS image from the contents of the 'spiffs_files' directory
# that fits the partition named 'spiffs1'. FLASH_IN_PROJECT indicates that
# the generated image should be flashed when the entire project is flashed to
# the target with 'idf.py -p PORT flash'. 
spiffs_create_partition_image(spiffs1 ../spiffs_files FLASH_IN_PROJECT)
