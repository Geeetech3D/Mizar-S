import os
filenames = []
for filename in os.listdir('.\\bin'):
    filenames.append('\"' + filename +'\",')
filenamesFile = open('.\\list.txt','w')
for str in filenames:
    filenamesFile.write(str + '\n')
filenamesFile.close()
