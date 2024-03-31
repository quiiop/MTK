BT sink module usage guide

Brief:          This module is the sink service which integrates A2DP, AVRCP profiles.
                It works as a Bluetooth speaker and supports the speaker features, such as, playing and pausing music,
                moving to previous song and next song.
Usage:          GCC: Include the module with "include $(SOURCE_DIR)/middleware/MTK/bt_sink/module.mk" in your GCC project makefile.
                KEIL: Drag the middleware/MTK/bt_sink folder to your project.
                      Add middleware/bt_sink/inc to include paths.
                IAR: Drag the middleware/MTK/bt_sink folder to your project.
                     Add middleware/MTK/bt_sink/inc to include paths.
Dependency:     This module has the dependency with Bluetooth. Please also make sure to include Bluetooth.
Relative doc:   None.
Example project: Please find the bga_sdk_demo project under /project/mt7933_hdk/apps folder.
