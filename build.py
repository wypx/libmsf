#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os

def DisplayGithub():
    print("\n")
    print("          Welcome to join me http://luotang.me                 ")
    print("          Welcome to join me https://github.com/wypx/libmsf    ")
    print("          Welcome to join me https://github.com/wypx/mobile    ")
    print("\n")

def DisplayLogo():
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

def MakeBuildDir(buildPath):
    if not os.path.exists(buildPath):
        os.makedirs(buildPath) 
        print("-- Dir \'" + buildPath + '\' has been created successfully.')
    else:
        print("-- Dir \'" + buildPath + '\' has already exist now.')

def MakeBuildLib(buildPath):
    os.chdir(buildPath)
    os.system("cmake ../../lib")
    os.system("make -j8")
    #os.system("make install")

def MakeBuildApp(buildPath):
    os.chdir(buildPath)
    os.system("cmake ../../app")
    os.system("make -j8")
    #os.system("make install")

def MakeBuildProto(buildPath):
    os.chdir(buildPath)
    os.system("protoc -I=./ --cpp_out=./ *.proto")
    
def MakeBuildClean(build_path):
    #os.removedirs(build_path)
    os.system("cd ..")
    os.system("rm -rvf build")

if __name__ == '__main__':
    DisplayGithub()
    # DisplayLogo()
    print("\n")
    print("\n******************* Micro Service Framework Build Starting **************************\n")
    BuildRoot = os.getcwd()
    MakeBuildDir("build/lib")
    MakeBuildDir("build/app")
    MakeBuildLib(BuildRoot + "/build/lib")
    MakeBuildApp(BuildRoot + "/build/app")
    # MakeBuildClean("build")
    print("\n******************* Micro Service Framework Build Ending ****************************\n")

