import os
import ujson
from media.sensor import *
from media.display import *
from media.media import *
from time import *
import nncase_runtime as nn
import ulab.numpy as np
import time
import utime
import image
import random
import gc
from machine import UART

display_mode="hdmi"
if display_mode=="lcd":
    DISPLAY_WIDTH = ALIGN_UP(800, 16)
    DISPLAY_HEIGHT = 480
else:
    DISPLAY_WIDTH = ALIGN_UP(1920, 16)
    DISPLAY_HEIGHT = 1080

OUT_RGB888P_WIDTH = ALIGN_UP(1280, 16)
OUT_RGB888P_HEIGH = 720

root_path="/sdcard/mp_deployment_source/"
config_path=root_path+"deploy_config.json"
deploy_conf={}
debug_mode=1

# ===== 新增：两次发送之间的最小间隔(ms) =====
SEND_INTERVAL_MS = 200
_last_send_time = 0

class ScopedTiming:
    def __init__(self, info="", enable_profile=True):
        self.info = info
        self.enable_profile = enable_profile

    def __enter__(self):
        if self.enable_profile:
            self.start_time = time.time_ns()
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        if self.enable_profile:
            elapsed_time = time.time_ns() - self.start_time
            print(f"{self.info} took {elapsed_time / 1000000:.2f} ms")

def read_deploy_config(config_path):
    # 打开JSON文件以进行读取deploy_config
    with open(config_path, 'r') as json_file:
        try:
            config = ujson.load(json_file)
        except ValueError as e:
            print("JSON 解析错误:", e)
    return config

# 任务后处理
def softmax(x):
    exp_x = np.exp(x - np.max(x))
    return exp_x / np.sum(exp_x)

def sigmoid(x):
    return 1 / (1 + np.exp(-x))


# UART初始化 K230→STM32
# UART2: TX=GPIO11, RX=GPIO12, 波特率115200
uart = UART(UART.UART2, baudrate=115200, tx=11, rx=12, timeout=10)

# 解析标签 -> 花色+点数
# 标签格式: 10A/10B/10C/10D (10+花色), 2A/2B..AD (数字+花色), KingA/KingB(王), AA/AB(未识别)
# 花色映射: A=1(黑桃), B=2(红心), C=3(方块), D=4(梅花)
# 点数映射: 1=A, 2-10, 11=J, 12=Q, 13=K
# 特殊: KingA/KingB=王(小王=14, 大王=15), AA/AB=未识别(0,0)
def parse_card_label(label):
    # 王/大小王
    if label == "KingA":
        return (5, 14)
    elif label == "KingB":
        return (5, 15)
    elif label.startswith("00"):
        return (0, 0)

    suit_char = label[-1]
    suit = 0
    if suit_char == "A": suit = 1
    elif suit_char == "B": suit = 2
    elif suit_char == "C": suit = 3
    elif suit_char == "D": suit = 4

    if label.startswith("10"):
        rank = 10
    else:
        num_part = label[:-1]
        if num_part == "A": rank = 1
        elif num_part == "J": rank = 11
        elif num_part == "Q": rank = 12
        elif num_part == "K": rank = 13
        else:
            try: rank = int(num_part)
            except: rank = 0

    return (suit, rank)

def send_card_info(label):
    """通过UART向STM32发送扑克牌信息"""
    global _last_send_time
    suit, rank = parse_card_label(label)
    if suit == 0 and rank == 0:
        return  # 无牌不发送

    # ===== 新增：发送间隔限制，避免淹没STM32 =====
    now_ms = time.ticks_ms()
    if time.ticks_diff(now_ms, _last_send_time) < SEND_INTERVAL_MS:
        return
    _last_send_time = now_ms

    frame = bytearray([0xAA, 0x55, suit, rank, 0, 0x0D, 0x0A])
    frame[4] = (0xAA + 0x55 + suit + rank) & 0xFF

    # ===== 修复：改为非阻塞式写入，分段检查TX状态 =====
    # 先尝试直接写，K230 UART TX FIFO不满时不阻塞
    written = uart.write(frame)
    # 打印诊断信息
    print(f"[UART SEND] label={label} suit={suit} rank={rank} bytes_written={written if written else 0}")


def classification():
    print("start")

    # 使用json读取内容初始化部署变量
    deploy_conf=read_deploy_config(config_path)
    kmodel_name=deploy_conf["kmodel_path"]
    labels=deploy_conf["categories"]
    confidence_threshold=deploy_conf["confidence_threshold"]
    img_size=deploy_conf["img_size"]
    num_classes=deploy_conf["num_classes"]
    cls_idx=-1
    score=0.0

    print(f"[CONFIG] confidence_threshold={confidence_threshold}, num_classes={num_classes}, img_size={img_size}")

    # init kpu and load kmodel
    kpu = nn.kpu()
    ai2d = nn.ai2d()
    kpu.load_kmodel(root_path+kmodel_name)
    ai2d.set_dtype(nn.ai2d_format.NCHW_FMT,
                                   nn.ai2d_format.NCHW_FMT,
                                   np.uint8, np.uint8)
    ai2d.set_resize_param(True, nn.interp_method.tf_bilinear, nn.interp_mode.half_pixel )
    ai2d_builder = ai2d.build([1,3,OUT_RGB888P_HEIGH,OUT_RGB888P_WIDTH], [1,3,img_size[0],img_size[1]])
    # 初始化并配置sensor
    sensor = Sensor()
    sensor.reset()
    # 设置镜像
    sensor.set_hmirror(False)
    # 设置翻转
    sensor.set_vflip(False)
    # 通道0直接给到显示VO，格式为YUV420
    sensor.set_framesize(width = DISPLAY_WIDTH, height = DISPLAY_HEIGHT)
    sensor.set_pixformat(PIXEL_FORMAT_YUV_SEMIPLANAR_420)
    # 通道2给到AI做算法处理，格式为RGB888
    sensor.set_framesize(width = OUT_RGB888P_WIDTH , height = OUT_RGB888P_HEIGH, chn=CAM_CHN_ID_2)
    sensor.set_pixformat(PIXEL_FORMAT_RGB_888_PLANAR, chn=CAM_CHN_ID_2)
    # 绑定通道0的输出到vo
    sensor_bind_info = sensor.bind_info(x = 0, y = 0, chn = CAM_CHN_ID_0)
    Display.bind_layer(**sensor_bind_info, layer = Display.LAYER_VIDEO1)
    if display_mode=="lcd":
        # 设置为ST7701显示，默认800x480
        Display.init(Display.ST7701, to_ide = True)
    else:
        # 设置为LT9611显示，默认1920x1080
        Display.init(Display.LT9611, to_ide = True)

    #创建OSD图像
    osd_img = image.Image(DISPLAY_WIDTH, DISPLAY_HEIGHT, image.ARGB8888)

    try:
        # media初始化
        MediaManager.init()
        # 启动sensor
        sensor.run()
        # init
        rgb888p_img = None
        ai2d_input_tensor = None
        data = np.ones((1,3,img_size[0],img_size[1]),dtype=np.uint8)
        ai2d_output_tensor = nn.from_numpy(data)
        # ===== 新增：诊断用计数器 =====
        frame_count = 0
        detection_count = 0

        while True:
            with ScopedTiming("total",debug_mode > 0):
                rgb888p_img = sensor.snapshot(chn=CAM_CHN_ID_2)
                if rgb888p_img == -1:
                    print("capture_image failed")
                    camera.release_image(CAM_DEV_ID_0, CAM_CHN_ID_2, rgb888p_img)
                    continue
                # for rgb888planar
                if rgb888p_img.format() == image.RGBP888:
                    ai2d_input = rgb888p_img.to_numpy_ref()
                    ai2d_input_tensor = nn.from_numpy(ai2d_input)
                    ai2d_builder.run(ai2d_input_tensor, ai2d_output_tensor)
                    # set input
                    kpu.set_input_tensor(0, ai2d_output_tensor)
                    # run kmodel
                    kpu.run()
                    # get output
                    results = []
                    for i in range(kpu.outputs_size()):
                        output_data = kpu.get_output_tensor(i)
                        result = output_data.to_numpy()
                        del output_data
                        results.append(result)
                    if num_classes>2:
                        softmax_res=softmax(results[0][0])
                        cls_idx=np.argmax(softmax_res)
                        if softmax_res[cls_idx]>confidence_threshold:
                            score=softmax_res[cls_idx]
                            print("classification result:")
                            print(labels[cls_idx])
                            print("score",score)
                            detection_count += 1
                        else:
                            cls_idx=-1
                            score=0.0
                    else:
                        sigmoid_res=sigmoid(results[0][0][0])
                        if sigmoid_res>confidence_threshold:
                            cls_idx=1
                            score=sigmoid_res
                            print("classification result:")
                            print(labels[1])
                            print("score",score)
                            detection_count += 1
                        else:
                            cls_idx=0
                            score=1-sigmoid_res
                            print("classification result:")
                            print(labels[0])
                            print("score",score)
                frame_count += 1

                osd_img.clear()
                if cls_idx>=0:
                    osd_img.draw_string_advanced(5,5,32,"result:"+labels[cls_idx]+" score:"+str(score),color=(0,255,0))
                    send_card_info(labels[cls_idx])
                else:
                    osd_img.draw_string_advanced(5,5,32,"No card detected",color=(255,0,0))

                # ===== 每10秒打印一次诊断统计 =====
                if frame_count % 300 == 0 and frame_count > 0:
                    print(f"[DIAG] frames={frame_count} detections={detection_count} rate={detection_count*100/frame_count:.1f}%")

                Display.show_image(osd_img, 0, 0, Display.LAYER_OSD3)
                rgb888p_img = None
                gc.collect() #用于需要调用gc.mem_alloc()的内存
    except Exception as e:
        print(f"An error occurred during buffer used: {e}")
    finally:
        os.exitpoint(os.EXITPOINT_ENABLE_SLEEP)
        del ai2d_input_tensor
        del ai2d_output_tensor
        #停止摄像头输出
        sensor.stop()
        #去初始化显示设备
        Display.deinit()
        #释放媒体缓冲区
        MediaManager.deinit()
        gc.collect()
        time.sleep(1)
        nn.shrink_memory_pool()
    print("end")
    return 0


if __name__=="__main__":
    classification()