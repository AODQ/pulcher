#!/usr/bin/python3

# builds project for multiple platforms

if __name__ == "__main__":
  import sys, os
  if (len(sys.argv) == 1):
    print("Error: must provide location of where build should occur")
    exit()

  if (not os.path.exists(sys.argv[1])):
    print("Error: build directory does not yet exist, use ./configure.py")
    exit()

  strInstallWin64 = sys.argv[1] + "/install-release-win64"
  strInstallWin32 = sys.argv[1] + "/install-release-win32"

  strBuildWin64 = sys.argv[1] + "/build-release-win64"
  strBuildWin32 = sys.argv[1] + "/build-release-win32"
  strBuildLinux = sys.argv[1] + "/build-release-linux"

  strNinjaWin64 = "ninja -C " + strBuildWin64 + " install"
  strNinjaWin32 = "ninja -C " + strBuildWin32 + " install"
  strNinjaLinux = "ninja -C " + strBuildLinux + " install"

  for i in [strBuildWin64, strBuildWin32, strBuildLinux]:
    if (not os.path.exists(i)):
      print(
        "Error: build directory '"+i+" does not yet exist, use ./configure.py"
      )
      exit()

  print()
  print()
  print()
  print("---------------- windows (64-bit) ----------------")
  print(strNinjaWin64)
  os.system(strNinjaWin64)
  os.system(
    "cp -r /usr/x86_64-w64-mingw32/bin/*.dll " + strInstallWin64+"/bin/."
  )

  #print("---------------- windows (32-bit) ----------------")
  #print(strNinjaWin32)
  #os.system(strNinjaWin32)
  #os.system(
  #  "cp -r /usr/x86_64-w64-mingw32/bin/*.dll " + strInstallWin32+"/bin/."
  #)

  print()
  print()
  print()
  print("---------------- linux            ----------------")
  print(strNinjaLinux)
  os.system(strNinjaLinux)
