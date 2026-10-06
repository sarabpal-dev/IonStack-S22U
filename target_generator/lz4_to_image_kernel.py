from pathlib import Path
import lz4.frame
import struct

compressed = Path("boot.img.lz4").read_bytes()
boot = lz4.frame.decompress(compressed)
Path("boot.img").write_bytes(boot)

kernel_size = struct.unpack_from("<I", boot, 0x08)[0]
Path("Image").write_bytes(boot[0x1000:0x1000 + kernel_size])