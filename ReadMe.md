# [LVGL](https://github.com/lvgl/lvgl) on UEFI Environment.

## Build

```
build -p LvglPkg\LvglPkg.dsc -a X64 -t VS2015x86 -b RELEASE
```

## LvglLib Usage

1. Include LvglLib like other UEFI Library Class in your UEFI_APPLICATION
2. In App ENTRY_POINT:
   - Call `UefiLvglInit()` to do init
   - Call `UefiLvglAppRegister (MyApp)` to show `MyApp`
   - Call `UefiLvglDeinit()` to do deinit, and you may need another code to do clear up for `MyApp`

## Demo

- [LvglDemoSetup.efi](./Demo/Bin/LvglDemoSetup.efi)
  ![Setup1](./Demo/Images/Setup1.png)
  ![Setup2](./Demo/Images/Setup2.png)

- [LvglDemoApp.efi(New, just show UEFI logo)](./Demo/Bin/LvglDemoApp.efi)
  ![LvglDemoApp](./Demo/Images/LvglDemoApp.png)

- [LvglApp.efi(Old)](./Demo/Bin/LvglApp.efi)
  ![LvglApp](./Demo/Images/Demo.png)

### Demo Usage

1. Download [OVMF.fd](./Demo/Bin/OVMF.fd)
2. Create EfiFiles folder and copy Demo efi binary to it
3. qemu-system-x86_64.exe -bios OVMF.fd -hda fat:rw:EfiFiles -net none -usb -device usb-mouse -serial file:./log.txt
4. Boot to UEFI Shell
5. `fs0:` then [Enter], `DemoName.efi` then [Enter]
6. Press `Esc` to exit Demo

## TODO
- [x] Absolute Pointer Mouse
- [x] Simple Pointer Mouse
- [ ] Mouse Wheel
- [x] Log/Debug Print
- [ ] File System
- [ ] VS2022/GCC/Clang
- [ ] Code Clean(Remove Unused Source File in .inf)
- [ ] ...

## Note
1. For Edk2 EmulatorPkg user, use the RELEASE build or build LvglPkg/Application/LvglDemoApp.inf in EmulatorPkg.dsc