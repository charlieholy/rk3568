#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/rculist.h>  // RCU 链表头文件
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/slab.h>      // kmalloc/kfree 必需头文件
#include <linux/types.h>     // 基础类型定义

// 模块许可证（必须）
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Demo");
MODULE_DESCRIPTION("RCU Demo Driver");

// 定义 RCU 保护的链表节点结构
struct rcu_node {
    int id;
    char name[32];
    struct list_head list;  // 链表节点（RCU 保护）
};

// 全局 RCU 链表头
static LIST_HEAD(rcu_demo_list);

// 读写线程句柄
static struct task_struct *reader_thread;
static struct task_struct *writer_thread;

// ==================== 读线程（RCU 读端） ====================
static int rcu_reader(void *data) {
    struct rcu_node *node;  // 读线程内的 node 声明
    int count = 0;

    while (!kthread_should_stop()) {
        rcu_read_lock(); // 进入 RCU 读临界区

        // 遍历 RCU 链表
        list_for_each_entry_rcu(node, &rcu_demo_list, list) {
            pr_info("[RCU Reader] 读取节点: id=%d, name=%s\n", node->id, node->name);
            msleep(10);
            count++;
            if (count >= 5) break;
        }

        rcu_read_unlock(); // 退出 RCU 读临界区
        msleep(200);
    }

    pr_info("[RCU Reader] 读线程退出\n");
    return 0;
}

// ==================== 写线程（RCU 写端） ====================
static int rcu_writer(void *data) {
    int id = 1;
    struct rcu_node *old_node = NULL;  // 旧节点指针
    struct rcu_node *new_node = NULL;  // 新节点指针
    struct rcu_node *node = NULL;      // 关键修复：声明写线程用的 node 变量

    while (!kthread_should_stop()) {
        // 步骤1：分配并初始化新节点
        new_node = kmalloc(sizeof(struct rcu_node), GFP_KERNEL);
        if (!new_node) {
            pr_err("[RCU Writer] 内存分配失败\n");
            msleep(1000);
            continue;
        }
        new_node->id = id;
        snprintf(new_node->name, sizeof(new_node->name), "node_%d", id);
        pr_info("[RCU Writer] 准备添加新节点: id=%d\n", id);

        // 步骤2：原子添加新节点到 RCU 链表
        list_add_rcu(&new_node->list, &rcu_demo_list);
        pr_info("[RCU Writer] 新节点已添加\n");

        // 步骤3：删除旧节点（id > 2 时）
        if (id > 2) {
            old_node = NULL; // 重置旧节点指针
            rcu_read_lock(); // 读临界区查找（安全）

            // 遍历链表找旧节点（此时 node 已声明，无编译错误）
            list_for_each_entry_rcu(node, &rcu_demo_list, list) {
                if (node->id == id - 2) {
                    old_node = node;
                    // 原子移除旧节点（不释放内存）
                    list_del_rcu(&old_node->list);
                    pr_info("[RCU Writer] 旧节点(id=%d)已从链表移除，等待宽限期\n", old_node->id);
                    break;
                }
            }

            rcu_read_unlock(); // 退出读临界区

            // 步骤4：等待宽限期 + 释放旧节点
            if (old_node) {
                synchronize_rcu(); // 等待所有读端完成
                kfree(old_node);
                pr_info("[RCU Writer] 旧节点(id=%d)内存已释放\n", id - 2);
            }
        }

        id++;
        msleep(2000); // 每2秒更新一次
    }

    pr_info("[RCU Writer] 写线程退出\n");
    return 0;
}

// ==================== 模块初始化 ====================
static int __init rcu_demo_init(void) {
    pr_info("[RCU Demo] 模块加载\n");

    // 创建读线程
    reader_thread = kthread_run(rcu_reader, NULL, "rcu_reader");
    if (IS_ERR(reader_thread)) {
        pr_err("[RCU Demo] 创建读线程失败\n");
        return PTR_ERR(reader_thread);
    }

    // 创建写线程
    writer_thread = kthread_run(rcu_writer, NULL, "rcu_writer");
    if (IS_ERR(writer_thread)) {
        pr_err("[RCU Demo] 创建写线程失败\n");
        kthread_stop(reader_thread);
        return PTR_ERR(writer_thread);
    }

    return 0;
}

// ==================== 模块卸载 ====================
static void __exit rcu_demo_exit(void) {
    struct rcu_node *node, *tmp;

    pr_info("[RCU Demo] 模块卸载\n");

    // 停止读写线程
    if (!IS_ERR(reader_thread)) {
        kthread_stop(reader_thread);
    }
    if (!IS_ERR(writer_thread)) {
        kthread_stop(writer_thread);
    }

    // 清理所有节点
    synchronize_rcu();
    list_for_each_entry_safe(node, tmp, &rcu_demo_list, list) {
        list_del_rcu(&node->list);
        kfree(node);
    }

    pr_info("[RCU Demo] 模块卸载完成\n");
}

// 模块入口/出口
module_init(rcu_demo_init);
module_exit(rcu_demo_exit);
