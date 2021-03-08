# crashreport
crashreport is modified based on [crashrpt](http://crashrpt.sourceforge.net/).

Removed UI and network transport part. For now, only the crash information collecting remains.


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

