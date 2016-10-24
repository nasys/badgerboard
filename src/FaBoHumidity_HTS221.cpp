/**
 @file FaBoHumidity_HTS221.cpp
 @brief This is a library for the FaBo Humidity I2C Brick.

   http://fabo.io/208.html

   Released under APACHE LICENSE, VERSION 2.0

   http://www.apache.org/licenses/

 @author FaBo<info@fabo.io>
*/

#include "FaBoHumidity_HTS221.h"

/**
 @brief コンストラクタ
 @param [in] addr I2C Slaveアドレス
*/
FaBoHumidity_HTS221::FaBoHumidity_HTS221(uint8_t addr) {
  _i2caddr = addr;
  Wire.begin();
}

/**
 @brief デバイス使用開始
 @retval true 正常終了
 @retval false デバイスエラー
*/
bool FaBoHumidity_HTS221::begin() {
  if ( checkDevice() ) {
    powerOn();
    configDevice();
    readCoef();
    return true;
  } else {
    return false;
  }
}

/**
 @brief デバイス接続確認
 @retval true 接続中
 @retval false デバイス無し
*/
bool FaBoHumidity_HTS221::checkDevice() {
  // デバイスが接続されているか確認
  if ( readI2c(HTS221_WHO_AM_I) == HTS221_DEVICE_ID ) {
    return true;
  } else {
    return false;
  }
}

/**
 @brief デバイス起動、データレート設定
*/
void FaBoHumidity_HTS221::powerOn() {
  uint8_t data = 0;
  data |= HTS221_PD;
  data |= HTS221_ODR_1HZ;
  writeI2c(HTS221_CTRL_REG1, data);
}

/**
 @brief 出力データ解像度設定
*/
void FaBoHumidity_HTS221::configDevice() {
  uint8_t data = 0;
  data |= HTS221_AVGH_32;
  data |= HTS221_AVGT_16;
  writeI2c(HTS221_AV_CONF, data);
}

/**
 @brief 校正データの読み込み
*/
void FaBoHumidity_HTS221::readCoef() {
  uint8_t data = 0;
  _H0_rH_x2 = readI2c(HTS221_H0_RH_X2);
  _H1_rH_x2 = readI2c(HTS221_H1_RH_X2);
  data = readI2c(HTS221_T1_T0_MSB);
  _T0_degC_x8  = ( data & 0x3 ) << 8;
  _T0_degC_x8 |= readI2c(HTS221_T0_DEGC_X8);
  _T1_degC_x8  = ( data & 0xC ) << 6;
  _T1_degC_x8 |= readI2c(HTS221_T1_DEGC_X8);
  _H0_T0_OUT  = readI2c(HTS221_H0_T0_OUT_H) << 8;
  _H0_T0_OUT |= readI2c(HTS221_H0_T0_OUT_L);
  _H1_T0_OUT  = readI2c(HTS221_H1_T0_OUT_H) << 8;
  _H1_T0_OUT |= readI2c(HTS221_H1_T0_OUT_L);
  _T0_OUT  = readI2c(HTS221_T0_OUT_H) << 8;
  _T0_OUT |= readI2c(HTS221_T0_OUT_L);
  _T1_OUT  = readI2c(HTS221_T1_OUT_H) << 8;
  _T1_OUT |= readI2c(HTS221_T1_OUT_L);
}

/**
 @brief 湿度出力
 @return 湿度(Rh%)
*/
float FaBoHumidity_HTS221::getHumidity() {
  uint8_t data;
  int16_t H_OUT = 0;
  float t_H0_rH, t_H1_rH;
  float humidity = 0.0;

  data = readI2c(HTS221_STATUS_REG);
  if ( data & HTS221_H_DA ) {
    H_OUT  = readI2c(HTS221_HUMIDITY_OUT_H) << 8;
    H_OUT |= readI2c(HTS221_HUMIDITY_OUT_L);
    // 1/2にする
    t_H0_rH = _H0_rH_x2 / 2.0;
    t_H1_rH = _H1_rH_x2 / 2.0;
    // 線形補間でもとめる
    humidity = t_H0_rH + ( t_H1_rH - t_H0_rH ) * ( H_OUT - _H0_T0_OUT ) / ( _H1_T0_OUT - _H0_T0_OUT );
  }
  return humidity;
}

/**
 @brief 温度出力
 @return 温度(Deg C)
*/
float FaBoHumidity_HTS221::getTemperature() {
  uint8_t data = 0;
  int16_t T_OUT = 0;
  float t_T0_degC, t_T1_degC;
  float temperature = 0.0;

  data = readI2c(HTS221_STATUS_REG);
  if ( data & HTS221_T_DA ) {
    T_OUT  = readI2c(HTS221_TEMP_OUT_H) << 8;
    T_OUT |= readI2c(HTS221_TEMP_OUT_L);
    // 1/8にする
    t_T0_degC = _T0_degC_x8 / 8.0;
    t_T1_degC = _T1_degC_x8 / 8.0;
    // 線形補間でもとめる
	Serial.println(t_T0_degC);
	Serial.println(t_T1_degC);
	Serial.println(T_OUT);
	Serial.println(_T0_OUT);
    temperature = t_T0_degC + ( t_T1_degC - t_T0_degC ) * ( T_OUT - _T0_OUT ) / ( _T1_OUT - _T0_OUT );
  }
  return temperature;
}

////////////////////////////////////////////////////////////////

/**
 @brief I2C読込
 @param [in] registerAddr レジスタアドレス
 @return 読込データ
*/
uint8_t FaBoHumidity_HTS221::readI2c(uint8_t registerAddr) {
  Wire.beginTransmission(_i2caddr);
  Wire.write(registerAddr);
  Wire.endTransmission();
  Wire.requestFrom(_i2caddr, (uint8_t)1);
  uint32_t time_now = millis();
  while(millis() - time_now < 50 && !Wire.available());
  return Wire.read();
}

/**
 @brief I2C書込
 @param [in] registerAddr レジスタアドレス
 @param [in] data 書込データ
*/
void FaBoHumidity_HTS221::writeI2c(uint8_t registerAddr, uint8_t data) {
  Wire.beginTransmission(_i2caddr);
  Wire.write(registerAddr);
  Wire.write(data);
  Wire.endTransmission();
}
