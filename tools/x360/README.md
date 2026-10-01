# tools/x360: reading the Xbox 360 disc

Host tools for looking at Plants vs. Zombies on Xbox 360 (the PopCap
Arcade disc, title 584109FF). Nothing here goes into the build; they were
used to compare the 360 edition's art with the TV edition's (the TV edition
ships the 360's button pictures, which the port relabels on the console:
source/pvz_english.c, x360_buttons).

1. `X360_ISO=<disc.iso> python3 xdvdfs.py` lists the disc;
   `xdvdfs.py x <path> <dir>` extracts a file, e.g. the game package
   `Content/0000000000000000/584109FF/000D0000/281104A67F7C961E2736A5F6F524D43FC81EB79158`.
2. `python3 stfs.py <package>` lists it (a PIRS package); `stfs.py <package> x <dir>`
   extracts main.pak, en.pak and the rest.
3. `cc -O2 -o lzx lzx.c && ./lzx main.pak seg 17 main_dec.pak`: main.pak is
   XCompress (0x0FF512ED): a table of uncompressed sizes, then one LZX stream
   (128 KB window) per 0x20000-byte block, or a block stored as it is.
4. `python3 pak360.py main_dec.pak <dir>` (and `en.pak`): the PopCap pak, each
   file's data after a 16-bit padding length.
5. `python3 ptx2png.py --all <dir> <outdir>`: the .ptx textures (DXT5 in linear
   rows, 16-bit big-endian words, a 16-byte trailer: width, height, pitch,
   format) as PNG.
