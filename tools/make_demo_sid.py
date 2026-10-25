"""Generate a small test PSID file (C major scale) to check the player.

Usage: python3 tools/make_demo_sid.py data/demo_gama.sid
"""
import struct,sys
code = bytes.fromhex(
 "A90F8D18D4" "A9098D05D4" "A9A08D06D4" "A9088D03D4" "A9008510" "8511" "60"   # init $1000
 "C610" "1022" "A90C" "8510" "A940" "8D04D4" "A611" "BD4210" "8D00D4" "BD4A10" "8D01D4"
 "A941" "8D04D4" "E8" "8A" "2907" "8511" "60")
assert len(code)==0x42, hex(len(code))
notes=[0x1167,0x1389,0x15ED,0x173B,0x1A14,0x1D45,0x20DB,0x22CF]
code += bytes(n&0xff for n in notes)+bytes(n>>8 for n in notes)
hdr = b"PSID"+struct.pack(">HHHHHHHI",2,0x7C,0x1000,0x1000,0x101B,1,1,0)
def s(t): return t.encode().ljust(32,b"\0")
hdr += s("Test scale")+s("Claude")+s("2026")+struct.pack(">HBBH",0x14,0,0,0)
assert len(hdr)==0x7C
open(sys.argv[1],"wb").write(hdr+code)
