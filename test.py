from subprocess import call
from datetime import datetime
import time
import sys
fsize = [256, 1024, 8192, 32768]
full = [.25, .9]
disksize = [1000000, 4000000, 16000000, 32000000]
size = 0
print("test,file size, full amount, disksize, seconds")

for x in fsize:
    for y in full:
        count = (int)((disksize[size] * y)/x) #how many files we'll use

        call(["rm", "benchmark"])
        call(["rm", "otfcbc"])
        call(["rm", "otfecb"])
        call(["rm", "ondisk"])
        call(["rm", "ondisk2"])
        call(["rm", "diskcbc"])
        call(["rm", "diskecb"])

        #run benchmark, just add then remove that many files
        t = time.time()
        for i in range(0, count):
            rc = call(["timeout", "30m", "./filefs", "-a", "/home/dale/CryptoProject/"+str(x)+"/"+str(i), "-f", "benchmark"])
        print("benchmark,"+str(x)+","+str(y)+","+str(disksize[size])+","+str(time.time() - t))


        #run just OTF with CBC
        t = time.time()
        for i in range(0, count):
            rc = call(["timeout", "30m", "./filefs", "-a", "/home/dale/CryptoProject/"+str(x)+"/"+str(i), "-o", "-m", "-f", "otfcbc"])
        print("otf cbc,"+str(x)+","+str(y)+","+str(disksize[size])+","+str(time.time() - t))


        #run just OTF with EBC
        t = time.time()
        for i in range(0, count):
            rc = call(["timeout", "30m", "./filefs", "-a", "/home/dale/CryptoProject/"+str(x)+"/"+str(i), "-o", "-f", "otfecb"])
        print("otf ecb,"+str(x)+","+str(y)+","+str(disksize[size])+","+str(time.time() - t))

        #run just full disk with CBC
        t = time.time()
        rc = call(["timeout", "30m", "./filefs", "-f", "diskcbc"])
        for i in range(0, count):
            rc = call(["timeout", "30m", "./filefs", "-a", "/home/dale/CryptoProject/"+str(x)+"/"+str(i), "-f", "diskcbc"])
        rc = call(["timeout", "30m", "./filefs", "-u", "ondisk", "-f", "diskcbc"])
        print("full disk cbc,"+str(x)+","+str(y)+","+str(disksize[size])+","+str(time.time() - t))

        #run OTF and full disk with CBC
        t = time.time()
        rc = call(["timeout", "30m", "./filefs", "-f", "diskcbc"])
        for i in range(0, count):
            rc = call(["timeout", "30m", "./filefs", "-a", "/home/dale/CryptoProject/"+str(x)+"/"+str(i), "-o", "-f", "diskcbc"])
        rc = call(["timeout", "30m", "./filefs", "-u", "ondisk", "-f", "diskcbc"])
        print("combo cbc,"+str(x)+","+str(y)+","+str(disksize[size])+","+str(time.time() - t))

        #run OTF and full disk with EBC (probably will fail)
        t = time.time()
        rc = call(["timeout", "30m", "./filefs", "-f", "diskcbc"])
        for i in range(0, count):
            rc = call(["timeout", "30m", "./filefs", "-a", "/home/dale/CryptoProject/"+str(x)+"/"+str(i), "-o", "-f", "diskecb"])
        rc = call(["timeout", "30m", "./filefs", "-u", "ondisk2", "-m", "-f", "diskecb"])
        print("combo ebc,"+str(x)+","+str(y)+","+str(disksize[size])+","+str(time.time() - t))
