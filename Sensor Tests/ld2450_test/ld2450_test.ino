#define RXD 16
#define TXD 17

typedef struct RadarTarget
{
    uint16_t id;  // 目标ID
    int16_t x;           // X坐标，单位mm
    int16_t y;           // Y坐标，单位mm
    int16_t speed;       // 速度，单位cm/s
    uint16_t resolution; // 距离分辨率，单位mm
} RadarTarget_t;

RadarTarget_t nowTargets[3]; // 存储当前帧的目标

/**
* Reads radar data and populates an array of RadarTarget objects.
*
* @param rec_buf The buffer that contains the radar data.
* @param len The length of the radar data buffer.
* @param targets An array of RadarTarget objects to store the radar targets.
* @param numTargets The number of radar targets to read.
*
* @return Returns 1 on successful reading of radar data, or 0 if the data is incomplete.
*
* @throws None
**/
int readRadarData(byte rec_buf[], int len, RadarTarget targets[], uint16_t numTargets) {
  // if (radar_serial.available() > 0) {
  //     byte rec_buf[256] = "";
  //     int len = radar_serial.readBytes(rec_buf, sizeof(rec_buf));

      for (int i = 0; i < len; i++) {
          if (rec_buf[i] == 0xAA && rec_buf[i + 1] == 0xFF && rec_buf[i + 2] == 0x03 && rec_buf[i + 3] == 0x00 && rec_buf[i + 28] == 0x55 && rec_buf[i + 29] == 0xCC) { // 检查帧头和帧尾
              String targetInfo = ""; // 存储目标信息的字符串
              int index = i + 4; // 跳过帧头和帧内数据长度字段

              for (int targetCounter = 0; targetCounter < numTargets; targetCounter++) {
                  if (index + 7 < len) {
                      RadarTarget target;
                      target.x = (int16_t)(rec_buf[index] | (rec_buf[index + 1] << 8));
                      target.y = (int16_t)(rec_buf[index + 2] | (rec_buf[index + 3] << 8));
                      target.speed = (int16_t)(rec_buf[index + 4] | (rec_buf[index + 5] << 8));
                      target.resolution = (uint16_t)(rec_buf[index + 6] | (rec_buf[index + 7] << 8));

                      // debug_serial.println(target.x);
                      // debug_serial.println(target.y);
                      // debug_serial.println(target.speed);

                      // 检查x和y的最高位，调整符号
                      if (rec_buf[index + 1] & 0x80) target.x -= 0x8000;
                      else target.x = -target.x;
                      if (rec_buf[index + 3] & 0x80) target.y -= 0x8000;
                      else target.y = -target.y;
                      if (rec_buf[index + 5] & 0x80) target.speed -= 0x8000;
                      else target.speed = -target.speed;

                      // 赋值目标信息
                      // debug_serial.println(targetCounter + 1);
                      targets[targetCounter].id = targetCounter + 1;
                      targets[targetCounter].x = target.x;
                      targets[targetCounter].y = target.y;
                      targets[targetCounter].speed = target.speed;
                      targets[targetCounter].resolution = target.resolution;

                      // // 输出目标信息
                      // debug_serial.print("目标 ");
                      // debug_serial.print(++targetCounter); // 增加目标计数器并输出
                      // debug_serial.print(": X: ");
                      // debug_serial.print(target.x);
                      // debug_serial.print("mm, Y: ");
                      // debug_serial.print(target.y);
                      // debug_serial.print("mm, 速度: ");
                      // debug_serial.print(target.speed);
                      // debug_serial.print("cm/s, 距离分辨率: ");
                      // debug_serial.print(target.resolution);
                      // debug_serial.print("mm; ");

                      // 添加目标信息到字符串
                      targetInfo += "目标 " + String(targetCounter + 1) + ": X: " + target.x + "mm, Y: " + target.y + "mm, 速度: " + target.speed + "cm/s, 距离分辨率: " + target.resolution + "mm; ";

                      index += 8; // 移动到下一个目标数据的开始位置
                  }
              }

              // 输出目标信息
              // debug_serial.println(targetInfo);

              // 输出原始数据
              // debug_serial.print("原始数据: ");
              for (int j = i; j < i + 30; j++) {
                  if (j < len) {
                      // debug_serial.print(rec_buf[j], HEX);
                      // debug_serial.print(" ");
                  }
              }
              // debug_serial.println("\n"); // 换行，准备输出下一帧数据

              i = index; // 更新外部循环的索引
          }
          // else return 0; // 数据不完整，返回-1
      }
      return 1;
  // }
  // else return 0;  // 串口缓冲区为空，返回0
}


void setup() {
  Serial.begin(256000);
  Serial2.begin(256000, SERIAL_8N1, RXD, TXD);
  delay(100);

  Serial.println("Starting...");
}


void loop() {
  if (Serial2.available() > 0) {
    byte rec_buf[256] = "";
    int len = Serial2.readBytes(rec_buf, sizeof(rec_buf));

    int radar_flag = readRadarData(rec_buf, len, nowTargets, 3);
    //Serial.println(radar_flag);

    // if (radar_flag == 1) {
    //   for (int i = 0; i < 3; i++) {
    //     Serial.print("Target ");
    //     Serial.print(nowTargets[i].id);
    //     Serial.print(": X = ");
    //     Serial.print(nowTargets[i].x);
    //     Serial.print("mm, Y = ");
    //     Serial.print(nowTargets[i].y);
    //     Serial.print("mm, Speed = ");
    //     Serial.print(nowTargets[i].speed);
    //     Serial.print("cm/s, Resolution = ");
    //     Serial.print(nowTargets[i].resolution);
    //     Serial.print("mm; \n");
    //   }

    //   Serial.print("\n\n");
    // }
    if ((nowTargets[0].x != 0 && nowTargets[0].y != 0) || (nowTargets[1].x != 0 && nowTargets[1].y != 0) || (nowTargets[2].x != 0 && nowTargets[2].y != 0)){
      Serial.println("Exists");
    }else{
      Serial.println("Gone");
    }
  }
  delay(500);
}
