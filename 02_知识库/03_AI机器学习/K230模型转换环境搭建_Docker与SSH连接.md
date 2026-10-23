---
类型: 知识
状态: 已完成
标签: [K230, Docker, WSL, 环境配置, 模型转换, SSH, Remote-SSH, nncase]
创建: 2026-08-23
来源: 学不会电磁场B站图文教程
---

# K230 模型转换环境搭建（Docker + WSL + SSH）

## 简介

K230 模型转换（`.onnx` → `.kmodel`）依赖官方提供的 **`k230_model_converter` Docker 镜像**。本笔记记录从零搭建这条转换环境链路的完整步骤：**WSL 装 Ubuntu → 装 Docker → 加载转换镜像 → 启动容器 → VS Code Remote-SSH 连接**。

> 📌 本笔记由「学不会电磁场」的图文教程整理优化而来。
>
> 图文教程原文：<https://www.bilibili.com/opus/1115699316611088393>

---

## 一、环境准备：WSL 安装 Ubuntu

打开终端（Win+R，输入 `cmd`）：

1. **查看当前 Ubuntu 版本**
   ```bash
   wsl --list
   ```
   未安装时为空列表。![[Pasted image 20260823160651.png]]

2. **查看可安装版本列表**
   ```bash
   wsl --list --online
   ```

3. **安装 Ubuntu-22.04**
   ```bash
   wsl --install Ubuntu-22.04
   ```
   - 网络不稳定可能导致失败，多试几次即可。
   - 安装过程需设置用户名和密码：
     - `Enter new UNIX username`：用户名，**必须为英文**
     - `New password`：密码，输入时**不显示**，记住后回车

---

## 二、Docker 安装（阿里云镜像源）

> 进入 Ubuntu 后，若终端不是 Ubuntu 环境（确保终端如下图状态），先运行 `wsl` 再 `cd` 回到 home。
>
> ![[Pasted image 20260823162435.png]]

### 2.1 基础安装

```bash
sudo apt-get update                        # 需输入前面设置的密码
sudo apt-get upgrade

# 移除旧版 docker（若存在）
sudo apt-get remove docker docker-engine docker.io containerd runc

# 安装前置依赖
sudo apt-get install ca-certificates curl gnupg lsb-release

# 添加阿里云密钥
curl -fsSL http://mirrors.aliyun.com/docker-ce/linux/ubuntu/gpg | sudo apt-key add -

# 添加阿里云软件源
sudo add-apt-repository "deb [arch=amd64] http://mirrors.aliyun.com/docker-ce/linux/ubuntu $(lsb_release -cs) stable"

# 安装 docker
sudo apt-get install docker-ce docker-ce-cli containerd.io
```

### 2.2 免 sudo 配置 + 启动

```bash
# 将当前用户加入 docker 组，之后无需 sudo 即可使用 docker
sudo groupadd docker
sudo usermod -aG docker $USER

sudo systemctl start docker
sudo apt-get -y install apt-transport-https ca-certificates curl software-properties-common
sudo service docker restart

# 重启使组权限生效
```

### 2.3 验证安装

```bash
exit                     # 退出 Ubuntu，回到 Windows
wsl --list               # 查看是否安装成功
```

> 💡 **多版本 Ubuntu 切换**：若装过多个版本，默认进入的是带「（默认值）」标记的版本。用以下命令切换默认版本：
> ```bash
> wsl --set-default Ubuntu-20.04
> ```

重新进入 Ubuntu：
```bash
wsl
cd
docker images            # 刚安装时为空列表
```

---

## 三、加载 k230_model_converter 镜像

从资料包拷贝镜像 tar 到当前目录：

```bash
# 格式：cp /mnt/<文件路径> <空格> .
cp /mnt/H/Projects/K230/yolo/k230_model_converter.tar .
```

> 💡 `/mnt/` 前缀是 WSL 访问 Windows 磁盘的挂载点：`/mnt/H/...` 即 Windows 的 `H:\...`。

加载镜像：

```bash
docker load -i k230_model_converter.tar
```

此时 `docker images` 应显示 `k230_model_converter:2.0`，即加载成功。![[Pasted image 20260823164128.png]]

### 镜像内部环境（转换工具链，L1 一手证据）

> 取自镜像 config 与 `.bash_history`，可 SSH 进容器后自行验证。

- **系统**：Ubuntu 20.04（amd64），Python 3.8，TZ=Asia/Shanghai，内置 openssh-server（22 端口）
- **核心工具链**：
  - **nncase 2.9.0**（`nncase-2.9.0-cp38-cp38-manylinux...whl`）+ **nncase-kpu** ← 模型转换核心
  - onnx 1.9.0 / onnx-simplifier 0.3.6 / onnxruntime 1.8.0
  - cmake 3.30.2、dotnet-runtime-7.0（nncase 2.x 编译器是 .NET 7 实现）
- **支持 target**：cpu / k510 / k230 / xpu
- `.bash_history` 还原的安装序列：

  ```bash
  wget https://github.com/kendryte/nncase/releases/download/v2.9.0/nncase-2.9.0-cp38-cp38-manylinux_2_17_x86_64.manylinux2014_x86_64.whl
  pip install nncase
  pip install nncase-kpu
  ```

**关键版本约束**：nncase 与 nncase-kpu 版本必须严格一致，且要与开发板 CanMV 固件版本匹配（2.9.0 转换 → 烧 nncase_v2.9.0 固件）。版本不匹配是「转换成功但板端结果随机」的头号根因。

---

## 四、启动容器

```bash
docker run -it --name k230 -p 22222:22 k230_model_converter:2.0 /bin/bash
```

参数说明：

| 参数 | 含义 |
|------|------|
| `docker run` | 新建并启动容器 |
| `-it` | 交互式终端 |
| `--name k230` | 容器命名为 `k230` |
| `-p 22222:22` | 端口映射：主机 22222 → 容器 22（SSH） |
| `k230_model_converter:2.0` | 镜像名（K230 模型转换镜像） |
| `/bin/bash` | 容器启动命令 |


运行后即进入容器。

### 启动容器内 SSH

> ⚠️ **关键坑（已踩过）**：`sudo service ssh restart` 重启的是 **WSL 发行版**的 ssh，**不是容器里的 sshd**。容器启动命令是 `/bin/bash`，不会自动拉起 sshd，需手动在容器内启动：
> ```bash
> docker exec k230 sudo -n /usr/sbin/sshd
> ```
> 详见排坑笔记 [[K230容器_RemoteSSH连不上_sshd未启动]]。

---

## 五、退出容器 + 查看 IP

- 退出容器回到 Ubuntu（容器保持后台运行）：**Ctrl+P 然后 Ctrl+Q**
- 查看容器 IP（需先装 net-tools）：

```bash
sudo apt install net-tools
ifconfig
```

![[1ab9a67d34b98fe62d26ede4d868b3f4.png]]

---

## 六、VS Code Remote-SSH 连接

1. VS Code 安装插件 **Remote - SSH**（注意切换为「远程隧道 / SSH」模式）。
2. 连接：
   ```bash
   ssh user@<IP地址> -p 22222
   # 密码：12345678
   ```
   - 示例：`ssh user@172.27.61.117 -p 22222`（原文笔者的 IP，会随 WSL 重启变化，以 `ifconfig` 实测为准）

---

## 七、日常启动顺序（已优化）

容器停止后，按以下顺序重启：

```bash
# Win+R → cmd
wsl
cd
docker start k230
docker exec k230 sudo -n /usr/sbin/sshd     # 在容器内启动 sshd（替代原 service ssh restart）
```

> 💡 已封装一键脚本 `k230ssh`（自动确保容器 + sshd 都在跑并打印连接命令），见 [[k230ssh_安装说明]] / [[K230容器_RemoteSSH连不上_sshd未启动]]。

---

## 与原文的差异修正（供对照）

| 原文 | 优化版 | 说明 |
|------|--------|------|
| 标题「onnx转kmodel环境配置」 | 「Docker + WSL + SSH 环境搭建」 | 原文标题与正文不符，正文实为环境搭建 |
| `-p 2222:22` | `-p 22222:22` | 端口笔误，按实测纠正 |
| `sudo service ssh restart` | `docker exec k230 sudo -n /usr/sbin/sshd` | 原命令重启的是 WSL 的 ssh，非容器 sshd |
| 错别字「dacker」 | `docker` | 修正 |

---

## 相关链接

- [[K230_YOLO训练转换与部署]]（转换命令与量化参数详解）
- [[铁球检测模型_2026电赛]]（nncase 2.9.0 转换实例）
- [[K230容器_RemoteSSH连不上_sshd未启动]]（sshd 未启动排坑）
- [[k230ssh_安装说明]]（一键启动脚本）
- [[K230_nncase图像分类部署]]（板端部署）
- [[K230开发板档案]]（目标板卡）
