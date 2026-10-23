---
类型: 排坑
状态: 已完成
标签: [K230, Docker, WSL, SSH, Remote-SSH, VSCode]
创建: 2026-08-23
---

# K230 容器 Remote-SSH 连不上（sshd 未在容器内启动）

## 环境
- 芯片 / 板卡：K230（k230_model_converter:2.0 转换镜像，跑在 Docker 容器 `k230` 里）
- 工具链 / 版本：WSL2（Ubuntu-24.04）+ Docker + VS Code Remote-SSH

## 现象
- VS Code 用 `ssh user@172.27.61.117 -p 22222` 连接容器 k230 一直连不上。
- 端口 22222 其实在监听（`docker ps` 显示 `0.0.0.0:22222->22/tcp`），但握手无响应。

## 原因
- 容器 k230 的启动命令是 `/bin/bash`，`docker start k230` 只拉起 bash，**容器内的 sshd 没有被启动**。
- 用户按习惯执行的 `sudo service ssh restart` 重启的是 **WSL 发行版（Ubuntu-24.04）的 ssh**，不是容器里的 sshd——两者是两套进程，互不相干。
- 结果：22222 端口被 docker 占用并转发，但容器内部 22 端口没人监听，连接握手自然失败。

## 排查过程（命令序列）
```bash
wsl -l -v                        # 确认发行版在跑
docker ps -a --filter name=k230  # 容器 Up，端口映射正常
docker exec k230 bash -c 'ps aux | grep sshd'   # 无 sshd 进程
docker exec k230 bash -c 'service ssh status'   # sshd is not running
docker exec k230 grep -E 'Port|PermitRootLogin|PasswordAuthentication' /etc/ssh/sshd_config
# -> 全为默认注释：Port 22 / PasswordAuthentication yes / PermitRootLogin prohibit-password
```
- 关键判断：sshd 二进制（`/usr/sbin/sshd`）与 host key 都在，纯属「没启动」。

## 解决
1. 在容器内拉起 sshd（容器用户 `user` 在 sudo 组且可无密码提权）：
   ```bash
   docker exec k230 sudo -n /usr/sbin/sshd
   ```
2. 验证：
   ```bash
   docker exec k230 bash -c 'ps aux | grep sshd'   # 出现 [listener]
   # 从 WSL 侧测端口：
   timeout 5 bash -c 'cat < /dev/tcp/172.27.61.117/22222' | head -c 60
   # -> SSH-2.0-OpenSSH_8.2p1 Ubuntu-4ubuntu0.13   （握手成功）
   ```
3. 连接：`ssh user@172.27.61.117 -p 22222`（密码 12345678）

## 教训 / 关键点
- **`service ssh restart` 管的是宿主机（WSL 发行版），不是容器里的 sshd**。容器内要单独起 sshd。
- **容器重启后 sshd 会消失**（sshd 不是 PID 1，`docker restart` 不会自动拉起）。正确启动命令是 `docker start k230 && docker exec k230 sudo /usr/sbin/sshd`。
- 已封装一键脚本 `k230ssh`（见 [[k230ssh_安装说明]]），自动确保「容器在跑 + sshd 在跑」并打印连接命令。

## 相关链接
- [[k230ssh_安装说明]]（启动脚本备份，06_工具箱）
- [[K230开发板档案]]
- [[K230_YOLO训练转换与部署]]
- [[铁球检测模型_2026电赛]]
