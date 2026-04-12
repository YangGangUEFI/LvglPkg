# [LVGL](https://github.com/lvgl/lvgl) on UEFI Environment.

## Build

#### X64-MSVC
```
build -p LvglPkg\LvglPkg.dsc -a X64 -t VS2022 -b RELEASE
```

#### X64-GCC
```
. edksetup.sh
build -p LvglPkg/LvglPkg.dsc -a X64 -t GCC -b RELEASE
```

#### AARCH64-GCC
```
export GCC_AARCH64_PREFIX=aarch64-none-linux-gnu-
. edksetup.sh
build -p LvglPkg/LvglPkg.dsc -a AARCH64 -t GCC -b RELEASE
```

## LvglLib Usage

1. Include LvglLib like other UEFI Library Class in your UEFI_APPLICATION
2. In App ENTRY_POINT:
   - ~~Call `UefiLvglInit()` to do init~~
   - Call `UefiLvglAppRegister (MyApp)` to show `MyApp`
   - ~~Call `UefiLvglDeinit()` to do deinit, and you may need another code to do clear up for `MyApp`~~

## Demo

- [UefiDashboard.efi](./Demo/Bin/UefiDashboard.efi)
  ![UefiDashboard1](./Demo/Images/UefiDashboard/UefiDashboard1.png)
  ![UefiDashboard2](./Demo/Images/UefiDashboard/UefiDashboard2.png)
  ![UefiDashboard3](./Demo/Images/UefiDashboard/UefiDashboard3.png)

### Demo Usage

1. Download [OVMF.fd](./Demo/Bin/OVMF.fd)
2. Create EfiFiles folder and copy `UefiDashboard.efi` binary to it
3. qemu-system-x86_64 -bios OVMF.fd -hda fat:rw:EfiFiles -net none -device qemu-xhci,id=xhci -device usb-kbd,bus=xhci.0 -device usb-mouse,bus=xhci.0 -serial stdio
4. Boot to UEFI Shell
5. `fs0:` then [Enter], `UefiDashboard.efi` then [Enter]
6. Press `Esc` to exit `UefiDashboard`

## TODO
- [x] Absolute Pointer Mouse
- [x] Simple Pointer Mouse
- [x] Mouse Wheel
- [x] Log/Debug Print
- [ ] File System
- [ ] ~~VS2022~~/~~AARCH64-GCC~~/Clang
- [ ] Code Clean(Remove Unused Source File in .inf)
- [ ] ...

## Note
1. For Edk2 EmulatorPkg user, use the RELEASE build or build LvglPkg/Application/LvglDemoApp.inf in EmulatorPkg.dsc
