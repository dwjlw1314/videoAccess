#!/bin/bash
#Beijing Chen Safety Technology Co Ltd
echo "This is a shell to Install VAS_Access..."
echo -e "\e[1;31mThis version is <1.4.7.Release> of the program\e[0m"

#The way to determine whether you have root
ROOT_UID=0
if [ "$UID" -eq "$ROOT_UID" ];then
    echo -e "\e[1;34m\nThe Current is ROOT Users\e[0m"
else
    echo -e "\e[1;34mThe User Switches After Re Executing the Script\e[0m"
    su -
    exit 0
fi

#Get current path
CurrentPath=$(cd "$(dirname "$0")"; pwd);

#Add the yum configuration information
yumPath="/etc/yum.repos.d/"
#这里的-d 参数判断$yumPath是否存在
if [ ! -d "$yumPath" ];then
    echo -e "\e[1;34mThe yum program is Not Installed!!\e[0m"
    exit 0
fi

#这里的-f参数判断$yumConfFile是否存在
yumConfFile="/etc/yum.repos.d/rhel6.repo"
if [ ! -f "$yumConfFile" ];then
	touch $yumConfFile
    echo "touch $yumConfFile file"
fi

#这里判断$mountPath是否存在
mountPath="/VAmnt/"
if [ ! -d "$mountPath" ];then
    mkdir "$mountPath"
    echo "mkdir $mountPath directory!!!"
fi

echo "" > $yumConfFile
record=$(grep -i "VAS_Access" $yumConfFile | wc -l)
if [ 0 -eq $record ];then
	rm -rf $yumPath/rhel-source.repo
    sed -i '1i\[VIDEOACCESS]\nname=VAS_Access\nbaseurl=file://'$mountPath'Server\nenabled=1\ngpgcheck=1\n' $yumConfFile
else
    echo "<$yumConfFile>The data has been added!!"
fi

#安装相应程序
if [ -f $CurrentPath/VAS_Access.tar.gz ];then
    tar -zxvf $CurrentPath/VAS_Access.tar.gz -C $CurrentPath
else
    echo "VAS_Access.tar.gz Packet Not Exist!!!"
    exit 0
fi

#挂载光盘到相应目录
mountName="library32.iso"
mount -o loop $CurrentPath/$mountName $mountPath

#安装相应到依赖包
function installPacket() {
    echo "yum -y install $1 --nogpgcheck"
    yum -y install $1 --nogpgcheck
    if (( $? == 1 )); then
    	echo -e "\e[1;31mPKG $1 failed to be installed\e[0m"
    else
        echo -e "\e[1;34mPKG $1 has been installed successfully\e[0m"
   	fi
}

installPacket *stdc*.i686
installPacket *uuid*.i686
installPacket *boost*.i686
installPacket *Xv*.i686
installPacket *glibc*.i686
installPacket *glibc-2.12*.i686
installPacket *alsa*.i686
installPacket *krb5-libs*.i686

#umount相关挂载文件和删除文件
umount /VAmnt
rm -rf /VAmnt

#检查程序安装包是否完整
count=0
videoAccess="VideoAccess.conf"
zlog="VideoZlog.conf"
confFile=$(ls -l $CurrentPath |awk '/^-/ {print $NF}')
for i in $confFile
do
    if [ "$videoAccess" = "$i" ];then
        echo -e "\e[1;34mThe VideoAccess.conf file Exist\e[0m"
        let count+=1;
    elif [ "$zlog" = "$i" ];then
        echo -e "\e[1;34mThe VideoZlog.conf file Exist\e[0m"
        let count+=1;
    elif [ "VAS_Access" = "$i" ];then
        echo -e "\e[1;34mThe VAS_Access file Exist\e[0m"
        let count+=1;
    fi
done

if [ 3 -ne $count ];then
    echo -e "\e[1;34mThe configuration file is not complete\e[0m"
    exit 0
fi

#查看当前目录下到lib文件夹信息
ldConfFile="/etc/ld.so.conf"
upgrade="upgrade"
logdir="log"
libDir=$(ls -l $CurrentPath |awk '/^d/ {print $NF}')
for i in $libDir
do
    if [ "$upgrade" = "$i" ];then
        echo "The upgrade dir don't lib dir"
    elif [ "$logdir" = "$i" ];then
        echo "The log dir don't lib dir"
    elif [ 0 -eq $(grep -i "$CurrentPath/$i/lib" $ldConfFile | wc -l) ];then
    	echo $CurrentPath/$i/lib >> $ldConfFile
    fi
done

#添加程序自启动脚本
autostartsh="/etc/rc.d/init.d/VAS_AccessAutoStart.sh"
rclocal="/etc/rc.local"
if [ ! -f "$autostartsh" ];then
    echo "" > $autostartsh
	touch $autostartsh
	chmod 755 $autostartsh
    sed -i '1i\#!/bin/bash\ncd '$CurrentPath'\n'$CurrentPath'/VAS_Access' $autostartsh
    echo $autostartsh >> $rclocal
fi

#添加修改文件描述取值范围
openfile="/etc/security/limits.conf"
sed -i '$a\* soft nofile 65535\n* hard nofile 65535' $openfile

#生成log文件夹
mkdir "$CurrentPath/log/"

#配置生效
ldconfig
echo -e "\e[1;43mInstall ok!!!!\e[0m"

#启动程序
#chmod 755 $CurrentPath/VAS_Access
#$CurrentPath/VAS_Access
