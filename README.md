# crashreport
crashreport is modified based on [crashrpt](http://crashrpt.sourceforge.net/).

Update:
- Removed UI and network transport part, only the crash information collecting remains.
- Only support unicode charset.
- Support high dpi screenshot(such as 4k screen).
- Support installing with [vcpkg](https://github.com/microsoft/vcpkg).

TODO:
1. Using Qt to redesign UI.

# Install with [vcpkg](https://github.com/microsoft/vcpkg)
1. Copy `vcpkg_port\crashreport` folder to `<vcpkg>\ports` folder.
2. Using `vcpkg` command to install.
```bat
# Shared library
vcpkg install crashreport:x86-windows

# Static library
vcpkg install crashreport:x86-windows-static
```

