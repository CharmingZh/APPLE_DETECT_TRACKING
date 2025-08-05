import cv2
import numpy as np

# --- 修改这里 ---
IMAGE_PATH = 'color_frame_40_1.png'  # 替换成您的一张苹果图片的文件名
# --------------

def nothing(x):
    pass

# 创建一个带滑动条的窗口
cv2.namedWindow('HSV Tuner')
cv2.createTrackbar('H_min', 'HSV Tuner', 0, 179, nothing)
cv2.createTrackbar('S_min', 'HSV Tuner', 0, 255, nothing)
cv2.createTrackbar('V_min', 'HSV Tuner', 0, 255, nothing)
cv2.createTrackbar('H_max', 'HSV Tuner', 179, 179, nothing)
cv2.createTrackbar('S_max', 'HSV Tuner', 255, 255, nothing)
cv2.createTrackbar('V_max', 'HSV Tuner', 255, 255, nothing)

# 读取图片
image = cv2.imread(IMAGE_PATH)
if image is None:
    print(f"错误：无法读取图片 '{IMAGE_PATH}'")
    exit()

hsv_image = cv2.cvtColor(image, cv2.COLOR_BGR2HSV)
cv2.imshow('Original Image', image)

print("\n请拖动滑动条，直到Mask窗口中的苹果区域变白，其他区域变黑。")
print("完成后，按 'q' 键退出，并记录下H, S, V 的 min/max 值。")

while True:
    # 读取滑动条的当前值
    h_min = cv2.getTrackbarPos('H_min', 'HSV Tuner')
    s_min = cv2.getTrackbarPos('S_min', 'HSV Tuner')
    v_min = cv2.getTrackbarPos('V_min', 'HSV Tuner')
    h_max = cv2.getTrackbarPos('H_max', 'HSV Tuner')
    s_max = cv2.getTrackbarPos('S_max', 'HSV Tuner')
    v_max = cv2.getTrackbarPos('V_max', 'HSV Tuner')

    # 定义 HSV 范围
    lower_bound = np.array([h_min, s_min, v_min])
    upper_bound = np.array([h_max, s_max, v_max])

    # 创建蒙版
    mask = cv2.inRange(hsv_image, lower_bound, upper_bound)
    cv2.imshow('Mask', mask)

    # 按 'q' 键退出
    if cv2.waitKey(1) & 0xFF == ord('q'):
        print("\n最终的 HSV 阈值:")
        print(f"Lower: [{h_min}, {s_min}, {v_min}]")
        print(f"Upper: [{h_max}, {s_max}, {v_max}]")
        break

cv2.destroyAllWindows()