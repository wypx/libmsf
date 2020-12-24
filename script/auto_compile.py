#!/usr/bin/python3
# -*- coding: utf-8 -*-

import os, time, paramiko

def SendSSHCommand(command):
  try:
    if not os.path.exists(os.path.expanduser("~/.ssh/id_rsa")) or not os.path.exists(os.path.expanduser("~/.ssh/id_rsa.pub")):
      # 在客户端生成ssh密钥,设置好参数，这样就不会中途要求输入
      if os.system("ssh-keygen -t rsa -P '' -f ~/.ssh/id_rsa"):
        return False

    # 建立一个sshclient对象
    ssh = paramiko.SSHClient()
    # 允许将信任的主机自动加入到host_allow 列表，此方法必须放在connect方法的前面
    ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    # 指定本地的RSA私钥文件,如果建立密钥对时设置的有密码，password为设定的密码，如无不用指定password参数
    # pkey = paramiko.RSAKey.from_private_key_file('/home/super/.ssh/id_rsa', password='12345')
    # pkey = paramiko.RSAKey.from_private_key_file("~/.ssh/id_rsa")
    # 建立连接
    # ssh.connect(hostname="192.168.150.123",
    #             port=22,
    #             username="tang.luo",
    #             pkey=pkey)
    ssh.connect(hostname="192.168.150.123",
                port=22,
                username="tang.luo",
                password="tang")
    # 开启ssh管道
    # sess = ssh.get_transport().open_session()
    # sess.get_pty()
    # sess.invoke_shell()
    # #执行指令

    cmdList = ['df -h\n', 'pwd\n']
    # for cmd in cmdList:
    #   sess.sendall(cmd)
    #   time.sleep(0.5)
    #   result = sess.recv(102400)
    #   result = result.decode(encoding='UTF-8',errors='strict').strip()

    # for cmd in cmdList:
    #   stdin, stdout, stderr = ssh.exec_command(cmd)
    #   result = stdout.read()
    #   result = result.decode(encoding='UTF-8', errors='strict').strip()
    #   print (result)
  
    # 执行命令
    stdin, stdout, stderr = ssh.exec_command("ls -l")
    # 结果放到stdout中，如果有错误将放到stderr中
    print(stdout.read().decode().strip())

    EnterDockerCMD = "docker exec -it compile.tang.luo.new scl enable devtoolset-8 bash"
    # 执行命令
    stdin, stdout, stderr = ssh.exec_command(EnterDockerCMD)
    # 结果放到stdout中，如果有错误将放到stderr中
    print(stdout.read().decode().strip())

    # 执行命令
    stdin, stdout, stderr = ssh.exec_command("ls -l")
    # 结果放到stdout中，如果有错误将放到stderr中
    print(stdout.read().decode().strip())


    # 关闭连接
    # sess.close()
    ssh.close()
  except Exception as e:
    log.exception(e)

def SendSSHCommand2(command):
  try:
    if not os.path.exists(os.path.expanduser("~/.ssh/id_rsa")) or \
       not os.path.exists(os.path.expanduser("~/.ssh/id_rsa.pub")):
      # 在客户端生成ssh密钥,设置好参数，这样就不会中途要求输入
      if os.system("ssh-keygen -t rsa -P '' -f ~/.ssh/id_rsa"):
        return False
    # 使用用户名，密码建立ssh链接
    transport = paramiko.Transport(("192.168.150.123", 22))
    try:
      transport.connect(username="tang.luo", password="tang")
    except Exception:
      return False

    ssh = paramiko.SSHClient()
    ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    ssh._transport = transport  # 将sftp和ssh一同建立
    sftp = paramiko.SFTPClient.from_transport(transport)
 
    # 上传公钥
    # sftp.put(os.path.expanduser("~/.ssh/id_rsa.pub"), "/tang.luo/.ssh/filebackkey.pub")
 
    # 添加公钥 这里根据实际情况进行修改，设置成用户名下的.ssh
    # stdin, stdout, stderr = ssh.exec_command(
    #     "cat /root/.ssh/filebackkey.pub >> /root/.ssh/authorized_keys") 
    # if stderr.channel.recv_exit_status():
    #   ssh.close()
    #   return False

    # 这里根据实际情况进行修改，设置成用户名下的.ssh
    # stdin, stdout, stderr = ssh.exec_command("chmod 600 /root/.ssh/authorized_keys")
    # if stderr.channel.recv_exit_status():
    #   ssh.close()
    #   return False
    # ubuntu 重启ssh服务
    # stdin, stdout, stderr = ssh.exec_command("service sshd restart")
    # if stderr.channel.recv_exit_status():
    #   ssh.close()
    #   return False

    # 执行命令
    stdin, stdout, stderr = ssh.exec_command(command)
    # 结果放到stdout中，如果有错误将放到stderr中
    print(stdout.read().decode().strip())
    # print(stderr.read())

    transport.close()


  except Exception as e:
    log.exception(e)

if __name__ == "__main__":
  print("Auto compile %s" % "metaserver")
  SendSSHCommand("pwd")
  SendSSHCommand2("pwd")