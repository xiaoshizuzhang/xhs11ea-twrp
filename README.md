# TWRP for Seewo XHS11-EA (Allwinner A133 / sun50iw10p1, Android 10, arm32)

用 GitHub Actions 编译的 TWRP 设备树。参数全部取自官方 recovery.img（`firmware_x\Firmware\images\recovery.img`），
内核为官方预编译内核（从官方 recovery.img 提取，22.2MB）。

## 目录结构

```
xhs11ea_twrp_source/
├── .github/workflows/build-twrp.yml   # GitHub Actions 编译流程
└── device/ceres/xhs11ea/
    ├── Android.mk
    ├── AndroidProducts.mk
    ├── BoardConfig.mk                  # A133 boot 参数 + TWRP 配置
    ├── omni_xhs11ea.mk                # 产品定义（动态分区 + lptools）
    ├── recovery.fstab                 # 官方分区表（data=UDISK f2fs 明文路线）
    └── kernel                         # 官方 recovery 提取的预编译内核
```

## 使用方法（需 GitHub 账号）

1. 在 GitHub 新建一个仓库（public/private 均可），把本目录内容 push 上去：

   ```bash
   cd xhs11ea_twrp_source
   git init
   git add .
   git commit -m "TWRP device tree for XHS11-EA"
   git branch -M main
   git remote add origin https://github.com/<你的账号>/<仓库名>.git
   git push -u origin main
   ```

2. 仓库 → Actions 标签页 → 自动触发 "Build TWRP for Seewo XHS11-EA"；
   也可手动点 "Run workflow"。

3. 编译约 40~60 分钟。完成后在 Actions 运行页底部下载 `twrp-xhs11ea` artifact，
   里面就是 `recovery.img`。

## 刷入方法

```bash
# 方式一：fastboot
fastboot flash recovery recovery.img
fastboot reboot recovery

# 方式二：PhoenixUSBPro（与之前刷 TWRP 明文版相同）
# 或用自制 v20 镜像把 recovery 分区替换后整体刷入
```

## 关键设计

- **boot 参数**：`kernel_addr=0x40080000, ramdisk_addr=0x43000000, tags=0x40000100,
  page=2048, header v2`，与官方 recovery.img 逐字节一致 → u-boot 加载方式不变。
- **DTB**：官方 recovery.img 的 v2 dtb_size=0（dtb 由 u-boot 自行加载，dtbo 分区实际为空），
  编译产物同样不带 dtb 段，行为与官方一致。
- **data 明文**：fstab 的 /data 不带 fileencryption（配合已刷的 vendor 去 FBE 版，
  TWRP 可直接读明文）。
- **动态分区**：`PRODUCT_USE_DYNAMIC_PARTITIONS=true` + `lptools`，
  TWRP 可正确挂载 /dev/block/mapper/{system,vendor,product}。
- **MTP**：TWRP 10 原生走 configfs usb（A133 官方内核支持 f_mtp/ffs），
  编译版自带 MTP + adb，无需移植。

## 已知注意点

- recovery 分区 = 32MB。A133 内核 22.2MB + TWRP ramdisk 可能逼近/超过 32MB。
  若产物 recovery.img > 32MB，需要先把 recovery 分区从 32MB 扩到 64MB
  （改 MBR 里 recovery lenlo 0x10000→0x20000，从 cache 借空间），再做 v33 定制镜像。
  编译产物出来后可先量大小判断。
- 若编译报错，下载 `build-log` artifact 排查（最常见是 repo sync 网络超时，重跑一次即可；
  或 openjdk-8 安装失败，workflow 里已有 fallback 分支）。
