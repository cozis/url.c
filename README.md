# url.c
This is a small library to parse and manipulate URLs in conformance to RFC 3986 and WHATWG.

It features
* No allocations
* No dependencies
* The ability to switch between RFC 3986 and WHATWG with a flag
* Relative reference parsing and resolution
* URL normalization
* Doesn't rely on null-terminated strings

# Usage
You just copy paste `url.h` and `url.c` in your source tree and compile them as usual.

Some example programs are provided in the `examples/` folder. To compile them, run `build.bat` on Windows or `build.sh` on Linux.

# Testing
The parser and reference resolver is tested against the URL section of the [web platform tests](https://github.com/web-platform-tests/wpt). Note that not all tests pass. This is due to the parser not doing any normalization and not supporting more niche features such as converting `\` characters to `/` or supporting drive letters like `C:` in the URL's path.

To run the test suite, `cd` into `tests/` and compile the test runner using `build.sh` (Linux) or `build.bat` (Windows). Then, run `./test.out` (Linux) or `./test.exe` (Windows).
