import mmap,struct,sys
import numpy as np
path=sys.argv[1]; targets=[int(x,16) for x in sys.argv[2:]]
with open(path,'rb') as f:
 d=mmap.mmap(f.fileno(),0,access=mmap.ACCESS_READ)
 # executable segment values for these builds
 eh=struct.unpack_from('<16sHHIQQQIHHHHHH',d,0); phoff=eh[5]; phentsize=eh[9]; phnum=eh[10]
 for i in range(phnum):
  p=struct.unpack_from('<IIQQQQQQ',d,phoff+i*phentsize)
  if p[0]==1 and p[1]&1: file0,vma,size=p[2],p[3],p[5]; break
 buf=memoryview(d)[file0:file0+size]
 print(f'file={file0:x} vma={vma:x} size={size:x}')
 for target in targets:
  print(f'target {target:x}')
  for off in range(4):
   a=np.frombuffer(buf[off:len(buf)-((len(buf)-off)%4)],dtype='<i4')
   pos=file0+off+np.arange(a.size,dtype=np.int64)*4
   endv=vma+(pos-file0)+4
   hits=np.nonzero(a.astype(np.int64)==target-endv)[0]
   for h in hits[:30]:
    fp=int(pos[h]); print(hex(fp),hex(vma+fp-file0-8),bytes(d[fp-8:fp+8]).hex())
 del buf
 d.close()
