## 3.0.1 (2016-02-06)

Bugfixes:

- Added work-around for build for icecc.a using old CXXABI (#24)
- Fixed build with Qt 5.5
- Improved how docbook2man is searched for (PR #21)

Internal Changes:

- Added Doxygen support to CMake
- Modernized CMake code (FindIcecream.cmake, etc.)
- Modernized source code to use C++11 features (override, nullptr, auto)

## 3.0.0 (2015-02-08)

Features:

- Star View got a fresh look, improved Detailed Host View (https://github.com/icecc/icemon/pull/17)
- Added simple man page (https://github.com/icecc/icemon/pull/9)
- Starting a change log

Bugfixes:

- Fix RPATH issues (https://github.com/icecc/icemon/pull/18)
- Lots of other small bug fixes in the Icemon views

Removals:

- Dropped the "Pool View", which was never fully implemented

Internal Changes:

- Ported icemon to Qt5
- No longer depends on kdelibs
- Better separation of model/view throughout the source code
    - Enables possibility to write QtQuick-based views now
