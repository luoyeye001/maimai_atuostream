#include <Keyboard.h>
//有问题请发issue，designed by 桃玖
const byte SENSOR_A_PIN = 2;   // 声明传感器A引脚
const byte SENSOR_B_PIN = 3;   // 声明传感器B引脚

// 状态定义
enum State { 
  IDLE,          // 空闲状态
  MOTION,        // 运动状态
  WAIT_FOR_KEY,  // 等待输出按键信号状态
  KEY_SENT       // 输出按键信号后状态
} state;

const unsigned long SAMPLE_INTERVAL = 100;    // 每次采样间隔（毫秒）为100ms
const unsigned int SAMPLE_NUM = 50;           // 每次采样次数为50

const float THRESHOLD_A = 0.1;                // 传感器A的触发阈值为0.1
const float THRESHOLD_B = 0.3;                // 传感器B的触发阈值为0.3

const unsigned long STANDBY_TIME = 5 * 1000;  // 缓冲时间（毫秒）为5s
const unsigned long HOLD_TIME = 10 * 60 * 1000;    // 输出按键后的保持时间（毫秒）为10min
const unsigned long IDLE_TIME = 10 * 60 * 1000;    // 空闲超时时间为10分钟

unsigned long lastMillis = 0;
unsigned int sampleCountA = 0;
unsigned int sampleCountB = 0;
boolean dataReady;               // 连续检测到有人存在的标志
unsigned long lastMotionMillis;  // 上一次有人存在的时间
unsigned long holdBeginMillis;   // 按键输出后的开始保持时间

// 初始化函数
void setup() {
  Keyboard.begin();
  pinMode(SENSOR_A_PIN, INPUT);
  pinMode(SENSOR_B_PIN, INPUT);
  dataReady = false;
  state = IDLE;
}

// 状态转移处理函数
void processState() {
  switch (state) {
    case IDLE:
      if (dataReady) {  // 判断是否进入运动状态
        state = MOTION;
        lastMotionMillis = millis();
      }
      break;

    case MOTION:
      if (!dataReady || millis() - lastMotionMillis >= STANDBY_TIME) {  // 超过缓冲时间则进入等待输出按键信号状态
        state = WAIT_FOR_KEY;
      }
      break;

    case WAIT_FOR_KEY: {
      // 根据情况进行输出按键
      float avgA = (float)sampleCountA / SAMPLE_NUM;
      float avgB = (float)sampleCountB / SAMPLE_NUM;

      if (avgA > THRESHOLD_A && avgB < THRESHOLD_B) {  
        Keyboard.press(KEY_LEFT_SHIFT);
        Keyboard.press(KEY_F1);
        delay(100);  
        Keyboard.releaseAll();     
      } else if (avgB > THRESHOLD_B && avgA < THRESHOLD_A) {  
        Keyboard.press(KEY_LEFT_SHIFT);
        Keyboard.press(KEY_F2);
        delay(100);  
        Keyboard.releaseAll();    
      } else if (avgA > THRESHOLD_A && avgB > THRESHOLD_B) {  
        Keyboard.press(KEY_LEFT_SHIFT);
        Keyboard.press(KEY_F3);
        delay(100); 
        Keyboard.releaseAll(); 
      } else {   // A区域和B区域都没有人
        Keyboard.press(KEY_LEFT_SHIFT);
        Keyboard.press(KEY_F4);
        delay(100);  
        Keyboard.releaseAll(); 
      }
      
      holdBeginMillis = millis();    // 记录按键输出时间
      state = KEY_SENT;
      break;
    }

    case KEY_SENT:
      if (millis() - holdBeginMillis >= HOLD_TIME) {   // 超过保持时间则回到空闲状态，同时重置数据标志和采样计数器
        state = IDLE;
        dataReady = false;
        sampleCountA = 0;
        sampleCountB = 0;
      }
      break;
  }
}

// 循环函数
void loop() {
  unsigned long currentMillis = millis();
  float avgA = 0.0;   // 在这里声明这两个变量
  float avgB = 0.0;  if (currentMillis - lastMillis >= SAMPLE_INTERVAL) {
    lastMillis = currentMillis;

    // 采样，计算平均值
    for (int i = 0; i < SAMPLE_NUM; i++) {
      sampleCountA += digitalRead(SENSOR_A_PIN);
      sampleCountB += digitalRead(SENSOR_B_PIN);
    }

    // 计算平均值
    avgA = (float)sampleCountA / SAMPLE_NUM;
    avgB = (float)sampleCountB / SAMPLE_NUM;

    // 当有人存在时，计入数据，并设置标志位
    if (avgA > THRESHOLD_A || avgB > THRESHOLD_B) {
      lastMotionMillis = millis();
      dataReady = true;
    }

    switch (state) {
      case IDLE:
        processState();     // 运动状态检测函数
        break;

      case MOTION:
        // 检测是否仍有人在该区域
        if (avgA <= THRESHOLD_A && avgB <= THRESHOLD_B) {
          state = IDLE;
          dataReady = false;
          sampleCountA = 0;
          sampleCountB = 0;
        }
        break;

      case WAIT_FOR_KEY:
      case KEY_SENT:
        // 按键保持状态无需进行额外处理
        break;
    }

    // 当空闲超过一定时间时，强制进入空闲状态，重置数据标志和采样计数器
    if (state == IDLE && millis() - lastMotionMillis >= IDLE_TIME) {
      dataReady = false;
      sampleCountA = 0;
      sampleCountB = 0;
    }
  }

  // 如果连续五秒钟没有检测到人，则重新开始闲置状态，并重置数据标志及采样计数器
  if ((avgA <= THRESHOLD_A && avgB <= THRESHOLD_B) && (currentMillis - lastMotionMillis > SAMPLE_INTERVAL * 5)) {
    state = IDLE;
    dataReady = false;
    sampleCountA = 0;
    sampleCountB = 0;
  }
}
