single-header memory scanning library for linux and unix flavors that have
`/proc/$PID/maps` and `/proc/$PID/mem`

I wrote this specifically for WIP my osu! real-time data provider but it
might turn into a more complete memory library at some point

see the top of mfao.c for documentation or read the examples

# usage
you can build the dynamic library and install it:

```
./libbuild.sh
sudo cp ./libmfao.so /usr/lib/
sudo cp ./mfao.c /usr/include/
```

write a simple program...

```
#include "mfao.c"

int main(int argc, char* argv[]) {
  mfao_t m = mfao_new();
  mfao_set_process_name(m, "some_process");
  printf("%d\n", mfao_read_int32(m, 0x401000));
  return 0;
}
```

compile and run it (requires root):

```
gcc example.c -lmfao -o example && sudo ./examples
```

you can also do a single-header build without having to build and install
the dynamic library by simply defining `MFAO_IMPLEMENTATION`

```
#define MFAO_IMPLEMENTATION
#include "mfao.c"

/* ... */
```

```
gcc example.c -o example
```
