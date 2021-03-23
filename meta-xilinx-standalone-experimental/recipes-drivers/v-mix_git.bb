inherit distro_features_check

REQUIRED_DISTRO_FEATURES = "v-mix"

inherit esw python3native

DEPENDS += "python3-pyyaml-native xilstandalone lopper python3-dtc-native video-common"

ESW_COMPONENT_SRC = "/XilinxProcessorIPLib/drivers/v_mix/src/"
ESW_COMPONENT_NAME = "libv_mix.a"

addtask do_generate_driver_data before do_configure after do_prepare_recipe_sysroot
do_prepare_recipe_sysroot[rdeptask] = "do_unpack"
