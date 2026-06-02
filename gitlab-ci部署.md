部署gitlab及gitlab-runner 14.8.2
---------

### docker安装gitlab

参考

https://blog.csdn.net/wq975826228/article/details/153732069?ops_request_misc=elastic_search_misc&request_id=8b17abd8c40af653ec62faba07761de9&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~baidu_landing_v2~default-1-153732069-null-null.142^v102^pc_search_result_base6&utm_term=%E6%9C%AC%E5%9C%B0docker%E9%83%A8%E7%BD%B2gitlab&spm=1018.2226.3001.4187

##### 拉取gitlab镜像

```bash
# 拉取最新版
docker pull gitlab/gitlab-ce  或者 docker pull gitlab/gitlab-ce:latest
# 拉取指定版本：
docker pull gitlab/gitlab-ce:18.5


#这里用的
docker pull gitlab/gitlab-ee:14.8.2-ee.0
docker pull gitlab/gitlab-ee:14.8
```



如果是离线环境

```bash
# 在线环境下打包镜像
docker save -o gitlab.tar gitlab/gitlab-ee:14.8.2-ee.0
#打包并压缩
docker save gitlab/gitlab-ee:14.8.2-ee.0 | gzip > gitlab-ee-14.8.tar.gz

# 把镜像上传到离线环境，导入镜像
docker load -i gitlab.tar
```

##### 启动gitlab

```bash
# --privileged=true 以特权模式运行容器，相当于赋予容器类似root的特权权限
docker run -d \
--restart unless-stopped --name gitlab \
-p 8443:443 -p 8090:80 -p 8022:22 \
-v /var/gitlab/etc:/etc/gitlab \
-v /var/gitlab/log:/var/log/gitlab \
-v /var/gitlab/opt:/var/opt/gitlab \
--privileged=true \
gitlab/gitlab-ee:14.8.2-ee.0
```

--restart 参数说明

| no                       | **默认策略**，容器退出后不会自动重启                         |
| ------------------------ | ------------------------------------------------------------ |
| always                   |                                                              |
| unless-stopped           |                                                              |
| on-failure[:max-retries] | **仅在容器因错误退出时（即退出码非0）重启**。可以限制最大重启次数，例如 `on-failure:3` 表示最多尝试重启3次 |

##### 启动端口配置

注意：
1.external_url配置了端口，并且端口不是80，就必须配置nginx[‘listen_port’] = 80，否则会访问不到
原因：external_url 带端口后，GitLab 会让 Nginx 监听该端口（8090），但 Docker 容器内对应的端口是 80 导致匹配上不
2.external_url不配置端口，可以访问，但存在问题：GitLab 生成的链接有问题
项目地址等就会缺少端口导致复制的地址访问不了
例：Clone 地址：http://192.168.56.10/root/java-gitlab.git --缺少端口
正确地址：http://192.168.56.10:8090/root/java-gitlab.git

![](image/ffb25e00a903400abaa642d062c58d37.png)



```bash
# 进入容器
docker exec -it gitlab /bin/bash
# 1.编辑gitlab.rb文件
vi /etc/gitlab/gitlab.rb
# gitlab访问地址，可以写域名。如果端口不写的话默认为80端口，如果配置了端口就必须配置nginx['listen_port'] = 80
external_url 'http://192.168.56.10:8090'
# 容器内监听端口80（映射到宿主机 8090）
nginx['listen_port'] = 80
# ssh主机ip
gitlab_rails['gitlab_ssh_host'] = '192.168.56.10'
# ssh连接端口
gitlab_rails['gitlab_shell_ssh_port'] = 8022
# 时区
gitlab_rails['time_zone'] = 'Asia/Shanghai'
# 开启备份功能
gitlab_rails['manage_backup_path'] = true
# 备份路径
gitlab_rails['backup_path'] = "/var/opt/gitlab/backups"
# 备份文件的权限
gitlab_rails['backup_archive_permissions'] = 0644
# 保存备份 60 天
gitlab_rails['backup_keep_time'] = 5184000


# 让配置生效（容器中执行）
gitlab-ctl reconfigure
```

**查看配置ip和端口是否和启动时一致，如果不一致需要修改**

```bash
# 编辑gitlab.yml文件（gitlab重启后需要重新配置）
vi /opt/gitlab/embedded/service/gitlab-rails/config/gitlab.yml
# 将文件中的port改成8090，和启动时候映射的端口一致
gitlab:
    host: 192.168.56.10
    port: 8090 
    https: false
或者
vi /var/opt/gitlab/gitlab-rails/etc/gitlab.yml
# 将文件中的port改成8090，和启动时候映射的端口一致
gitlab:
    # 虚拟机地址
    host: 192.168.56.10
    port: 8090 
    https: false

# 让配置生效（容器中执行）
gitlab-ctl restart
```

##### 设置登录密码

默认用户名：root
启动密码位置：cat /var/gitlab/etc/initial_root_password 或者容器内部（docker exec -it gitlab /bin/bash）cat /etc/gitlab/initial_root_password

```bash
# 重置密码
# 进入容器内部
docker exec -it gitlab /bin/bash

# 进入控制台，需要等一会，-e production：强制指定Rails控制台运行在production环境
gitlab-rails console -e production
# 查询id为1的用户，id为1的用户是超级管理员
# 会输出=> #<User id:1 @root>
user = User.where(id:1).first
# 修改密码
user.password='Qaz.123456'
# 保存(长度至少8个字符等校验会报错：Validation failed: Password must not contain commonly used combinations of words and letters, Password is too short (minimum is 8 characters) (ActiveRecord::RecordInvalid))
user.save!
# 测试密码是否正确，正确会返回=>true
user.valid_password?('Qaz.123456')
# 退出
exit
```





### 安装gitlab-runner（docker）

官方文档：https://docs.gitlab.com/runner/install/docker.html

```bash
docker pull gitlab/gitlab-runner:v14.8.2

#拉取runner-helper,不需要run
docker pull gitlab/gitlab-runner-helper:x86_64-v14.8.2
```

运行镜像

```bash
docker run -dit \
--name gitlab-runner \
--restart unless-stopped \
-v /srv/gitlab-runner/config:/etc/gitlab-runner \
-v /var/run/docker.sock:/var/run/docker.sock \
gitlab/gitlab-runner:v14.8.2
```

### 安装gitlab-runner（Windows）

创建目录（例如 `C:\GitLab-Runner`<font color=red>不要有中文路径</font>）并下载[官方exe文件](https://gitlab-runner-downloads.s3.amazonaws.com/latest/binaries/gitlab-runner-windows-amd64.exe)放入该目录，重命名为 `gitlab-runner.exe`

在该目录下打开管理员命令行，安装并启动服务：

powershell，<font color = red>管理员运行</font>

```
.\gitlab-runner.exe install
.\gitlab-runner.exe start
```

注册时需要从GitLab项目获取令牌（Settings -> CI/CD -> Runners）。使用交互式注册：

powershell

```
.\gitlab-runner.exe register
```



按提示输入GitLab URL、令牌，然后关键选择：

- **Executor**: 通常选 `shell`（直接在Windows系统执行命令）或 `docker-windows`（如果要用Windows容器）。
- **Tags**: 输入自定义标签，如 `windows`（后续在`.gitlab-ci.yml`中通过此标签调用）。
- **Shell**: 推荐 `powershell` 。



**一个config.toml例子**

```bash
concurrent = 1
check_interval = 0

[session_server]
  session_timeout = 1800

[[runners]]
  name = "win"
  url = "http://192.168.56.10:8090/"
  token = "sm4gxwixSxxG83NXsJT_"
  executor = "shell"
  shell = "powershell" #手动改
  environment = [	#添加工具到路径
    "PATH=C:\\Program Files\\Git\\bin;C:\\Program Files\\Git\\cmd;D:\\Program Files\\cmake-3.18.6-win64-x64\\bin;%PATH%"
  ]
  [runners.custom_build_dir]
  [runners.cache]
    [runners.cache.s3]
    [runners.cache.gcs]
    [runners.cache.azure]

```





### 配置runner

在gitlab项目中查看设置URL和Token参数

![](image/gitlab配置runner信息.png)



**选择运行未打标签的作业**

![](image/runner设置.png)



设置镜像拉取方式（否则会报错找不到镜像）



![image-20260318174253948](image/配置镜像获取方式.png)



``` tex
[[runners]]
  name = "test"
  url = "http://192.168.56.10:8090/"
  token = "CxT77yc_FBazxj1Rzxst"
  executor = "docker"
  [runners.custom_build_dir]
  [runners.cache]
    [runners.cache.s3]
    [runners.cache.gcs]
    [runners.cache.azure]
  [runners.docker]
    tls_verify = false
    image = "ub16runner:1.0"
    privileged = false
    disable_entrypoint_overwrite = false
    oom_kill_disable = false
    disable_cache = false
    volumes = ["/cache"]
    pull_policy = ["never"]
    shm_size = 0
```







### docker 扩容

虚拟机增加硬盘

管理->工具->虚拟介质管理

![image-20260318174542676](image/image-20260318174542676.png)

查看容量

```bash
df -h /var # Docker**默认位置*
```

**第一步：查看可用物理空间**

```bash
sudo vgdisplay ubuntu-vg
```

查看输出中的 **“Free PE / Size”** 行。如果有空闲空间（如 20.00 GiB），可以直接进行**第三步**。如果显示为 0，则需要先**第二步**扩展底层的物理卷。

**第二步：如果没有空闲空间，扩展物理卷**

```bash
# 1. 查看物理卷和磁盘分区情况
sudo pvdisplay

lsblk
# 2. 如果虚拟机磁盘已扩容但分区未利用，需先扩展分区
# 假设你的物理卷在 /dev/sda3 上
sudo growpart /dev/sda 3
# 3. 扩展物理卷
sudo pvresize /dev/sda3
# 4. 再次检查卷组空间
sudo vgdisplay ubuntu-vg
```

**第三步：扩展逻辑卷**

```bash
# 扩展逻辑卷（增加10G，可按需调整）
sudo lvextend -L +10G /dev/ubuntu-vg/ubuntu-lv

# 或者扩展到所有可用空间
sudo lvextend -l +100%FREE /dev/ubuntu-vg/ubuntu-lv
```

**第四步：扩展文件系统**

```bash
*# 对于ext4文件系统（Ubuntu默认）*

sudo resize2fs /dev/ubuntu-vg/ubuntu-lv
```

