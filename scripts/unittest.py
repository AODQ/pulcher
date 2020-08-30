#!/usr/bin/python3

# runs software for all platforms

if __name__ == "__main__":
  import sys, os

  if (not os.path.exists(sys.argv[1])):
    print("error: install directory does not yet exist, use ./configure.py")

  strInstallWin64 = sys.argv[1] + "/install-release-win64"
  strInstallWin32 = sys.argv[1] + "/install-release-win32"
  strInstallLinux = sys.argv[1] + "/install-release-linux"

  strBuildWin64 = sys.argv[1] + "/build-release-win64"
  strBuildWin32 = sys.argv[1] + "/build-release-win32"
  strBuildLinux = sys.argv[1] + "/build-release-linux"

  strNinjaWin64 = "ninja -C " + strBuildWin64 + " install"
  strNinjaWin32 = "ninja -C " + strBuildWin32 + " install"
  strNinjaLinux = "ninja -C " + strBuildLinux + " install"

  for i in [strInstallWin64, strInstallWin32, strInstallLinux]:
    if (not os.path.exists(i)):
      print(
        "Error: install directory '"+i+" does not yet exist, use ./configure.py"
      )
      exit()

  strLaunchLinux = (
    "cd " + strInstallLinux + "/bin/ ;"
  + "xterm "
    + " -geometry 80x20+20+30 "
    + " -T \"floating (pulcher linux)\" "
    + " -e '"
      + " export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:"
        + os.getcwd()+"/"+strInstallLinux+"/lib/"
      + " ; ./pulcher-client --resolution 640x480"+" ; "
      + "echo \"exitting . . .\"; pwd;"
      + "sleep 1.5"
    + "' 2> /dev/null &"
  )

  strLaunchWin64 = (
    "cd " + strInstallWin64 + "/bin/ ;"
  + "xterm "
      + " -geometry 80x20+500+30 "
      + " -T \"floating (pulcher win64)\" "
      + " -e '"
        + "./pulcher-client.exe --resolution 640x480"+"; "
        + "echo \"exitting . . .\"; pwd;"
        + "sleep 1.5"
      + "' 2> /dev/null &"
  )

  os.system(strNinjaLinux)
  os.system(strNinjaWin64)
  #os.system(strNinjaWin32)

  print()
  print()
  print()

  print("----------- linux -----------")
  print(strLaunchLinux)
  os.system(strLaunchLinux)

  print()
  print()
  print()

  print("----------- win64 -----------")
  print(strLaunchWin64)
  os.system(strLaunchWin64)

  print()
