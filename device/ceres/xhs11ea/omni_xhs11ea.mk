$(call inherit-product, vendor/omni/config/common.mk)

# 设备标识
PRODUCT_DEVICE := xhs11ea
PRODUCT_NAME := omni_xhs11ea
PRODUCT_BRAND := Seewo
PRODUCT_MODEL := XHS11-EA
PRODUCT_MANUFACTURER := Seewo
PRODUCT_RECOVERY_NAME := recovery

# 动态分区支持（TWRP 挂载 super 内 system/vendor/product 需要）
PRODUCT_USE_DYNAMIC_PARTITIONS := true

# lptools: lpmake/lpunpack 等动态分区工具，TWRP 读取 super metadata 依赖
PRODUCT_PACKAGES += \
    lptools \
    lpmake

# 明文 data 路线（vendor 已去 FBE），TWRP 不需要 keymaster 解密
PRODUCT_PACKAGES += \
    toybox \
    e2fsck \
    fsck.f2fs

PRODUCT_COPY_FILES += \
    device/ceres/xhs11ea/recovery.fstab:recovery/root/etc/recovery.fstab
