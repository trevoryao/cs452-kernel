# Kernel Assignment 3
Developed by Niclas Heun (nheun) and Trevor J. Yao (t27yao).

## Compilation
Build the kernel from the root directory by running one of `make k3`
It may be necessary to set the `XDIR` variable to point to the directory containing the cross-compiler.
For example, in the `linux.student.cs` environment, `XDIR` should be `/u/cs452/public/xdev` (i.e. before the `bin` directory). Therefore, the program can be compiled in the `linux.student.cs` environment by changing directory to the program root and executing:
```
make k3 XDIR=/u/cs452/public/xdev
```
Backup binaries are located in the /bin directory.

## Execution
As usual. That is, upload to the TFTP server to the desired RPi, and power on the corresponding RPi.

## Program Operation
Runs itself. The Kernel starts up and executes the user-level entrypoint function `void user_main(void)`. The User level programs required for this assignment are found in `user/includes/k3.h`,
and the entrypoint executes the first program.
