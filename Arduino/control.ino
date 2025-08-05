// --- 常量定义 ---
// 你可以根据实际接线修改这些引脚号
const int motorAPin = 7; // 控制左侧通道 A 的电机/电磁阀
const int motorBPin = 8; // 控制右侧通道 B 的电机/电磁阀

// 定义电机/电磁阀每次动作的持续时间（毫秒）
// 150毫秒对于一个快速的推杆或电磁阀动作来说是比较典型的值
const int actionPulseMillis = 100;

// --- 初始化 ---
// setup() 函数只会在Arduino启动时运行一次
void setup() {
    // 初始化串口通信，波特率必须与C++程序中的设置完全一致
    Serial.begin(9600);

    // 将两个电机引脚设置为输出模式
    pinMode(motorAPin, OUTPUT);
    pinMode(motorBPin, OUTPUT);

    // 确保在程序开始时，两个电机都处于关闭状态（低电平）
    digitalWrite(motorAPin, LOW);
    digitalWrite(motorBPin, LOW);

    // 通过串口发送一条启动信息，方便你确认Arduino已准备就绪
    Serial.println("Arduino Sorter is ready. Waiting for commands...");
}

// --- 主循环 ---
// loop() 函数会无限循环执行
void loop() {
    // 检查串口的缓冲区中是否有可读的数据
    if (Serial.available() > 0) {
        // 读取一个字符指令
        char command = Serial.read();

        // 根据收到的指令执行相应的动作
        switch (command) {
            case 'A':
                // 如果收到 'A'，打印信息到串口监视器并触发A电机
                Serial.println("Command 'A' received. Triggering Motor A.");
                triggerMotor(motorAPin);
                break;

            case 'B':
                // 如果收到 'B'，打印信息到串口监视器并触发B电机
                Serial.println("Command 'B' received. Triggering Motor B.");
                triggerMotor(motorBPin);
                break;

            default:
                // 如果收到任何其他字符，则忽略不处理
                break;
        }
    }
}

// --- 辅助函数 ---
// 用于触发电机产生一个短暂的脉冲动作
void triggerMotor(int pin) {
    digitalWrite(pin, HIGH);    // 打开电机/电磁阀
    delay(actionPulseMillis);   // 保持一小段时间
    digitalWrite(pin, LOW);     // 关闭电机/电磁阀，完成一次推/拉动作
}
