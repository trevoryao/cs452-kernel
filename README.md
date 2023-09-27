# CS 452 -- Kernel
Developed by Niclas Heun (nheun) and Trevor J. Yao (t27yao).

## Documentation
Documentation for compiling and running each Kernel Assignment (i.e. `K1`, `K2`, ...) can be
found in `docs/k<1, 2, 3, 4>`. Compilation instructions are the same for all, and are reproduced below.
Kernel Assignment READMEs are also found there, and mostly reproduce this document.

## Compilation
Build the program from the root directory by running one of `make`, `make all`, or `make kernel`.
The prebuilt image is located in the repo root, titled `kernel.img`.
It may be necessary to set the `XDIR` variable to point to the directory containing the cross-compiler.
For example, in the `linux.student.cs` environment, `XDIR` should be `/u/cs452/public/xdev` (i.e. before the `bin` directory). Therefore, the program can be compiled in the `linux.student.cs` environment by changing directory to the program root and executing:
```
make XDIR=/u/cs452/public/xdev
```

## Execution
As usual. That is, upload to the TFTP server to the desired RPi, and power on the corresponding RPi.
