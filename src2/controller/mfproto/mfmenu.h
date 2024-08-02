#ifndef MF_MENU_H
#define MF_MENU_H

#include "debug.h"
extern DebugSerial debug;
extern mfBusRGB ledOK;
#include "mfsettings.h"
#include "mfsettings_data.h"
#include "mftranslation.h"
#include <vector>

#pragma GCC push_options
#pragma GCC optimize("Os")

enum mfMenuItemType {
  MF_CONTAINS_ITEM = 0,
  MF_CONTAINS_LIST,
  MF_CONTAINS_CALL
};
#define MFI(a)                                                                 \
  { MF_CONTAINS_ITEM, (a) }
#define MFL(a)                                                                 \
  { MF_CONTAINS_LIST, (a) }
#define MFC(a)                                                                 \
  { MF_CONTAINS_CALL, (a) }
#define MF_MENUMEMBER std::pair<mfMenuItemType, void *>

/******************************************************************************
 * as im a novice wizard, i don't know properly magical way to make templates *
 * for printf format strings, so just overloaded it with good-old define      *
 ******************************************************************************/
#define MAX_FORMAT_BUFFER 50
#define __DEFAULT_FORMAT__(T, f)                                               \
  String defaultFormat(T __dummy) {                                            \
    (void)__dummy;                                                             \
    return (f);                                                                \
  }

/* hope linker will delete unused overloads :/ */
__DEFAULT_FORMAT__(float, "%.2f");
__DEFAULT_FORMAT__(double, "%.2lf");
__DEFAULT_FORMAT__(uint8_t, "%u");
__DEFAULT_FORMAT__(uint16_t, "%u");
__DEFAULT_FORMAT__(uint32_t, "%lu");
__DEFAULT_FORMAT__(int, "%d");
__DEFAULT_FORMAT__(int8_t, "%d");
__DEFAULT_FORMAT__(int16_t, "%d");
__DEFAULT_FORMAT__(int32_t, "%ld");
__DEFAULT_FORMAT__(bool, "%u");
/******************************************************************************/

class mfMenuList; // need to declare it, definition is below
class mfMenuItemGen;
void menuWalkthrough(mfLCD &lcd, mfMenuList *current);
void processValue(mfLCD &lcd, mfMenuItemGen *item);

class mfMenuItemGen { // generic item, without real values
private:
  std::vector<mfMenuItemGen *>::iterator _me;

public:
  inline static std::vector<mfMenuItemGen *> instances = {};
  MF_TRANSLATION title;
  mfMenuList *parent;
  void *settingsHandle;
  void pushParent(mfMenuList *_parent) { parent = _parent; }
  bool disabled = false;
  std::function<bool()> disabledCheck = nullptr;
  std::function<void(mfMenuItemGen *)> onSet = nullptr;

  std::vector<MF_TRANSLATION> valueNames;
  mfMenuItemGen(MF_TRANSLATION _title, mfMenuList *_parent) {
    init(_title, _parent);
    settingsHandle = nullptr;
  }
  ~mfMenuItemGen() { deInit(); }
  void init(MF_TRANSLATION _title, mfMenuList *_parent) {
    instances.push_back(this);
    _me = instances.end();
    title = _title;
  }
  void deInit() { instances.erase(_me); }
  virtual void saveValue(uint32_t index) = 0;
  virtual uint32_t getValueIndex(void *value) = 0;
  virtual void clear(void) = 0;
  uint32_t getValueIndex(String &value) {
    for (uint32_t i = 0; i < valueNames.size(); i++)
      if (getTranslation(valueNames[i]) == value)
        return i;
    return ~0U; // not found;
  }
};

/******************************************************************************/

template <typename T> class mfMenuItem : public mfMenuItemGen {
  // templated child(s), real values appear here
private:
  String formatString = "";
  void initFormat(const char *format) {
    if (!format)
      formatString = defaultFormat((T)0);
    else
      formatString = String(format);
  }

public:
  std::vector<T> values;
  /* all constructors should fill two arrays, one with real values, another
   * with printable equivalent */
  void initValues(std::vector<std::pair<T, MF_TRANSLATION>> _values) {
    /* fill arrays from array of pairs */
    pushPairs(_values);
  }
  void initValues(std::vector<T> _values) {
    /* fill arrays from array of real values, printable
     * values are generated using formatItem call with
     * default or custom format string */
    std::vector<std::pair<T, MF_TRANSLATION>> _tempvalues;
    for (auto it = _values.begin(); it != _values.end(); it++)
      _tempvalues.push_back({*it, {formatItem(*it)}});
    pushPairs(_tempvalues);
  }
  void initValues(T start, T end, T step) {
    /* fill arrays with defined range of values, real values calculated with
     * start/end/step, printable values are generated using formatItem call
     * with default or custom format string */
    std::vector<std::pair<T, MF_TRANSLATION>> _tempvalues;
    /* align the end of range as (start + X*step) */
    end = start + (round((end - start) / step) * step);
    T i = start;
    do {
      _tempvalues.push_back({i, {formatItem(i)}});
      i += step;
    } while (i - step != end);
    pushPairs(_tempvalues);
  }
  void clear(void) {
    values.clear();
    valueNames.clear();
  }
  mfMenuItem(MF_TRANSLATION _title, T *_handle,
             std::vector<std::pair<T, MF_TRANSLATION>> _values,
             const char *format = nullptr)
      : mfMenuItemGen(_title, nullptr) {
    settingsHandle = (void *)_handle;
    initFormat(format);
    initValues(_values);
  }
  mfMenuItem(MF_TRANSLATION _title, T *_handle, std::vector<T> _values,
             const char *format = nullptr)
      : mfMenuItemGen(_title, nullptr) {
    settingsHandle = (void *)_handle;
    initFormat(format);
    initValues(_values);
  }
  mfMenuItem(MF_TRANSLATION _title, T *_handle, T start, T end, T step,
             const char *format = nullptr)
      : mfMenuItemGen(_title, nullptr) {
    settingsHandle = (void *)_handle;
    initFormat(format);
    initValues(start, end, step);
  }

  void pushPairs(std::vector<std::pair<T, MF_TRANSLATION>> _values) {
    /* split pairs and push into arrays */
    for (auto it = _values.begin(); it != _values.end(); it++) {
      values.push_back(it->first);
      valueNames.push_back(it->second);
    }
  }
  String formatItem(T value) {
    char buf[MAX_FORMAT_BUFFER];
    snprintf(buf, MAX_FORMAT_BUFFER - 1, formatString.c_str(), value);
    return String(buf);
  }
  ~mfMenuItem() {}
  void saveValue(uint32_t index) {
    /* write _selected_ real value to value handle */
    if (settingsHandle)
      *((T *)settingsHandle) = values.at(index);
    /* call callback after writing if exists */
    if (onSet) {
      onSet(this);
    }
  }
  uint32_t getValueIndex(void *value) {
    T localValue = *((T *)value);
    for (uint32_t i = 0; i < valueNames.size(); i++) {
      if (formatItem(values[i]) == formatItem(localValue))
        return i;
    }
    return ~0U; // not found;
  }
};

/******************************************************************************/
class mfMenuCall { // call subroutine from menu
private:
  std::vector<mfMenuCall *>::iterator _me;
  mfMenuList *parent;

public:
  inline static std::vector<mfMenuCall *> instances = {};
  MF_TRANSLATION title;
  std::function<void(mfMenuCall *call)> call;
  bool disabled = false;
  std::function<bool()> disabledCheck = nullptr;
  mfMenuCall(MF_TRANSLATION _title,
             std::function<void(mfMenuCall *call)> _call);
  ~mfMenuCall();
  void pushParent(mfMenuList *_parent);
};

class mfMenuList { // menu list container
private:
  std::vector<mfMenuList *>::iterator _me;

public:
  inline static std::vector<mfMenuList *> instances = {};
  MF_TRANSLATION title;
  mfMenuList *parent;
  bool disabled = false;
  std::function<bool()> disabledCheck = nullptr;
  void pushParent(mfMenuList *_parent) { parent = _parent; }
  std::vector<MF_MENUMEMBER> items = {};
  mfMenuList(MF_TRANSLATION _title, std::vector<MF_MENUMEMBER> _items,
             bool returnField = true);
  ~mfMenuList();
  std::vector<MF_MENUMEMBER> getItems(void);
  void setItemsParent(void);
};

mfMenuCall::mfMenuCall(MF_TRANSLATION _title,
                       std::function<void(mfMenuCall *call)> _call)
    : parent(nullptr), title(_title), call(_call) {
  instances.push_back(this);
  _me = instances.end();
}
mfMenuCall::~mfMenuCall() { instances.erase(_me); }
void mfMenuCall::pushParent(mfMenuList *_parent) { parent = _parent; }

mfMenuList::mfMenuList(MF_TRANSLATION _title, std::vector<MF_MENUMEMBER> _items,
                       bool returnField)
    : parent(nullptr) {
  title = _title;
  instances.push_back(this);
  _me = instances.end();
  /* insert all privided items to internal container */
  items.insert(items.end(), _items.begin(), _items.end());
  /* set parent of all contained lists */
  setItemsParent();
}
void mfMenuList::setItemsParent(void) {
  for (uint16_t it = 0; it < items.size(); it++) {
    switch (items[it].first) {
    case MF_CONTAINS_LIST: {
      mfMenuList *list = (mfMenuList *)items[it].second;
      list->pushParent(this);
    } break;
    case MF_CONTAINS_CALL: {
      mfMenuCall *call = (mfMenuCall *)items[it].second;
      call->pushParent(this);
    } break;
    case MF_CONTAINS_ITEM:
      break;
    }
  }
}
mfMenuList::~mfMenuList() { instances.erase(_me); }
std::vector<MF_MENUMEMBER> mfMenuList::getItems(void) {
  std::vector<MF_MENUMEMBER> tempItems = {};
  for (uint16_t i = 0; i < items.size(); i++) {
    switch (items[i].first) {
    case MF_CONTAINS_LIST: {
      mfMenuList *list = (mfMenuList *)items[i].second;
      if (list->disabledCheck)
        list->disabled = list->disabledCheck();
      if (!list->disabled)
        tempItems.push_back(items[i]);
    } break;
    case MF_CONTAINS_ITEM: {
      mfMenuItemGen *item = (mfMenuItemGen *)items[i].second;
      if (item->disabledCheck)
        item->disabled = item->disabledCheck();
      if (!item->disabled)
        tempItems.push_back(items[i]);
    } break;
    case MF_CONTAINS_CALL: {
      mfMenuCall *call = (mfMenuCall *)items[i].second;
      if (call->disabledCheck)
        call->disabled = call->disabledCheck();
      if (!call->disabled)
        tempItems.push_back(items[i]);
    } break;
    default:
      tempItems.push_back(items[i]);
      break;
    }
  }
  return tempItems;
}

/**************************************************************************/

mfSettings settings;
MF_SETTINGS &data = settings.data;
MF_LANGUAGE &LANG = settings.data.language;

MF_TRANSLATION TR_yes = {"Yes", "ДА"};
MF_TRANSLATION TR_no = {"No", "НЕТ"};
/**************************************************************************/
MF_TRANSLATION TR_language = {"Language", "Язык"};
MF_TRANSLATION TR_langeng = {"English"};
MF_TRANSLATION TR_langrus = {"Русский"};
__DEFAULT_FORMAT__(MF_LANGUAGE, "%u");
auto languageItem =
    new mfMenuItem<MF_LANGUAGE>(TR_language, &data.language,
                                {{ENGLISH, TR_langeng}, {RUSSIAN, TR_langrus}});
/**************************************************************************/
MF_TRANSLATION TR_axles = {"Wheels", "Оси"};
MF_TRANSLATION TR_axles_front = {"Front wheels only", "Только передняя ось"};
MF_TRANSLATION TR_axles_rear = {"Rear wheels only", "Только задняя ось"};
MF_TRANSLATION TR_axles_all = {"All wheels", "Все оси"};
__DEFAULT_FORMAT__(axleType, "%u");
auto axlesItem = new mfMenuItem<axleType>(TR_axles, &data.axles,
                                          {{MF_FRONT_AXLE, TR_axles_front},
                                           {MF_REAR_AXLE, TR_axles_rear},
                                           {MF_ALL_AXLES, TR_axles_all}});
/**************************************************************************/
MF_TRANSLATION TR_valves = {"Valve type", "Тип клапанов"};
MF_TRANSLATION TR_valves3 = {"3 valves", "3 клапана"};
MF_TRANSLATION TR_valves4 = {"4 valves", "4 клапана"};
MF_TRANSLATION TR_valves6 = {"6 valves", "6 клапанов"};
MF_TRANSLATION TR_valves8 = {"8 valves", "8 клапанов"};
__DEFAULT_FORMAT__(valveType, "%u");
auto valvesItem34 = new mfMenuItem<valveType>(
    TR_valves, &data.valves,
    {{MF_3VALVES, TR_valves3}, {MF_4VALVES, TR_valves4}});
auto valvesItem68 = new mfMenuItem<valveType>(
    TR_valves, &data.valves,
    {{MF_6VALVES, TR_valves6}, {MF_8VALVES, TR_valves8}});
/**************************************************************************/
MF_TRANSLATION TR_receiver = {"Receiver", "Ресивер"};
MF_TRANSLATION TR_present = {"Yes", "Есть"};
auto receiverItem = new mfMenuItem<bool>(TR_receiver, &data.receiverPresent,
                                         {{false, TR_no}, {true, TR_present}});
/**************************************************************************/
MF_TRANSLATION TR_compOnPressure = {"Compressor ON", "Компрессор вкл."};
MF_TRANSLATION TR_compOffPressure = {"Compressor OFF", "Компрессор выкл."};
MF_TRANSLATION TR_compEnable = {"Minimal pressure", "Минимальное давление"};
MF_TRANSLATION TR_compTempSensor = {"Overheat control", "Датчик температуры"};
MF_TRANSLATION TR_off = {"OFF", "ВЫКЛ"};
auto compOnPressureItem = new mfMenuItem<uint16_t>(
    TR_compOnPressure, &data.compOnPressure, MF_COMP_ON_START, MF_COMP_ON_END,
    MF_COMP_ONOFF_STEP, "%uPSI");

auto compOffPressureItem = new mfMenuItem<uint16_t>(
    TR_compOffPressure, &data.compOffPressure, MF_COMP_OFF_START,
    MF_COMP_OFF_END, MF_COMP_ONOFF_STEP, "%uPSI");

auto compEnableItem = new mfMenuItem<uint16_t>(
    TR_compEnable, &data.compEnablePressure, 0, 100, 10, "%uPSI");

auto compTempSensorItem = new mfMenuItem<uint16_t>(
    TR_compTempSensor, &data.compTempSensor, 50, 200, 10, "%u\xFС");
/**************************************************************************/
MF_TRANSLATION TR_dryer = {"Dryer", "Осушитель"};
MF_TRANSLATION TR_dryerOncomp = {"Compressor built-in", "На компрессоре"};
MF_TRANSLATION TR_dryerStandalone = {"Standalone", "Отдельный"};
auto dryerItem = new mfMenuItem<dryerType>(TR_dryer, &data.dryer,
                                           {{MF_DRYER_NO, TR_no},
                                            {MF_DRYER_YES, TR_dryerStandalone},
                                            {MF_DRYER_COMP, TR_dryerOncomp}});
MF_TRANSLATION TR_exhaustTime = {"Exhaust time", "Время продувки"};
auto exhaustTimeItem = new mfMenuItem<uint8_t>(
    TR_exhaustTime, &data.dryerExhaustTime, 0, 10, 1, "%u sec");
/**************************************************************************/
MF_TRANSLATION TR_speedSensEnabled = {"Speed control", "Модуль скорости"};
auto speedSensItem =
    new mfMenuItem<bool>(TR_speedSensEnabled, &data.speedSensEnabled,
                         {{false, TR_no}, {true, TR_present}});
MF_TRANSLATION TR_speedSensLimit1 = {"Limit 1", "Порог 1"};
MF_TRANSLATION TR_speedSensLimit2 = {"Limit 2", "Порог 2"};
MF_TRANSLATION TR_speedSensLimit2preset = {"Limit 2 preset", "Пресет порога 2"};
auto speedSensLimit1Item = new mfMenuItem<uint16_t>(
    TR_speedSensLimit1, &data.speedSensLimit1, 10, 100, 10, "%u km/h");
auto speedSensLimit2Item = new mfMenuItem<uint16_t>(
    TR_speedSensLimit2, &data.speedSensLimit2, 10, 100, 10, "%u km/h");
auto speedSensLimit2presetItem = new mfMenuItem<uint8_t>(
    TR_speedSensLimit2preset, &data.speedSensLimit2preset, 20, 80, 10, "%u%%");

auto speedSensList =
    new mfMenuList(TR_speedSensEnabled,
                   {MFI(speedSensItem), MFI(speedSensLimit1Item),
                    MFI(speedSensLimit2Item), MFI(speedSensLimit2presetItem)});
/**************************************************************************/
MF_TRANSLATION TR_engineSensEnabled = {"Engine start sense",
                                       "Контроль двигателя"};
auto engineSensItem =
    new mfMenuItem<bool>(TR_engineSensEnabled, &data.engineSensEnabled,
                         {{false, TR_no}, {true, TR_yes}});
/**************************************************************************/
MF_TRANSLATION TR_upOnStart = {"Height on start", "Подъем при старте"};
auto autoHeightItem = new mfMenuItem<bool>(TR_upOnStart, &data.upOnStart,
                                           {{false, TR_no}, {true, TR_yes}});

MF_TRANSLATION TR_autoHeightCorrection = {"Auto height tracking",
                                          "Автокоррекция клиренса"};
auto autoHeightCorrectionItem =
    new mfMenuItem<bool>(TR_autoHeightCorrection, &data.autoHeightCorrection,
                         {{false, TR_no}, {true, TR_yes}});

/**************************************************************************/
MF_TRANSLATION TR_buttonManualMode = {"Control button", "Режим кнопки"};
MF_TRANSLATION TR_buttonPresets = {"Presets", "Пресеты"};
MF_TRANSLATION TR_buttonManual = {"Manual", "Ручной"};
auto buttonManualModeItem =
    new mfMenuItem<bool>(TR_buttonManualMode, &data.buttonManualMode,
                         {{false, TR_buttonPresets}, {true, TR_buttonManual}});
/**************************************************************************/
MF_TRANSLATION TR_buttonOkColor = {"Middle button color",
                                   "Цвет средней кнопки"};
MF_TRANSLATION TR_buttonUpDownColor = {"Side buttons color",
                                       "Цвет крайних кнопок"};
#define BAR "\xE\xE\xE\xE\xE\xE\xE\xE##"
#define TR_COLOR(a, b)                                                         \
  MF_TRANSLATION a = {                                                         \
      String(BAR) + String(RGB(((b)&0xFF0000) >> (16 + 3),                     \
                               ((b)&0xFF00) >> (8 + 2), ((b)&0xFF) >> (3)))};
TR_COLOR(TR_white, MF_WHITE);
TR_COLOR(TR_red, MF_RED);
TR_COLOR(TR_green, MF_GREEN);
TR_COLOR(TR_blue, MF_BLUE);
TR_COLOR(TR_yellow, MF_YELLOW);
TR_COLOR(TR_violet, MF_VIOLET);
TR_COLOR(TR_cyan, MF_CYAN);
TR_COLOR(TR_orange, MF_ORANGE);
TR_COLOR(TR_salad, MF_SALAD);
TR_COLOR(TR_pink, MF_PINK);
TR_COLOR(TR_indigo, MF_INDIGO);
TR_COLOR(TR_azure, MF_AZURE);
TR_COLOR(TR_spring, MF_SPRING);
TR_COLOR(TR_coral, MF_CORAL);
TR_COLOR(TR_lightblue, MF_LIGHTBLUE);
TR_COLOR(TR_lightgreen, MF_LIGHTGREEN);
TR_COLOR(TR_witchhazel, MF_WITCHHAZEL);
TR_COLOR(TR_fuchsia, MF_FUCHSIA);
TR_COLOR(TR_aquamarine, MF_AQUAMARINE);
std::vector<std::pair<buttonColorType, MF_TRANSLATION>> _colors = {
    {MF_WHITE, TR_white},
    {MF_RED, TR_red},
    {MF_BLUE, TR_blue},
    {MF_GREEN, TR_green},
    {MF_YELLOW, TR_yellow},
    {MF_VIOLET, TR_violet},
    {MF_CYAN, TR_cyan},
    {MF_ORANGE, TR_orange},
    {MF_SALAD, TR_salad},
    {MF_PINK, TR_pink},
    {MF_INDIGO, TR_indigo},
    {MF_AZURE, TR_azure},
    {MF_SPRING, TR_spring},
    {MF_CORAL, TR_coral},
    {MF_LIGHTBLUE, TR_lightblue},
    {MF_LIGHTGREEN, TR_lightgreen},
    {MF_WITCHHAZEL, TR_witchhazel},
    {MF_FUCHSIA, TR_fuchsia},
    {MF_AQUAMARINE, TR_aquamarine}};
auto buttonOkColorItem = new mfMenuItem<buttonColorType>(
    TR_buttonOkColor, &data.buttonOkColor, _colors);
auto buttonUpDownColorItem = new mfMenuItem<buttonColorType>(
    TR_buttonUpDownColor, &data.buttonUpDownColor, _colors);
/**************************************************************************/
MF_TRANSLATION TR_buttonBrightness = {"Button brightness", "Яркость кнопки"};
auto buttonBrightnessItem = new mfMenuItem<uint8_t>(
    TR_buttonBrightness, &data.buttonBrightness, 10, 100, 10, "%u%%");
/**************************************************************************/
MF_TRANSLATION TR_sensitivity = {"Sensitivity", "Чувствительность"};
MF_TRANSLATION TR_senshigh = {"High", "Высокая"};
MF_TRANSLATION TR_sensmedium = {"Normal", "Нормальная"};
MF_TRANSLATION TR_senslow = {"Low", "Низкая"};
auto sensitivityItem = new mfMenuItem<uint8_t>(
    TR_sensitivity, &data.sensitivity,
    {{255, TR_senshigh}, {100, TR_sensmedium}, {20, TR_senslow}});
/**************************************************************************/
MF_TRANSLATION TR_delay = {"Delay", "Задержка"};
auto delayItem =
    new mfMenuItem<uint8_t>(TR_delay, &data.delay, 3, 10, 1, "%u sec");
/**************************************************************************/
MF_TRANSLATION TR_wifiEnabled = {"WIFI"};
auto wifiEnabledItem = new mfMenuItem<bool>(TR_wifiEnabled, &data.wifiEnabled,
                                            {{false, TR_no}, {true, TR_yes}});
/**************************************************************************/
MF_TRANSLATION TR_btEnabled = {"Bluetooth"};
auto btEnabledItem = new mfMenuItem<bool>(TR_btEnabled, &data.btEnabled,
                                          {{false, TR_no}, {true, TR_yes}});
/**************************************************************************/
MF_TRANSLATION TR_compressorEnabled = {"Compressor", "Компрессор"};
auto compressorEnabledItem =
    new mfMenuItem<bool>(TR_compressorEnabled, &data.compressorEnabled,
                         {{false, TR_no}, {true, TR_present}});
/**************************************************************************/
MF_TRANSLATION TR_speedLock = {"Compressor lock", "Блокировка компрессора"};
auto speedLockItem = new mfMenuItem<uint8_t>(TR_speedLock, &data.speedLock, 10,
                                             100, 10, "%u km/h");
/**************************************************************************/
MF_TRANSLATION TR_voltageCorrection = {"Voltage correction",
                                       "Калибровка напряжения"};
auto voltageCorrectionItem = new mfMenuItem<float>(
    TR_voltageCorrection, &data.voltageCorrection, -1.0f, 1.0f, 0.1f, "%.1fV");
/**************************************************************************/
MF_TRANSLATION TR_preset1 = {"Preset 1", "Пресет 1"};
MF_TRANSLATION TR_preset2 = {"Preset 2", "Пресет 2"};
MF_TRANSLATION TR_preset3 = {"Preset 3", "Пресет 3"};
MF_TRANSLATION TR_preset4 = {"Preset 4", "Пресет 4"};
MF_TRANSLATION TR_preset5 = {"Preset 5", "Пресет 5"};

auto preset1Item =
    new mfMenuItem<uint8_t>(TR_preset1, &data.presets[0], 0, 100, 10, "%u%%");
auto preset2Item =
    new mfMenuItem<uint8_t>(TR_preset2, &data.presets[1], 0, 100, 10, "%u%%");
auto preset3Item =
    new mfMenuItem<uint8_t>(TR_preset3, &data.presets[2], 0, 100, 10, "%u%%");
auto preset4Item =
    new mfMenuItem<uint8_t>(TR_preset4, &data.presets[3], 0, 100, 10, "%u%%");
auto preset5Item =
    new mfMenuItem<uint8_t>(TR_preset5, &data.presets[4], 0, 100, 10, "%u%%");
/**************************************************************************/
MF_TRANSLATION TR_save = {"Save settings", "Сохранить настройки"};
auto saveCall = new mfMenuCall(TR_save, [](mfMenuCall *call) {
  vPortEnterCritical();
  settings.save();
  vPortExitCritical();
});

MF_TRANSLATION TR_resetSettings = {"Reset settings", "Сбросить настройки"};
auto resetCall = new mfMenuCall(TR_resetSettings,
                                [](mfMenuCall *call) { settings.reset(); });

MF_TRANSLATION TR_recvSettings = {"Receiver settings", "Настройки ресивера"};
auto receiverSettingsList = new mfMenuList(
    TR_recvSettings, {MFI(compOnPressureItem), MFI(compOffPressureItem),
                      MFI(compEnableItem), MFI(compTempSensorItem)});

MF_TRANSLATION TR_buttonSetup = {"Button setup", "Настройка кнопки"};
auto buttonSetupList = new mfMenuList(
    TR_buttonSetup, {MFI(buttonOkColorItem), MFI(buttonUpDownColorItem),
                     MFI(buttonBrightnessItem)});

MF_TRANSLATION TR_calibration = {"Calibration", "Калибровка"};
MF_TRANSLATION TR_manualCalibration = {"Manual calibration",
                                       "Ручная калибровка"};
MF_TRANSLATION TR_autoCalibration = {"Autocalibration ", "Автокалибровка"};
MF_TRANSLATION TR_calAdjust = {"Adjustment", "Корректировка"};
MF_TRANSLATION TR_speedCalibration = {"Calibrate Speed", "Калибровка скорости"};
MF_TRANSLATION TR_frontLeft = {"Front left", "Переднее левое"};
MF_TRANSLATION TR_frontRight = {"Front Right", "Переднее правое"};
MF_TRANSLATION TR_rearLeft = {"Rear left", "Заднее левое"};
MF_TRANSLATION TR_rearRight = {"Rear right", "Заднее правое"};
MF_TRANSLATION TR_front = {"Front wheels", "Передняя ось"};
MF_TRANSLATION TR_rear = {"Rear wheels", "Задняя ось"};

auto speedCalibrationCall =
    new mfMenuCall(TR_speedCalibration, [](mfMenuCall *call) {});

auto calFrontLeftCall = new mfMenuCall(TR_frontLeft, [](mfMenuCall *call) {});
auto calFrontRightCall = new mfMenuCall(TR_frontRight, [](mfMenuCall *call) {});
auto calRearLeftCall = new mfMenuCall(TR_rearLeft, [](mfMenuCall *call) {});
auto calRearRightCall = new mfMenuCall(TR_rearRight, [](mfMenuCall *call) {});
auto calFrontCall = new mfMenuCall(TR_front, [](mfMenuCall *call) {});
auto calRearCall = new mfMenuCall(TR_rear, [](mfMenuCall *call) {});

auto manualCalibrationList = new mfMenuList(
    TR_manualCalibration,
    {MFC(calFrontLeftCall), MFC(calFrontRightCall), MFC(calRearLeftCall),
     MFC(calRearRightCall), MFC(calFrontCall), MFC(calRearCall)});

auto autoCalibrationCall =
    new mfMenuCall(TR_autoCalibration, [](mfMenuCall *call) {});

auto calibrationList = new mfMenuList(
    TR_calibration, {MFI(axlesItem), MFI(valvesItem34), MFI(valvesItem68),
                     MFC(autoCalibrationCall), MFL(manualCalibrationList),
                     MFC(speedCalibrationCall)});

MF_TRANSLATION TR_reset = {"Reset", "Сброс"};
auto resetList = new mfMenuList(TR_reset, {MFC(resetCall)});

MF_TRANSLATION TR_manualControl = {"Manual control", "Ручное управление"};
MF_TRANSLATION TR_manualDryer = {"Dryer exhaust", "Продувка осушителя"};
MF_TRANSLATION TR_manualComp = {"Compressor", "Компрессор"};
auto manualDryerCall = new mfMenuCall(TR_manualDryer, [](mfMenuCall *call) {});
auto manualCompressorCall =
    new mfMenuCall(TR_manualComp, [](mfMenuCall *call) {});
auto manualControlList = new mfMenuList(
    TR_manualControl, {MFC(manualCompressorCall), MFC(manualDryerCall)});

MF_TRANSLATION TR_systemList = {"System settings", "Системное меню"};
MF_TRANSLATION TR_information = {"Information", "Информация"};
auto informationCall = new mfMenuCall(TR_information, [](mfMenuCall *call) {
  extern mfLCD lcd;
  const uint16_t TEXT_COLOR = RGB(31, 62, 31);
  lcd.boxwh(0, 20, lcd.width, lcd.height - 35, RGB(12, 18, 10));
  String model = getTranslation({"Model:", "Модель:"}, LANG);
  String modelV = "MFTech AirLogic PRO";
  String board = getTranslation({"Revision:", "Ревизия:"}, LANG);
  String revision = "REV 0.36";
  String firmware =
      getTranslation({"Firmware version:", "Версия прошивки:"}, LANG);
  String firmwareV = "0.0.1";
  lcd.print(4, 40, model, TEXT_COLOR);
  lcd.print(4, 60, board, TEXT_COLOR);
  lcd.print(4, 80, firmware, TEXT_COLOR);
  /*lcd.print(lcd.width - (modelV).length() * 6 + 5, 40, modelV, TEXT_COLOR);
  lcd.print(lcd.width - (revision).length() * 6 - 4, 60, revision, TEXT_COLOR);
  lcd.print(lcd.width - (firmwareV).length() * 6 - 1, 80, firmwareV,
            TEXT_COLOR);*/
  lcd.printR(lcd.width - 4, 40, modelV, TEXT_COLOR);
  lcd.printR(lcd.width - 4, 60, revision, TEXT_COLOR);
  lcd.printR(lcd.width - 4, 80, firmwareV, TEXT_COLOR);
  DELAY(5000);
});
auto systemList = new mfMenuList(
    TR_systemList,
    {MFC(informationCall), MFI(languageItem), MFI(speedLockItem),
     MFI(voltageCorrectionItem), MFL(manualControlList), MFL(resetList)});

MF_TRANSLATION TR_wizard = {"Setup wizard", "Мастер настройки"};
auto wizardList = new mfMenuList(
    TR_wizard,
    {MFI(languageItem), MFI(axlesItem), MFI(valvesItem34), MFI(valvesItem68),
     MFI(compressorEnabledItem), MFI(receiverItem), MFL(receiverSettingsList),
     MFI(dryerItem), MFL(speedSensList), MFI(engineSensItem),
     MFI(autoHeightItem), MFI(buttonManualModeItem), MFL(buttonSetupList),
     MFC(autoCalibrationCall)});

MF_TRANSLATION TR_controls = {"Controls", "Управление"};
auto controlsList = new mfMenuList(
    TR_controls, {MFI(buttonManualModeItem), MFL(buttonSetupList)});

MF_TRANSLATION TR_presets = {"Presets", "Пресеты"};
auto presetsList = new mfMenuList(
    TR_presets, {MFI(preset1Item), MFI(preset2Item), MFI(preset3Item),
                 MFI(preset4Item), MFI(preset5Item)});

MF_TRANSLATION TR_controller = {"Controller", "Контроллер"};
auto controllerList = new mfMenuList(
    TR_controller,
    {MFI(sensitivityItem), MFI(delayItem), MFI(engineSensItem),
     MFI(autoHeightItem), MFI(autoHeightCorrectionItem), MFL(controlsList),
     MFL(presetsList), MFI(compressorEnabledItem), MFI(receiverItem),
     MFL(receiverSettingsList), MFI(dryerItem), MFI(exhaustTimeItem),
     MFL(speedSensList)});

MF_TRANSLATION TR_wifibt = {"WIFI/BT settings", "Настройки WIFI/BT"};
auto wifibtList =
    new mfMenuList(TR_wifibt, {MFI(wifiEnabledItem), MFI(btEnabledItem)});
MF_TRANSLATION TR_settings = {"Settings", "Настройки"};
auto settingsList = new mfMenuList(
    TR_settings, {MFL(wizardList), MFL(calibrationList), MFL(controllerList),
                  MFL(wifibtList), MFL(systemList)});

void applyItemRules(void) {
  /* create dynamic item rules */
  compressorEnabledItem->onSet = [](mfMenuItemGen *value) {
    if (!data.compressorEnabled) {
      /* if compressor disabled, set dryer and receiver disabled */
      data.dryer = MF_DRYER_NO;
      data.receiverPresent = false;
    }
  };
  dryerItem->disabledCheck = []() {
    /* hide dryer in case of disabled compressor */
    return (!data.compressorEnabled);
  };
  receiverItem->disabledCheck = []() {
    /* hide receiver in case of disabled compressor */
    return (!data.compressorEnabled);
  };
  manualDryerCall->disabledCheck = []() { return (data.dryer == MF_DRYER_NO); };
  manualCompressorCall->disabledCheck = []() {
    return (!data.compressorEnabled);
  };
  /* process state of compressor at start */
  compressorEnabledItem->onSet(nullptr);

  valvesItem34->disabledCheck = []() {
    /* if all axles selected, disable 3/4 valves selector */
    return (data.axles == MF_ALL_AXLES);
  };
  valvesItem68->disabledCheck = []() {
    /* if not all axles selected, disable 6/8 valves selector */
    return (data.axles != MF_ALL_AXLES);
  };
  calFrontLeftCall->disabledCheck = []() {
    return (data.axles != MF_ALL_AXLES);
  };
  calFrontRightCall->disabledCheck = []() {
    return (data.axles != MF_ALL_AXLES);
  };
  calRearLeftCall->disabledCheck = []() {
    return (data.axles != MF_ALL_AXLES);
  };
  calRearRightCall->disabledCheck = []() {
    return (data.axles != MF_ALL_AXLES);
  };
  calFrontCall->disabledCheck = []() { return (data.axles != MF_FRONT_AXLE); };
  calRearCall->disabledCheck = []() { return (data.axles != MF_REAR_AXLE); };

  speedSensLimit1Item->disabledCheck = []() {
    return (!data.speedSensEnabled);
  };
  speedSensLimit2Item->disabledCheck = []() {
    return (!data.speedSensEnabled);
  };
  speedSensLimit2presetItem->disabledCheck = []() {
    return (!data.speedSensEnabled);
  };
  axlesItem->onSet = [](mfMenuItemGen *) {
    /* when axles setting changed, change valves setting respectively */
    uint8_t *p_valves = (uint8_t *)&data.valves;
    if (data.axles == MF_ALL_AXLES)
      *p_valves |= MF_ALL_AXLES_BIT; // set bit
    else
      *p_valves &= ~MF_ALL_AXLES_BIT; // reset bit
  };
  receiverSettingsList->disabledCheck = []() {
    /* disable receiver settings if receiver disabled */
    return (!data.receiverPresent);
  };
  exhaustTimeItem->disabledCheck = []() {
    /* disable dryer settings if dryer disabled */
    return (data.dryer != MF_DRYER_YES);
  };
  dryerItem->onSet = [](mfMenuItemGen *item) {
    /*little hack here, call other value selection after selection dryer
     * type*/
    if ((data.dryer == MF_DRYER_YES) /*&& (item->parent == wizardList)*/) {
      extern mfLCD lcd;
      processValue(lcd, exhaustTimeItem);
    }
  };
  /* add off setting to temp sensor */
  compTempSensorItem->initValues({{0, TR_off}});
  /* adjust compressor on/off setting depending on each other*/
  compOffPressureItem->onSet = [](mfMenuItemGen *) {
    uint16_t topOnmagrin = (data.compOffPressure > MF_COMP_ON_END)
                               ? MF_COMP_ON_END
                               : data.compOffPressure - MF_COMP_ONOFF_STEP;
    compOnPressureItem->clear();
    compOnPressureItem->initValues(MF_COMP_ON_START, topOnmagrin,
                                   MF_COMP_ONOFF_STEP);
    if (data.compOnPressure > topOnmagrin)
      data.compOnPressure = topOnmagrin;
  };
  compOnPressureItem->onSet = [](mfMenuItemGen *) {
    uint16_t bottomOffmagrin = (data.compOnPressure < MF_COMP_OFF_START)
                                   ? MF_COMP_OFF_START
                                   : data.compOnPressure + MF_COMP_ONOFF_STEP;
    compOffPressureItem->clear();
    compOffPressureItem->initValues(bottomOffmagrin, MF_COMP_OFF_END,
                                    MF_COMP_ONOFF_STEP);
    if (data.compOffPressure < bottomOffmagrin)
      data.compOffPressure = bottomOffmagrin;
  };
  compOffPressureItem->onSet(nullptr);
  compOnPressureItem->onSet(nullptr);
  /* temporary use lcd rgb led */
  extern mfBusRGB ledOK;
  extern mfBusRGB ledUP;
  extern mfBusRGB ledDOWN;

  /* add immediate action after color selection */
  buttonUpDownColorItem->onSet = [](mfMenuItemGen *) {
    /*uint8_t r, g, b;
    r = (data.buttonUpDownColor & 0xFF0000) >> 16;
    g = (data.buttonUpDownColor & 0xFF00) >> 8;
    b = (data.buttonUpDownColor & 0xFF);*/
    ledUP.setColor(data.buttonUpDownColor);
    ledDOWN.setColor(data.buttonUpDownColor);
  };
  buttonOkColorItem->onSet = [](mfMenuItemGen *) {
    /*uint8_t r, g, b;
    r = (data.buttonOkColor & 0xFF0000) >> 16;
    g = (data.buttonOkColor & 0xFF00) >> 8;
    b = (data.buttonOkColor & 0xFF);*/
    // ledOK.setColor(r, g, b);
    ledOK.setColor(data.buttonOkColor);
    // ledOK.setColor(RGB(r, g, b));
  };
  /* action to set brightness immediately */
  buttonBrightnessItem->onSet = [](mfMenuItemGen *) {
    ledOK.setBrightness(data.buttonBrightness);
    ledUP.setBrightness(data.buttonBrightness);
    ledDOWN.setBrightness(data.buttonBrightness);
  };
  /* run em first time */
  buttonOkColorItem->onSet(nullptr);
  buttonUpDownColorItem->onSet(nullptr);
  buttonBrightnessItem->onSet(nullptr);
  /* enable led */
  ledOK.on(~0U);
  ledDOWN.on(~0U);
  ledUP.on(~0U);
}

MF_STRINGS printNodes(mfMenuList *node, uint16_t current, uint16_t max = 7) {
  MF_STRINGS ret;
  ret.sel = UINT16_MAX;
  if (!max)
    return ret;
  int32_t allmembers = node->getItems().size();
  int32_t start = current - (max / 2);
  if (allmembers - start < max) {
    start = allmembers - max;
  }
  if (start < 0)
    start = 0;
  int32_t end = start + max;
  uint16_t counter = 0;
  for (int32_t i = start; i < end; i++) {
    // String selected = (i == current) ? ">" : " ";
    if (i == current)
      ret.sel = counter;
    counter++;

    if ((i < 0) || (i >= allmembers)) {
      ret.str.push_back("");
      ret.vals.push_back("");
    } else {
      switch (node->getItems()[i].first) {
      case MF_CONTAINS_CALL: {
        mfMenuCall *call = (mfMenuCall *)node->getItems()[i].second;
        ret.str.push_back(
            printfString("<%s>", getTranslation(call->title, LANG).c_str()));
        ret.vals.push_back("");
      } break;
      case MF_CONTAINS_LIST: {
        mfMenuList *list = (mfMenuList *)node->getItems()[i].second;
        ret.str.push_back(
            printfString(">%s", getTranslation(list->title, LANG).c_str()));
        ret.vals.push_back("");
      } break;
      case MF_CONTAINS_ITEM: {
        mfMenuItemGen *item = (mfMenuItemGen *)node->getItems()[i].second;
        ret.str.push_back(
            printfString(" %s", getTranslation(item->title, LANG).c_str()));
        uint32_t curValue = item->getValueIndex(item->settingsHandle);
        if (curValue > item->valueNames.size())
          curValue = 0;
        if (getTranslation(item->valueNames[curValue], LANG)
                .lastIndexOf("##") == -1)
          ret.vals.push_back(printfString(
              " %s", getTranslation(item->valueNames[curValue], LANG).c_str()));
        else
          ret.vals.push_back("");
      } break;
      }
    }
  }
  return ret;
}

MF_STRINGS printValues(mfMenuItemGen *node, uint16_t current,
                       uint16_t max = 7) {
  MF_STRINGS ret;
  ret.sel = UINT16_MAX;
  if (!max)
    return ret;
  uint32_t allmembers = node->valueNames.size();
  int32_t start = current - (max / 2);
  if (allmembers - start < max)
    start = allmembers - max;
  if (start < 0)
    start = 0;
  int32_t end = start + max;
  uint16_t counter = 0;
  for (int32_t i = start; i < end; i++) {
    if (i == current)
      ret.sel = counter;
    counter++;
    if ((i < 0) || (i >= (int32_t)allmembers)) {
      ret.str.push_back("");
      ret.vals.push_back("");
    } else {
      ret.str.push_back(printfString(
          "%s", getTranslation(node->valueNames[i], LANG).c_str()));
      ret.vals.push_back("");
    }
  }
  return ret;
}
MFLCD_FONTDATA fontData1 = {0};
MFLCD_FONTDATA fontData2 = {0};
/* convert UTF string to 8bit string, it's mostly made to adapt strings for
 * LCD, but here it's useful to count letters */
inline String RUS(String source) {
  uint32_t length = source.length();
  uint32_t i = 0;
  while (i < length) {
    switch (source[i]) {
    case 208:
      source.remove(i, 1);
      length--;
      break;
    case 209:
      source.remove(i, 1);
      length--;
      source[i] += 64;
      break;
    default:
      break;
    }
    i++;
  }
  return source;
}
#define TEXT_INTERVAL 15
#define HEADER_COLOR RGB(10, 16, 8)
#define HEADER_TEXT RGB(31, 62, 31)
#define ITEM_COLOR RGB(8, 12, 6)
#define ITEM_TEXT RGB(24, 48, 24)
#define STATUS_COLOR RGB(8, 14, 8)
#define STATUS_TEXT RGB(21, 35, 18)
#define WINDOW_COLOR RGB(3, 3, 3)
#define WINDOW_BORDER RGB(10, 17, 10)
void lcdUpdateMenu(mfLCD &lcd, mfMenuList *curList, uint16_t curPos) {
  uint16_t GENERIC_LINES = (lcd.height - (TEXT_INTERVAL + 20)) / TEXT_INTERVAL;
  lcd.noRefresh();
  lcd.clear(0);
  lcd.boxwh(0, 0, lcd.width, 20, HEADER_COLOR);
  lcd.setFont(1, &fontData1);
  lcd.boxwh(0, lcd.height - TEXT_INTERVAL, lcd.width, TEXT_INTERVAL,
            STATUS_COLOR);
  String header = getTranslation(curList->title, LANG);
  lcd.printC(0, lcd.width, 13, header, HEADER_TEXT);
  lcd.setFont(0, &fontData2);
  MF_STRINGS lines = printNodes(curList, curPos, GENERIC_LINES);
  lcd.boxwh(0, 20 + (TEXT_INTERVAL * lines.sel), lcd.width, TEXT_INTERVAL,
            ITEM_COLOR);
  lcd.print(4, lcd.height - 6, lines.vals[lines.sel], STATUS_TEXT);
  for (uint32_t i = 0; i < lines.str.size(); i++) {
    lcd.print(4, 21 + fontData2.yAdvance + (TEXT_INTERVAL * i), lines.str[i],
              ITEM_TEXT);
  }
  lcd.refresh();
}
void dumpDebug(mfBus *bus, uint8_t device);
void lcdUpdateValues(mfLCD &lcd, mfMenuItemGen *curItem, uint16_t curPos) {
  uint16_t WINDOW_OFFSET = 12;
  uint16_t GENERIC_LINES = (lcd.height - (WINDOW_OFFSET * 2)) / TEXT_INTERVAL;
  uint16_t WINDOW_H_OFFSET;
  uint16_t LINES_OF_VALUES = GENERIC_LINES;
  uint16_t valuesCount = curItem->valueNames.size();
  if (valuesCount < LINES_OF_VALUES) {
    LINES_OF_VALUES = valuesCount;
  }
  uint16_t valuesSize = (LINES_OF_VALUES + 1) * TEXT_INTERVAL;
  WINDOW_H_OFFSET = (lcd.height - valuesSize) / 2;

  lcd.noRefresh();
  lcd.boxwh(WINDOW_OFFSET - 1, WINDOW_H_OFFSET, lcd.width - (WINDOW_OFFSET * 2),
            lcd.height - (WINDOW_H_OFFSET * 2) - 1, WINDOW_COLOR);
  lcd.boxwh(WINDOW_OFFSET - 1, WINDOW_H_OFFSET, lcd.width - (WINDOW_OFFSET * 2),
            TEXT_INTERVAL, HEADER_COLOR);
  String header = getTranslation(curItem->title, LANG);
  lcd.setFont(0, &fontData2);
  lcd.printC(0, lcd.width, WINDOW_H_OFFSET + 10, header, HEADER_TEXT);
  MF_STRINGS lines = printValues(curItem, curPos, LINES_OF_VALUES);
  lcd.boxwh(WINDOW_OFFSET - 1,
            WINDOW_H_OFFSET + TEXT_INTERVAL + (TEXT_INTERVAL * lines.sel),
            lcd.width - (WINDOW_OFFSET * 2), TEXT_INTERVAL, ITEM_COLOR);
  for (uint32_t i = 0; i < lines.str.size(); i++) {
    String text = lines.str[i];
    uint16_t color = ITEM_TEXT;
    if (text.lastIndexOf("##") != -1) {
      uint16_t colorPos = text.lastIndexOf("##");
      String colorStr = text.substring(colorPos + 2);
      text = text.substring(0, colorPos);
      color = colorStr.toInt();
    }
    lcd.print(WINDOW_OFFSET + 3,
              TEXT_INTERVAL + WINDOW_H_OFFSET + 1 + fontData2.yAdvance +
                  (TEXT_INTERVAL * i),
              text, color);
  }
  lcd.rect(WINDOW_OFFSET - 1, WINDOW_H_OFFSET, lcd.width - WINDOW_OFFSET - 1,
           lcd.height - WINDOW_H_OFFSET - 1, WINDOW_BORDER);
  lcd.refresh();
}

std::vector<mfBusButton *> lcdButtons = {};
/* enumerate buttons, just for readability */
enum _lcdButtons { LCD_UP = 0, LCD_OK = 1, LCD_DOWN = 2 };

void processValue(mfLCD &lcd, mfMenuItemGen *item) {
  uint32_t cur = item->getValueIndex(item->settingsHandle);
  if (cur > item->valueNames.size())
    cur = 0;
  lcdUpdateValues(lcd, item, cur);
  while (1) {
    for (uint8_t i = 0; i < lcdButtons.size(); i++) {
      mfButtonFlags state = lcdButtons[i]->getState();
      if (state != MFBUTTON_NOSIGNAL) {
        // ledOK.blink(200, 100);
        switch (state) {
        case MFBUTTON_PRESSED:
          break;
        case MFBUTTON_PRESSED_LONG:
          if (i == LCD_OK) {
            return;
          }
          break;
        case MFBUTTON_RELEASED:
          if (i == LCD_UP) {
            if (cur > 0)
              cur--;
            else
              cur = item->valueNames.size() - 1;
            lcdUpdateValues(lcd, item, cur);
          }
          if (i == LCD_OK) {
            if (cur < item->valueNames.size()) {
              item->saveValue(cur);
              return;
            }
            lcdUpdateValues(lcd, item, cur);
          }
          if (i == LCD_DOWN) {
            if (cur + 1 < item->valueNames.size())
              cur++;
            else
              cur = 0;
            lcdUpdateValues(lcd, item, cur);
          }
          break;
        case MFBUTTON_RELEASED_LONG:
          break;
        default:;
        }
      }
      DELAY(10);
    }
    DELAY(10);
    YIELD();
  }
}
void menuWalkthrough(mfLCD &lcd, mfMenuList *current) {
  uint16_t currentPos = 0;
  current->setItemsParent();
  lcdUpdateMenu(lcd, current, currentPos);
  while (1) {
    for (uint8_t i = 0; i < lcdButtons.size(); i++) {
      mfButtonFlags state = lcdButtons[i]->getState();
      if (state != MFBUTTON_NOSIGNAL) {
        // ledOK.blink(100, 100);
        switch (state) {
        case MFBUTTON_PRESSED:
          break;
        case MFBUTTON_PRESSED_LONG:
          if (i == LCD_OK) {
            /*if (current->parent)
              current = current->parent;
            current->setItemsParent();
            currentPos = 0;*/
            vPortEnterCritical();
            settings.save();
            vPortExitCritical();
            // if (current->parent)
            return;
            // lcdUpdateMenu(lcd, current, currentPos);
          }
          break;
        case MFBUTTON_RELEASED:
          if (i == LCD_UP) {
            if (currentPos > 0)
              currentPos--;
            /*else
              currentPos = current->getItems().size() - 1;*/
            lcdUpdateMenu(lcd, current, currentPos);
          }
          if (i == LCD_OK) {
            switch (current->getItems()[currentPos].first) {
            case MF_CONTAINS_CALL: {
              mfMenuCall *callObj =
                  (mfMenuCall *)current->getItems()[currentPos].second;
              if (callObj->call) {
                callObj->call(callObj);
              }
            } break;
            case MF_CONTAINS_LIST: {
              /*current = (mfMenuList
              *)(current->getItems()[currentPos].second);
              current->setItemsParent();
              currentPos = 0;*/
              menuWalkthrough(
                  lcd, (mfMenuList *)(current->getItems()[currentPos].second));
            } break;
            case MF_CONTAINS_ITEM: {
              void *tempItemPtr = current->getItems()[currentPos].second;
              processValue(
                  lcd, (mfMenuItemGen *)current->getItems()[currentPos].second);
              /* we need to find a new current position in case of structure
               * changed */
              currentPos = 0; // initially at top
              std::vector<MF_MENUMEMBER> tempItems = current->getItems();
              for (uint32_t i = 0; i < tempItems.size(); i++)
                if (tempItems[i].second == tempItemPtr) {
                  currentPos = i;
                  break;
                }
            } break;
            default:
              break;
            }
            lcdUpdateMenu(lcd, current, currentPos);
          }
          if (i == LCD_DOWN) {
            if (currentPos + 1 < (uint16_t)current->getItems().size())
              currentPos++;
            /*else
              currentPos = 0;*/
            lcdUpdateMenu(lcd, current, currentPos);
          }
          break;
        case MFBUTTON_RELEASED_LONG:
          break;
        default:;
        }
      }
      DELAY(15);
    }
    YIELD();
  }
}

#if 1
void list_nodes(mfMenuList *root, int step = 0) {
  String spacer = "";
  for (auto i = 0; i < step + 3; i++)
    spacer += "*";
  debug.printf("[%s]", getTranslation(root->title, LANG).c_str());
  if (root->parent)
    debug.printf(", parent [%s]:\n",
                 getTranslation(root->parent->title, LANG).c_str());
  else
    debug.printf(":\n");

  for (uint16_t i = 0; i < root->items.size(); i++) {
    switch (root->items[i].first) {
    case MF_CONTAINS_LIST: {
      mfMenuList *list = (mfMenuList *)root->items[i].second;
      debug.printf("%s", spacer.c_str());
      debug.printf(" List[%u]: ", i);
      list_nodes(list, step + 3); // recurse into the next node
    } break;
    case MF_CONTAINS_ITEM: {
      mfMenuItemGen *item = (mfMenuItemGen *)(root->items[i].second);
      debug.printf("%s", spacer.c_str());
      debug.printf(" Item[%u]: %s ", i,
                   getTranslation(item->title, LANG).c_str());
      debug.printf("(%u): ", item->valueNames.size());
      for (uint16_t z = 0; z < item->valueNames.size(); z++)
        debug.printf("%s ", getTranslation(item->valueNames[z], LANG).c_str());
      debug.printf("\n");
    } break;
    case MF_CONTAINS_CALL: {
      mfMenuCall *call = (mfMenuCall *)(root->items[i].second);
      debug.printf("%s", spacer.c_str());
      debug.printf(" Call[%u]: %s\n", i,
                   getTranslation(call->title, LANG).c_str());
    } break;
    }
  }
}
#endif

#pragma GCC pop_options
#endif
