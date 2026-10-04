set pagination off
set confirm off
target remote :1234
maintenance packet qqemu.sstep=0x1
break mark_start
continue
continue
delete
python
import gdb
def pc(): return int(gdb.parse_and_eval("$pc")) & ~1
woke = int(gdb.parse_and_eval("(unsigned)&mark_woke")) & ~1
end_ = int(gdb.parse_and_eval("(unsigned)&mark_end")) & ~1
n = 0; nw = None
while True:
    gdb.execute("stepi", to_string=True); n += 1
    p = pc()
    if p == woke and nw is None: nw = n
    if p == end_: break
    if n > 60000: break
print("RESULT wake_insns=%s roundtrip_insns=%d" % (nw, n))
end
kill
quit
