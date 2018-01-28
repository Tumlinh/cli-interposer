# CLI interposer
If you are working on a public machine or a machine supervised by people you don't trust, you might want to hide your activities from them.  
This project takes advantage of the LD_PRELOAD trick to hide program arguments from tools such as ps and top.  
This technique doesn't require root privileges nor recompiling anything.  
Works for any dynamically linked ELF executable using libc

---

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