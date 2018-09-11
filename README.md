# CLI interposer
If you are using a public machine or a machine supervised by people you don't trust, you might want to hide your activities from them.  
This project takes advantage of the LD_PRELOAD trick to **hide program arguments** from tools such as `ps` and `top`.  
This technique **doesn't require root privileges** nor recompiling anything.  
Works for any dynamically linked ELF executable using libc

---

### Results
This array shows the results of the interposer against common inspection tools.  
Legend: A (arguments hidden), N (process name changed)

ps |top          |htop|lsof|gnome-system-monitor|unhide         
:-:|:-----------:|:--:|:--:|:------------------:|:-------------:
AN |A (N partial)|AN  |A   |A                   |AN (undetected)

### How to
+ Compile source
  ```
  gcc -fPIC -shared -ldl -o interposer.so main.c
  ```
+ Execute a program with dummy arguments
   ```
   LD_PRELOAD=$PWD/interposer.so curl http://kissmyarsefreakingadmin.com
   ```
+ Pass the real arguments via stdin

### How this works
Most inspection tools get information about processes in `/proc`, a special filesystem handled by the kernel. More precisely, arguments are accessible through `/proc/*/cmdline` which directly points to a piece of memory containing the arguments of the running program (`**argv`).  
The hack consists in erasing this part of memory while passing an alternative `**argv` pointing to the actual arguments to `main()` . This way the kernel is left with worthless data while the substantial information resides elsewhere.  
This requires a means to process arguments before they are handed to `main()`, hence the use of LD_PRELOAD, which overrides `__libc_start_main()`, the function in charge of initialising the program, with a function that handles the operations of obfuscation before calling the original `__libc_start_main()`, which in turn calls `main()`.  
You can find more information about Linux program startup [here](http://dbp-consulting.com/tutorials/debugging/linuxProgramStartup.html).

Two modes are available:
+ **Diversion mode**: fake arguments are passed to the program, real arguments are subsequently fetched by the interposer through stdin or another interface
    ```
                               kernel
                         (admin/user watching)
                                  |
                                  v            program stack
    +-----+   (dummy) args    +-------+   \ /    +-------+
    | CLI |------------------>| argv  |----X---->|       |
    +-----+                   +-------+   / \    |       |
       |        real args     +-------+          |       |
       +--------------------->| argv2 |--------->|       |
                              +-------+          +-------+
    ```
+ **Legacy mode**: real arguments are passed to the program, copied then erased asap by the interposer to prevent eavesdropping. The copy is then passed to `main()`.  
**Warning**: This mode is very handy but less secure, since there is a brief moment when real arguments are visible.
    ```
                               kernel
                         (admin/user watching)
                                  |
                                  v            program stack
    +-----+     real args     +-------+   \ /    +-------+
    | CLI |------------------>| argv  |----X---->|       |
    +-----+                   +-------+   / \    |       |
                    copy to argv2 |              |       |
                      erase argv  v              |       |
                              +-------+          |       |
                              | argv2 |--------->|       |
                              +-------+          +-------+
    ```

In addition to hiding arguments, the interposer can change the process name, i.e. `argv[0]`.     Although this works against `ps`, `top` is able to find the basename of the image, which is updated on binary loading (cf. `/proc/*/stat`). The memory mapped to this file is read-only, but you can still work around by renaming the binary to a longer name, as basenames do not exceed 16 characters (cf. [`task_struct`](https://elixir.bootlin.com/linux/latest/source/include/linux/sched.h#L835)).