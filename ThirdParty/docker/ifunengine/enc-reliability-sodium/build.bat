rem https://forums.docker.com/t/docker-for-windows-should-resync-vm-time-when-computer-resumes-from-sleep/17825
@powershell $datetime = Get-Date; $dt = $datetime.ToUniversalTime().ToString('yyyy-MM-dd HH:mm:ss'); docker run --net=host --ipc=host --uts=host --pid=host --security-opt=seccomp=unconfined --privileged --rm ubuntu date -s $dt
docker build --tag ifunengine-sodium-reliability .