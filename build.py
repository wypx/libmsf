#!/usr/bin/python3

import os

def MSF_BUILD_SHOW():
    print("\n")
    print("          Welcome to join me http://luotang.me                 ")
    print("          Welcome to join me https://github.com/wypx/libmsf    ")
    print("\n")

def MSF_BUILD_LOGO():
    print("                       .::::.                                  ")
    print("                     .::::::::.                                ")
    print("                    :::::::::::                                ")
    print("                 ..:::::::::::'                                ")
    print("               '::::::::::::'                                  ")
    print("                .::::::::::                                    ")
    print("           '::::::::::::::..                                   ")
    print("                ..::::::::::::.                                ")
    print("              ``::::::::::::::::                               ")
    print("               ::::``:::::::::'        .:::.                   ")
    print("              ::::'   ':::::'       .::::::::.                 ")
    print("            .::::'      ::::     .:::::::'::::.                ")
    print("           .:::'       :::::  .:::::::::' ':::::.              ")
    print("          .::'        :::::.:::::::::'      ':::::.            ")
    print("         .::'         ::::::::::::::'         ``::::.          ")
    print("     ...:::           ::::::::::::'              ``::.         ")
    print("    ```` ':.          ':::::::::'                  ::::..      ")
    print("                       '.:::::'                    ':'````..   ")

def MSF_MKDIR_BUILD(build_path):
    bDirExist = os.path.exists(build_path)
    if not bDirExist:
        os.makedirs(build_path) 
        print("-- Dir \'" + build_path + '\' has been created successfully.')
    else:
        print("-- Dir \'" + build_path + '\' has already exist now.')

def MSF_BUILD_LIBRARY(build_path):
    os.chdir(build_path)
    os.system("cmake ../../lib")
    os.system("make -j8")
    #os.system("make install")

def MSF_BUILD_APPLICATION(build_path):
    os.chdir(build_path)
    os.system("cmake ../../app")
    os.system("make -j8")
    #os.system("make install")

def MSF_BUILD_PROTOBUF(build_path):
    os.chdir(build_path)
    os.system("protoc -I=./ --cpp_out=./ *.proto")
    

def MSF_BUILD_CLEAN(build_path):
    #os.removedirs(build_path)
    os.system("cd ..")
    os.system("rm -rvf build")

if __name__ == '__main__':
    MSF_BUILD_SHOW()
    # MSF_BUILD_LOGO()
    print("\n")
    print("\n******************* Micro Service Framework Build Starting **************************\n")
    MSF_BUDILD_ROOT = os.getcwd()
    MSF_MKDIR_BUILD("build/lib")
    MSF_MKDIR_BUILD("build/app")
    MSF_BUILD_LIBRARY(MSF_BUDILD_ROOT + "/build/lib")
    MSF_BUILD_APPLICATION(MSF_BUDILD_ROOT + "/build/app")
    # MSF_BUILD_CLEAN("build")
    print("\n******************* Micro Service Framework Build Ending ****************************\n")

