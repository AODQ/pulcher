#!/usr/bin/python3

# configures project for multi-build (this is not necessary if you are
#   compiling for just one platform, or even if are willing to build each
#   platform manually)

if __name__ == "__main__":
  import sys, os
  if (len(sys.argv) == 1):
    print("Error: must provide location of where build should be installed to")
    exit()

  if (os.path.exists(sys.argv[1])):
    print("Error: trying to build in a location that already exists")
    exit()

  os.mkdir(sys.argv[1])

  strBuildWin64 = sys.argv[1] + "/build-release-win64"
  strBuildWin32 = sys.argv[1] + "/build-release-win32"
  strBuildLinux = sys.argv[1] + "/build-release-linux"

  strInstallWin64 = sys.argv[1] + "/install-release-win64"
  strInstallWin32 = sys.argv[1] + "/install-release-win32"
  strInstallLinux = sys.argv[1] + "/install-release-linux"

  strCmakeWin64 = (
    "cmake -B " + strBuildWin64 + " -DCMAKE_INSTALL_PREFIX="+strInstallWin64
  + ' -G "Ninja" -DCMAKE_BUILD_TYPE=RelWithDebInfo -DPULCHER_PLATFORM=Win64 .'
  )
  strCmakeWin32 = (
    "cmake -B " + strBuildWin32 + " -DCMAKE_INSTALL_PREFIX="+strInstallWin32
  + ' -G "Ninja" -DCMAKE_BUILD_TYPE=RelWithDebInfo -DPULCHER_PLATFORM=Win32 .'
  )
  strCmakeLinux = (
    "cmake -B " + strBuildLinux + " -DCMAKE_INSTALL_PREFIX="+strInstallLinux
  + ' -G "Ninja" -DCMAKE_BUILD_TYPE=RelWithDebInfo -DPULCHER_PLATFORM=Linux .'
  + ' -DCMAKE_EXPORT_COMPILE_COMMANDS=1'
  )

  os.mkdir(strBuildWin64)
  os.mkdir(strBuildWin32)
  os.mkdir(strBuildLinux)

  os.mkdir(strInstallWin64)
  os.mkdir(strInstallWin32)
  os.mkdir(strInstallLinux)

  print()
  print()
  print()
  print("---------------- windows (64-bit) ----------------")
  print(strCmakeWin64)
  os.system(strCmakeWin64)

  # not yet supported
  #print("---------------- windows (32-bit) ----------------")
  #print(strCmakeWin32)

  print()
  print()
  print()
  print("---------------- linux            ----------------")
  print(strCmakeLinux)
  os.system(strCmakeLinux)

  print("--- aliasing linux build-release")
  deleteLinuxLink = "rm " + sys.argv[1] + "/../build-release"
  print(deleteLinuxLink)
  os.system(deleteLinuxLink)
  aliasLinux = (
    "ln -s " + os.getcwd() + "/" + strBuildLinux
  + " " + sys.argv[1] + "/../build-release"
  )
  print(aliasLinux)
  os.system(aliasLinux)

  # make
  os.system("./scripts/multi-build.py " + sys.argv[1])
