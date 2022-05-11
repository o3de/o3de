import sys
import shutil
from pathlib import Path

fullPath = sys.argv[1]

path = Path(fullPath)
fileName = path.stem
pathToFile = path.parent.resolve()
extension = path.suffix

fileNameBackup = fileName + "_bckup"
fullBackupPath = (path.parent).joinpath(fileNameBackup).with_suffix(extension)

shutil.copy(fullPath, fullBackupPath)

print("Backup saved to \'%s\'" % fullBackupPath)

# Read in the file
with open(fullPath, 'r') as file :
  filedata = file.read()

# Replace the target string
filedata = filedata.replace('"$type": "SpawnTicketInstance"', '"$type": "AzFramework::EntitySpawnTicket"')
filedata = filedata.replace('"$type": "SpawnableAsset"', '"$type": "AzFramework::Scripts::SpawnableScriptAssetRef"')
filedata = filedata.replace('\"Asset\"', '\"asset\"')
filedata = filedata.replace('{2B5EB938-8962-4A43-A97B-112F398C604B}', '{BA62FF9A-A01E-4FEB-84C6-200881DF2B2B}')

# Write the file out again
with open(fullPath, 'w') as file:
  file.write(filedata)

print("Asset successfully updated")