# 希沃 XHS11-EA (Allwinner A133 / sun50iw10p1) TWRP 设备树
# 参数取自官方 recovery.img: D:\seewoTest\firmware_x\Firmware\images\recovery.img
#   kernel_addr=0x40080000 ramdisk_addr=0x43000000 tags_addr=0x40000100
#   page_size=0x800 (2048) header_version=2  dtb_size=0 (dtb 由 u-boot 管理)

TARGET_BOARD_PLATFORM := sun50iw10p1
TARGET_BOARD_PLATFORM_GPU :=

# ---- CPU 架构: A133 = 4x Cortex-A53, 32 位内核 (armv7a) ----
TARGET_ARCH := arm
TARGET_ARCH_VARIANT := armv8-a
TARGET_CPU_VARIANT := cortex-a53
TARGET_CPU_ABI := armeabi-v7a
TARGET_CPU_ABI2 := armeabi

# ---- boot image 布局（与官方 recovery.img 完全一致）----
BOARD_KERNEL_CMDLINE := selinux=1 androidboot.selinux=enforcing androidboot.dtbo_idx=0,1,2 buildvariant=user
BOARD_KERNEL_BASE := 0x40080000
BOARD_RAMDISK_OFFSET := 0x43000000
BOARD_SECOND_OFFSET := 0x40F00000
BOARD_TAGS_OFFSET := 0x40000100
BOARD_PAGE_SIZE := 2048
BOARD_BOOT_HEADER_VERSION := 2
BOARD_BOOTIMAGE_PARTITION_SIZE := 33554432
BOARD_RECOVERYIMAGE_PARTITION_SIZE := 33554432
BOARD_FLASH_BLOCK_SIZE := 2048

# 预编译内核：官方 recovery.img 提取（dtb 不内嵌，u-boot 自行加载）
TARGET_PREBUILT_KERNEL := device/ceres/xhs11ea/kernel

# ---- 动态分区 (super) ----
BOARD_SUPER_PARTITION := super
BOARD_SUPER_PARTITION_SIZE := 2147483648
BOARD_SUPER_PARTITION_GROUPS := seewo_dynamic
BOARD_SEEWO_DYNAMIC_PARTITION_SIZE := 2130706432
BOARD_SEEWO_DYNAMIC_PARTITIONS_PARTITION_LIST := system vendor product

# ---- TWRP 配置 ----
TARGET_RECOVERY_FSTAB := device/ceres/xhs11ea/recovery.fstab
TW_THEME := portrait_hdpi
TW_EXCLUDE_TWRPAPP := true
TW_EXCLUDE_SUPERSU := true
TW_INCLUDE_CRYPTO := false
TW_USE_TOOLBOX := true
TW_SCREEN_BLANK_ON_BOOT := false
# A133 背光路径未知时不做亮度调节，避免 rc 找不到路径报错
# TW_BRIGHTNESS_PATH := /sys/class/backlight/sunxi-backlight/brightness
TW_MAX_BRIGHTNESS := 255
TW_DEFAULT_BRIGHTNESS := 200

# 由 u-boot 管理 dtb，编译产物不带 dtb 段（与官方 recovery.img 一致）
BOARD_INCLUDE_DTB_IN_BOOTIMG := false
