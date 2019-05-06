#plan of attack:

#read data.csv as list of points
#create 8 or so blank lists for 1MB sparse, 1MB dense, 4MB sparse, etc
#iterate through list and add each data point into its proper list
data = open("data.csv", "r")

sparse1 = []
dense1 = []
sparse4 = []
dense4 = []
sparse16 = []
dense16 = []
sparse32 = []
dense32 = []

for l in data:
    line = l.split(",")
    if line[2] == '0.25':
        if line[3] == "1000000":
            sparse1.append(line)
        elif line[3] == "4000000":
            sparse4.append(line)
        elif line[3] == "16000000":
            sparse16.append(line)
        elif line[3] == "32000000":
            sparse32.append(line)
    elif line[2] == '0.9':
        if line[3] == "1000000":
            dense1.append(line)
        elif line[3] == "4000000":
            dense4.append(line)
        elif line[3] == "16000000":
            dense16.append(line)
        elif line[3] == "32000000":
            dense32.append(line)

print("File Size,Benchmark,OTF with CBC,OTF with EBC,Full Disk ECB,Full Disk ECB and OTF CBC,Full Disk and OTF CBC")
print("256,", end='')
for x in dense1:
    if x[1] == '256':
        print(x[4][0:-1], end=',')
print("\n1024,", end='')
for x in dense1:
    if x[1] == '1024':
        print(x[4][0:-1], end=',')
print("\n8192,", end='')
for x in dense1:
    if x[1] == '8192':
        print(x[4][0:-1], end=',')
print("\n32768,", end='')
for x in dense1:
    if x[1] == '32768':
        print(x[4][0:-1], end=',')

#then, for each of our 8 lists, go through and print them in a chart friendly format
