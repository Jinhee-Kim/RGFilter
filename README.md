# RGFilter

Registry activity monitor driver

## Installation

```cmd
sc create "RGFilter"
  type= kernel
  start= boot
  group= "Boot Bus Extender"
  binpath= "%SystemRoot%\system32\drivers\RGFilter.sys"
```

## Language

- c++ - Kernel Mode Driver

## Environment

- Windows

## Author

- Genie (hee38407@gmail.com)
