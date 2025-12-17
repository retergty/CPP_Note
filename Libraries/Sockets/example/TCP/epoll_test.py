import socket
import threading
import time
import random

# 配置
SERVER_IP = '127.0.0.1'
SERVER_PORT = 8080
ROBOT_COUNT = 500       # 模拟机器人的数量 (建议先从 100 开始测)
MSG_PER_ROBOT = 10      # 每个机器人发多少条消息
DELAY_RANGE = (0.1, 0.5) # 模拟网络延迟/发送间隔 (秒)

# 统计数据
success_count = 0
fail_count = 0
lock = threading.Lock()

def robot_task(robot_id):
    """模拟一个机器人的行为"""
    global success_count, fail_count
    
    try:
        # 1. 创建连接
        client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client.settimeout(5) # 设置 5 秒超时
        client.connect((SERVER_IP, SERVER_PORT))
        
        # print(f"[Robot-{robot_id}] Connected")

        # 2. 循环发送心跳
        for i in range(MSG_PER_ROBOT):
            msg = f"Robot-{robot_id}-Heartbeat-{i}"
            
            # 发送
            client.send(msg.encode('utf-8'))
            
            # 接收回声 (Echo)
            response = client.recv(1024)
            
            if response.decode('utf-8') == msg:
                # 简单验证回声是否正确
                with lock:
                    success_count += 1
            else:
                print(f"[Robot-{robot_id}] Error: Data mismatch!")

            # 随机休眠，模拟真实的不规律发送
            time.sleep(random.uniform(*DELAY_RANGE))

        # 3. 任务结束，关闭连接
        client.close()
        
    except Exception as e:
        with lock:
            fail_count += 1
        print(f"[Robot-{robot_id}] Exception: {e}")

def main():
    print(f"🚀 Starting Stress Test: {ROBOT_COUNT} robots...")
    start_time = time.time()
    
    threads = []

    # 1. 启动所有机器人线程
    for i in range(ROBOT_COUNT):
        t = threading.Thread(target=robot_task, args=(i,))
        threads.append(t)
        t.start()
        
        # 稍微错开一点启动时间，防止瞬间把系统端口耗尽
        if i % 50 == 0:
            time.sleep(0.1)

    # 2. 等待所有机器人完成任务
    for t in threads:
        t.join()

    end_time = time.time()
    duration = end_time - start_time

    # 3. 输出报告
    print("\n" + "="*30)
    print(f"📊 Test Report")
    print(f"Total Robots: {ROBOT_COUNT}")
    print(f"Total Messages: {ROBOT_COUNT * MSG_PER_ROBOT}")
    print(f"Time Taken: {duration:.2f} seconds")
    print(f"✅ Success: {success_count}")
    print(f"❌ Failed:  {fail_count}")
    print(f"Throughput: {success_count / duration:.2f} req/sec")
    print("="*30)

if __name__ == "__main__":
    main()