#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>

// 定义 /run 下的业务目录和文件路径
#define RUN_DIR "/run/my_daemon"
#define PID_FILE RUN_DIR "/my_daemon.pid"
#define LOCK_FILE RUN_DIR "/my_daemon.lock"

// 检查进程是否存活（用于判断 PID 文件是否有效）
int is_process_running(pid_t pid) {
    return (kill(pid, 0) == 0);
}

// 读取 PID 文件中的 PID
pid_t read_pid_file(const char *file_path) {
    FILE *fp = fopen(file_path, "r");
    if (!fp) return -1;

    pid_t pid;
    if (fscanf(fp, "%d", &pid) != 1) {
        fclose(fp);
        return -1;
    }
    fclose(fp);
    return pid;
}

// 创建 PID 文件
int create_pid_file() {
    int fd = open(PID_FILE, O_CREAT | O_WRONLY | O_TRUNC | O_CLOEXEC, 0644);
    if (fd == -1) {
        fprintf(stderr, "创建 PID 文件失败: %s (错误码: %d)\n", strerror(errno), errno);
        return -1;
    }

    // 获取当前进程 PID 并写入文件
    char pid_buf[32];
    snprintf(pid_buf, sizeof(pid_buf), "%d\n", (int)getpid());
    if (write(fd, pid_buf, strlen(pid_buf)) == -1) {
        fprintf(stderr, "写入 PID 失败: %s (错误码: %d)\n", strerror(errno), errno);
        close(fd);
        unlink(PID_FILE); // 写入失败则删除文件
        return -1;
    }

    fsync(fd); // 强制刷盘（tmpfs 也需要，确保数据写入）
    close(fd);
    printf("PID 文件创建成功: %s (PID: %d)\n", PID_FILE, getpid());
    return 0;
}

// 创建锁文件（空文件，仅用于标记运行状态）
int create_lock_file() {
    int fd = open(LOCK_FILE, O_CREAT | O_WRONLY | O_CLOEXEC, 0644);
    if (fd == -1) {
        fprintf(stderr, "创建锁文件失败: %s (错误码: %d)\n", strerror(errno), errno);
        return -1;
    }
    close(fd);
    printf("锁文件创建成功: %s\n", LOCK_FILE);
    return 0;
}

// 清理运行时文件（退出时调用）
void cleanup_runtime_files() {
    if (unlink(PID_FILE) == 0) {
        printf("已删除 PID 文件: %s\n", PID_FILE);
    }
    if (unlink(LOCK_FILE) == 0) {
        printf("已删除锁文件: %s\n", LOCK_FILE);
    }
    if (rmdir(RUN_DIR) == 0) {
        printf("已删除运行时目录: %s\n", RUN_DIR);
    }
}

int main() {
    // 步骤1：创建 /run/my_daemon 目录（权限 755，符合 /run 安全规范）
    if (mkdir(RUN_DIR, 0755) == -1) {
        if (errno != EEXIST) { // 目录已存在则忽略，否则报错
            fprintf(stderr, "创建目录 %s 失败: %s (错误码: %d)\n", RUN_DIR, strerror(errno), errno);
            return 1;
        }
        printf("运行时目录已存在: %s\n", RUN_DIR);
    } else {
        printf("运行时目录创建成功: %s\n", RUN_DIR);
    }

    // 步骤2：检查 PID 文件是否已存在，防止重复启动
    pid_t existing_pid = read_pid_file(PID_FILE);
    if (existing_pid > 0 && is_process_running(existing_pid)) {
        fprintf(stderr, "服务已在运行（PID: %d），退出！\n", existing_pid);
        return 1;
    } else if (existing_pid > 0) {
        // PID 文件存在但进程已死，删除无效 PID 文件
        unlink(PID_FILE);
        printf("发现无效 PID 文件，已删除！\n");
    }

    // 步骤3：创建 PID 文件和锁文件
    if (create_pid_file() != 0 || create_lock_file() != 0) {
        cleanup_runtime_files();
        return 1;
    }

    // 步骤4：模拟服务运行（睡眠 30 秒，可替换为实际业务逻辑）
    printf("服务启动成功，运行中（30 秒后退出）...\n");
    sleep(30);

    // 步骤5：退出前清理运行时文件
    cleanup_runtime_files();
    printf("服务正常退出！\n");

    return 0;
}
