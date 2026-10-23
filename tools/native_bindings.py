"""Generate runtime binding metadata from arbitrary compiled Switch scripts."""
import argparse,hashlib,json,struct,zlib
from pathlib import Path
BUILTINS={
    0x4d45474153415645,0x4d4547414c4f4144,0x5348564e58535631,0x5348564e584c4431,0x5348564e58415631,
    0x5348564e58504331,0x5348564e58504e31,0x5348564e58504931,0x5348564e58505331,0x5348564e58505431,
    0x5348564e58505447,0x5348564e5850524c,
    0x5348564e58504c31,0x5348564e58505031,0x5348564e58505032,0x5348564e58504531,0x5348564e58505631,
    0x5348564e58504b31,0x5348564e58505231,0x5348564e58504d31
}
def record(path,hashes):
    data=path.read_bytes()
    if data[:4]==b'RSC7':
        for window in (-15,15):
            try:data=zlib.decompress(data[16:],window);break
            except zlib.error:continue
        else:raise ValueError('Cannot decode resource: '+str(path))
    if len(data)<0x78:raise ValueError('Not a Switch script')
    u32=lambda at:struct.unpack_from('<I',data,at)[0]
    pointer=lambda at:struct.unpack_from('<Q',data,at)[0]&0xffffff
    code,count,name=u32(0x1c),u32(0x2c),u32(0x58)
    table,blocks=pointer(0x40),pointer(0x10)
    if not name or code<32 or count>65535 or blocks+8>len(data) or table+8*count>len(data):raise ValueError('Invalid script layout')
    first=pointer(blocks)
    if first+32>len(data):raise ValueError('Missing entry code')
    entries=[]
    for i in range(count):
        stored=struct.unpack_from('<Q',data,table+8*i)[0];r=(code+i)&63
        hash=((stored<<r)|(stored>>((-r)&63)))&((1<<64)-1)
        if hash in hashes:entries.append((i,hash))
    if not entries:return b'',dict(script=str(path),custom_entries=0)
    if len(entries)>256:raise ValueError('Too many custom entries')
    blob=struct.pack('<4I',name,code,count,len(entries))+data[first:first+32]+b''.join(struct.pack('<IIQ',i,0,h) for i,h in entries)
    return blob,dict(script=str(path),sha256=hashlib.sha256(path.read_bytes()).hexdigest(),name_hash=hex(name),custom_entries=len(entries))
def generate(scripts,output,extra=()):
    hashes=BUILTINS|set(extra);body=b'';records=[];names=set()
    for path in scripts:
        blob,proof=record(path,hashes)
        if blob:
            name=proof['name_hash']
            if name in names:raise ValueError('Duplicate script name: '+name)
            names.add(name);body+=blob;records.append(proof)
    if len(records)>64 or len(body)+16>65536:raise ValueError('Manifest exceeds runtime bounds')
    output.parent.mkdir(parents=True,exist_ok=True)
    output.write_bytes(struct.pack('<4I',0x424e4853,1,len(records),zlib.crc32(body))+body)
    return dict(records=records,bytes=output.stat().st_size,sha256=hashlib.sha256(output.read_bytes()).hexdigest())
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--script',type=Path,nargs='+',required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--hash',action='append',default=[],type=lambda x:int(x,0));a=p.parse_args();print(json.dumps(generate(a.script,a.output,a.hash),indent=2))
